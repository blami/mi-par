/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srpnompi: paralelni (OpenMPI) resitel ulohy
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <mpi.h>
#include "srpprint.h"
#include "srputils.h"
#include "srptask.h"
#include "srphist.h"
#include "srpdump.h"
#include "srpstack.h"

char *filename = NULL;
task_t *t, *tf;
stack_t *s;
hist_t *h;
move_t m;
stack_item_t *solution;     // nejlepsi nalezene reseni
unsigned int cc;            // pocitadlo analyzovanych stavu
int work_done = 0;

const int mpi_msg_max = 150;
int mpi_best_p = -1;        // nejlepsi dosazena orezova hodnota
int mpi_token_wait = 0;     // cekam na vracejiciho se peska
int mpi_msg_try = 0;        // pocet pokusu o ziskani prace (4 * pocet uzlu)
int mpi_token[2];           // pesek (0: id zadatele, 1: barva)
int node = NONE;            // mpi node (rank)
int node_count = NONE;      // pocet vypocetnich uzlu

/**
 * Zpravy MPI
 */
typedef enum {
	MSG_REQUEST = 1,        // zadost MPI_INT
	MSG_STACK,               // poz. odpoved MPI_INT (velikost prace)
	MSG_STACK_DATA,               // poz. odpoved MPI_PACKED (zasobnik)
	MSG_NOSTACK,             // neg. odpoved MPI_INT
	MSG_SOLUTION,           // zprava o soucasnem nejlepsim reseni
	MSG_TOKEN,              // token
	                        // (kdyz mam praci a prijde mi 1:0 pesek,
	                        // poslu TASK zadateli)
	MSG_UNKNOWN
} mpi_tag;

/**
 * Zpracovat prepinace vcetne overeni spravnosti.
 */
void parse_args(int argc, char **argv)
{
	int a;
	while((a = getopt(argc, argv, "vh")) != -1){
		switch(a) {
		case 'h':
			printf("srpmpi: paralelni (OpenMPI) resitel ulohy SRP\n\n"
				"Pouziti: srpnompi [prepinace] <soubor>\n"
				" -h            zobrazi tuto napovedu\n\n");
			exit(EXIT_SUCCESS);
			break;
		}
	}

	if(optind < argc) {
		if(optind + 1 < argc) {
			srpfprintf(stderr, node, "chyba: neocekavany argument");
			exit(EXIT_FAILURE);
		}

		filename = strndup(argv[optind], strlen(argv[optind]));
		return;
	}

	srpfprintf(stderr, node, "chyba: ocekavan nazev vstupniho souboru "
		"se zadanim ulohy");
	exit(EXIT_FAILURE);
}

/**
 * Nacteni ulohy ze souboru.
 */
int read_file(char *filename) {
	FILE *f = NULL;
	int r = 0;

	if(!(f = fopen(filename, "r"))) {
		srpfprintf(stderr, node, "chyba: nelze otevrit soubor `%s'",
			filename);
		exit(EXIT_FAILURE);
	}

	if(t = dump_unserialize(f)) {
		r = 1;
	}

	fclose(f);
	return r;
}

/**
 * Porovnani dvou sachovnic (indexy figurek nemusi sedet, jde jen o zaplneni
 * stejnych pozic).
 * \returns     1 pro stejne sachovnice, jinak 0
 */
int compare(task_t *tr, coords_t *B) {
	assert(tr);
	assert(B);
	int i;

	// projdu vsechny figurky v referencnim reseni a zjistim jestli jsou
	// jejich pozice obsazeny i na porovnavane sachovnici obsazeny
	for(i = 0; i < t->k; i++) {
		if(!task_get_pos(tr, B, tr->B[i])) {
			srpdebug("core", node, "porovnani <neshoda>");
			return 0;
		}
	}

	srpdebug("core", node, "porovnani <shoda>");
	return 1;
}

/**
 * Expanze vrcholu zasobniku.
 * \returns     pocet stavu na ktere byl vrchol zasobniku expandovan
 */
int expand() {
	assert(t);
	assert(s);
	stack_item_t *it;
	coords_t c;         // souradnice zdrojoveho policka posledniho tahu
	move_t m;           // tah
	unsigned int i;
	int dir;
	stack_item_t its;   // novy stav (vznikly expanzi)

	srpdebug("core", node, "expanze <zacatek>");

	// vyndani uzlu ze zasobniku
	it = stack_pop(s);

	assert(it);
	assert(it->B);
	assert(it->h);

	cc++;

	// kontroly smysluplnosti expanze

	// vzhledem k napocitane penalizaci nemusi mit smysl dale pokracovat
	// protoze nemame zaporne penalizace. Takove vetve lze oriznout.
	//if(solution != NULL) {
	if(mpi_best_p > 0 && it->p >= mpi_best_p) {
		srpdebug("core", node, "expanze <orez, p=%d je horsi nez "
			"nejlepsi p=%d>",
		it->p, mpi_best_p);
		stack_item_destroy(it);
		return;
	}

	// maximalni hloubka stavoveho stromu
	// FIXME zeptat se p.Simecka jestli je tohle legalni
	if(it->d == t->q) {
		srpdebug("core", node, "expanze <dosazena max. hloubka stav. "
			"prostoru q=%d>",
			t->q);
		stack_item_destroy(it);
		return;
	}

	c = hist_lookup_move(it->h, FROM, 1 /* posledni tah */);

	// pridat na zasobnik vsechny mozne tahy z tohoto stavu
	for(i = 0; i < t->k; i++) {
		//assert(it->B[i]);

		// zkusit vsechny smery tahu
		for(dir = RUU /* 1 */; dir <= LLD /* 8 */; dir++) {
			if(task_move(t, it->B, i, dir, &m, NULL, 1) == 1) {
				// tah se zdari, vyloucime tah zpet
				if(m[TO].x == c.x && m[TO].y == c.y)
					continue;

				// ulozit tah jako novy stav na zasobnik
				its.d = it->d + 1;
				its.p = it->p;
				its.h = hist_init(it->h);
				its.B = task_bdcpy(t, it->B);

				task_move(t, its.B, i, dir, &m, &its.p, 0);
				hist_append_move(its.h, m);

				stack_push(s, its);
			}
		}
	}

	srpdebug("core", node, "expanze <konec>");
	stack_item_destroy(it);
	return 0;
}

/**
 * MPI krok resiciho algoritmu.
 */
void mpi_solve_step() {
	assert(t);
	assert(s);

	// neni co resit
	if(stack_empty(s))
		return;

	// puvodni sekvenci zpusob hledani reseni na generickem zasobniku
	srpdebug("core", node, "zpracovani stavu <d=%d, p=%d>", stack_top(s)->d,
		stack_top(s)->p);

	//porovnani
	if(compare(tf, stack_top(s)->B)) {
		// na zasobniku je reseni ulohy
		if(solution == NULL) {
			// prvni reseni ulohy
			solution = stack_pop(s);

			srpdebug("core", node, "reseni <prvni, p=%d>", solution->p);
		} else {
			if(solution->p >= stack_top(s)->p) {
				// lepsi reseni ulohy
				srpdebug("core", node, "reseni <lepsi, p=%d, o=%d>",
					stack_top(s)->p, solution->p - stack_top(s)->p);

				stack_item_destroy(solution);
				solution = stack_pop(s);
			} else {
				// horsi reseni ulohy
				srpdebug("core", node, "reseni <horsi, p=%d, o=+%d>",
					solution->p, stack_top(s)->p - solution->p);
			}
		}
	}

	// zasobnik mohl byt vyprazdnen, pokud ne, expanze
	if(!stack_empty(s))
		expand();
}

/**
 * MPI vymena zadani pomoci broadcastu. Uzly se nejdrive dozvi potrebnou
 * velikost bufferu a posleze obdrzi strukturu ukolu.
 */
int mpi_bcast_task() {
	int l;              // delka bufferu zpravy
	char *b = NULL;     // buffer zpravy
	unsigned int n,k,q;

	// serializovat ulohu
	if(node == 0) {
		assert(t);
		b = task_mpipack(t, &l, 0);
	}

	// distribuovat velikost b
	MPI_Bcast(&l, 1, MPI_INT, 0, MPI_COMM_WORLD);
	srpdebug("mpi", node, "broadcast velikost zpravy <l=%d>", l);

	// pripravit buffer na ostatnich uzlech
	if(node > 0)
		b = (char *)utils_malloc(l * sizeof(char));

	MPI_Bcast(b, l, MPI_PACKED, 0, MPI_COMM_WORLD);
	srpdebug("mpi", node, "broadcast zadani ulohy");

	// de-serializovat ulohu
	if(node > 0) {
		assert(b);
		t = task_mpiunpack(b, l, 0);

		assert(t);
		srpdebug("mpi", node, "prijato zadani ulohy <n=%d, k=%d, q=%d>",
			t->n, t->k, t->q);
	}
	free(b);

	return 1;
}

/**
 * Odesle n-tinu vlastniho zasobniku prijemci dest.
 */
void mpi_send_stack(const int n, int dest) {
	assert(t);
	assert(s);
	unsigned int l;
	char *b = NULL;

	stack_t *sn = stack_divide(s, n);
	b = stack_mpipack(sn, t, &l);

	srpdebug("mpi", node, "odeslani zasobniku <s=%d, l=%db, dest=%d>",
		sn->s, l, dest);

	// poslat MSG_STACK s delkou zpravy se serializovanym zasobnikem
	MPI_Send(&l, 1, MPI_INT, dest, MSG_STACK, MPI_COMM_WORLD);

	// poslat serializovany zasobnik
	MPI_Send(b, l, MPI_PACKED, dest, MSG_STACK_DATA, MPI_COMM_WORLD);

	stack_destroy(sn);
	free(b);
}

/**
 * MPI prijem zasobniku a spojeni s lokalnim zasobnikem.
 */
void mpi_recv_stack(const int src) {
	assert(t);
	assert(s);
	MPI_Status mpi_status;
	unsigned int l;
	char *b = NULL;

	MPI_Recv(&l, 1, MPI_UNSIGNED, src, MSG_STACK,
		MPI_COMM_WORLD, &mpi_status);

	srpdebug("mpi", node, "prijem zasobniku <l=%db, src=%d>",
		l, mpi_status.MPI_SOURCE);

	// naalokuju buffer a zahajim blokujici cekani na MSG_STACK_DATA
	srpdebug("mpi", node, "cekani na zpravu <delka=%db>", l);
	MPI_Probe(src, MSG_STACK_DATA, MPI_COMM_WORLD,
		&mpi_status);
	b = (char *)utils_malloc(l * sizeof(char));
	MPI_Recv(b, l, MPI_PACKED, src, MSG_STACK_DATA,
		MPI_COMM_WORLD, &mpi_status);

	stack_t *sn = stack_mpiunpack(b, t, l);
	free(b);

	srpdebug("mpi", node, "prijat zasobnik <s=%d>", sn->s);

	// sloucit zasobniky
	stack_merge(s, sn);

	// nastavit pocet pokusu o ziskani prace zpet na maximum
	mpi_msg_try = 4 * node_count;
}

/**
 * MPI odeslani negativni odpoved.
 */
void mpi_send_nostack(const int dest) {
	assert(s);
	assert(t);

	srpdebug("mpi", node, "odeslani negativni odpovedi <dest=%d>",
		dest);
	MPI_Send(0, 1, MPI_UNSIGNED, dest,
		MSG_NOSTACK, MPI_COMM_WORLD);
}

/**
 * MPI obesilani uzlu lokalne dosazenou hodnotou p (optimalizace orezavani).
 */
void mpi_send_solution() {
	assert(s);
	assert(t);

	if(!solution)
}

/**
 * MPI inicializace vypoctu - rozeslani uvodnich ukolu 0-tym uzlem. Je mozne,
 * ze 0-ty uzel ulohu vyresi drive nez rozexpanduje zasobnik. Takova situace by
 * nemela vadit.
 */
void mpi_solve_init() {
	assert(t);
	assert(s);
	int i;
	stack_item_t it;

	// pocatecni stav (zadani ulohy)
	it.d = 0;
	it.p = 0;
	it.h = hist_init(NULL);
	it.B = task_bdcpy(t, NULL);
	stack_push(s, it);

	// rozresim ulohu tak, aby bylo co delit
	while(s->s < 10 * node_count) {
		srpdebug("mpi", node, "zasobnik je MELKY <s=%d uzlu=%d>",
			s->s, node_count);

		mpi_solve_step();
		if(stack_empty(s))
			break;
	}

	// rozeslat zpravy
	if(!stack_empty(s)) {
		srpdebug("mpi", node, "zasobnik je OK, "
			"distribuce <s=%d uzlu=%d>", s->s, node_count);
		for(i = 1; i < node_count; i++) {
			mpi_send_stack(node_count - i + 1, i);
		}
	}
}


/**
 * Kontrola a zpracovani zprav pomoci MPI.
 */
mpi_tag mpi_handle() {
	MPI_Status mpi_status;
	int mpi_flag;
	unsigned int l;
	unsigned int v = 1;
	char *b = NULL;

	MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &mpi_flag,
		&mpi_status);
	if(mpi_flag) {
		srpdebug("mpi", node, "zprava <tag=%d>", mpi_status.MPI_TAG);

		switch(mpi_status.MPI_TAG) {
		case MSG_REQUEST:
			srpdebug("mpi", node, "MSG_REQUEST");
			MPI_Recv(&v, 1, MPI_UNSIGNED, mpi_status.MPI_SOURCE, MSG_REQUEST,
				MPI_COMM_WORLD, &mpi_status);

			// TODO rozdelit praci a odpovedet MSG_STACK nebo MSG_NOSTACK
			if(s->s > 1) {
				mpi_send_stack(2, mpi_status.MPI_SOURCE);
			} else {
				mpi_send_nostack(mpi_status.MPI_SOURCE);
			}
			break;

		case MSG_STACK:
			srpdebug("mpi", node, "MSG_STACK");
			mpi_recv_stack(mpi_status.MPI_SOURCE);
			break;

		case MSG_STACK_DATA:
			srpdebug("mpi", node, "MSG_STACK");
			// toto je chyba, ukoncit proces
			break;

		case MSG_NOSTACK:
			srpdebug("mpi", node, "MSG_NOSTACK");
			if(mpi_msg_try > 0)
				// snizim pocet volnych pokusu
				mpi_msg_try--;
			else {
				// odeslu peska
			}
			break;

		case MSG_TOKEN:
			srpdebug("mpi", node, "MSG_TOKEN");
			// pesek, pokud idlim necham ho byt, jinak...
			break;

		default:
			// neznama zprava
			srpprintf(stderr, "mpi", node, "neznama zprava!");
			exit(EXIT_FAILURE);
		}
	} else {
		srpdebug("mpi", node, "zadne zpravy");
	}

	return MSG_UNKNOWN;
}

/**
 * Resici algoritmus.
 */
void mpi_solve() {
	assert(s);
	assert(t);
	int msg_c = 149;

	while(!work_done) {
		msg_c++;
		if(msg_c == mpi_msg_max) {
			msg_c = 0;
			mpi_handle();
		}

		if(!stack_empty(s)) {
			mpi_solve_step();
		} else {
			work_done = 1;
			/*
			while(mpi_msg_try > 0) {
				// posla
			}
			// poslu peska
			*/
		}
	}

	/*
	srpdebug("core", node, "hledani reseni");

	while(!stack_empty(s)) {
		srpdebug("core", node, "hloubka: %d, prohledane stavy: %d",
			stack_top(s)->d, cc);

		if(compare(tf, stack_top(s)->B)) {
			// na zasobniku je reseni ulohy
			srpdebug("core", node, "nalezeno reseni p=%d", stack_top(s)->p);

			if(solution == NULL) {
				solution = stack_pop(s);
				srpdebug("core", node, "reseni je: <prvni>");
			} else {
				if(solution->p > stack_top(s)->p) {
					stack_item_destroy(solution);
					solution = stack_pop(s);
					srpdebug("core", node, "reseni je: <prub.nejlepsi>");
				} else {
					srpdebug("core", node, "reseni je: <horsi o=%d>",
						stack_top(s)->p - solution->p);
				}
			}
			//dump_hist(stdout, stack_top(s)->h);

		}
		if(stack_empty(s))
			break;
		expand();
	}
	*/
}

/**
 * Vstupni bod programu.
 */
int main(int argc, char **argv) {

	// inicializace MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &node);
	MPI_Comm_size(MPI_COMM_WORLD, &node_count);

	srpdebug("mpi", node, "inicializace <uzel=%d/%d>", node, node_count);

	// nacteni ulohy a distribuce provadi uzel 0
	if(node == 0) {
		parse_args(argc, argv);

		// nacteni ulohy
		if(!read_file(filename)) {
			srpfprintf(stderr, node, "chyba: nelze nacist zadani ulohy SRP `%s''",
				filename);
			exit(EXIT_FAILURE);
		}

		if(filename)
			free(filename);
	}

	// distribuovat zadani
	mpi_bcast_task();

	// pripravit zasobnik
	s = stack_init();

	// referencni reseni pro porovnavani
	tf = task_init(t->n, t->k, t->q);
	task_setup(tf);

	// inicializace uzlu pocatecnim zadanim
	mpi_msg_try = 4 * node_count;
	cc = 0;
	if(node == 0)
		mpi_solve_init();

	MPI_Barrier(MPI_COMM_WORLD);

	if(node != 0) {
		mpi_handle();
	}

	MPI_Barrier(MPI_COMM_WORLD);

	/*
	if(node == 1) {
		int i;
		for(i = 0; i < s->s; i++) {
			srpdebug("mpi", node, "s->it[%d] d=%d p=%d", i, s->it[i].d, s->it[i].p);
		}
	}
	*/

	while(!stack_empty(s)) {
		mpi_solve_step();
	}

	// spustit resici algoritmus na vsech uzlech
	//mpi_solve();

	/*
	// uzel 0 zahaji vypocet (jediny ma pocatecni stav sachovnice)
	if(node == 0) {
		assert(s);

		// pocatecni stav

		// DEBUG
		int i;
		for(i=0; i < 3; i++) {
			srpdebug("mpi", node, "%d. expanze", i+1);
			expand();
		}
		// EOF: DEBUG

		b = stack_mpipack(s, t, &l);
		MPI_Send(&l, 1, MPI_UNSIGNED, 1, 1, MPI_COMM_WORLD);
		MPI_Send(b, l, MPI_PACKED, 1, 2, MPI_COMM_WORLD);
	}

	if(node == 1) {
		MPI_Recv(&l, 1, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD, &stat);
		srpdebug("mpi", node, "prijata velikost zpravy: %d", l);

		b = (char *)utils_malloc(l * sizeof(char));
		MPI_Recv(b, l, MPI_PACKED, 0, 2, MPI_COMM_WORLD, &stat);
		free(s);
		s = stack_mpiunpack(b, t, l);

		int i;
		for(i = 0; i < s->s; i++) {
			srpdebug("mpi", node, "s[%d] d=%d p=%d", i, s->it[i].d, s->it[i].p);
		}

		stack_t *sn;
		sn = stack_divide(s, 2);

		srpdebug("mpi", node, "Puvodni stack:");
		for(i = 0; i < s->s; i++) {
			srpdebug("mpi", node, "s[%d] d=%d p=%d", i, s->it[i].d, s->it[i].p);
		}

		srpdebug("mpi", node, "Novy stack:");
		for(i = 0; i < sn->s; i++) {
			srpdebug("mpi", node, "s[%d] d=%d p=%d", i, sn->it[i].d, sn->it[i].p);
		}

		stack_destroy(sn);
		*/

	// vypsat statistiky
//	srpprintf(node, "-----");
//	srpprintf(node, "uloha:");
//	dump_task(stdout, t);

	// FIXME node 0!
	// vypis reseni
	srpprintf(node, "prohledano stavu: %d", cc);

	if(!solution) {
		srpprintf(node, "reseni nebylo nalezeno!");
	} else {
		srpprintf(node, "reseni nalezeno p=%d", solution->p);
		dump_hist(stdout, solution->h);
	}

	// uklid
	stack_item_destroy(solution);
	stack_destroy(s);
	task_destroy(t);
	task_destroy(tf);

	// finalizace MPI
	srpdebug("mpi", node, "finalizace <uzel=%d/%d>", node, node_count);
	MPI_Finalize();
	return EXIT_SUCCESS;
}
