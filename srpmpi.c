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
#include <time.h>
#include <unistd.h>
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
stack_item_t *solution;             // nejlepsi nalezene reseni
unsigned int cc;                    // pocitadlo analyzovanych stavu
unsigned int co;                    // pocitadlo orezanych stavu
double tbeg;                        // pocatecni cas
double tend;                        // konecny cas

/**
 * Zpravy MPI
 */
typedef enum {
	MSG_REQUEST = 1,                // zadost o praci
	MSG_STACK,                      // odpoved velikost prace
	MSG_STACK_DATA,                 // odpoved zasobnik
	MSG_NOSTACK,                    // neg. odpoved
	MSG_PENALTY,                    // penalizace
	MSG_TOKEN,                      // token
	MSG_FINALIZE,                   // ukonceni vypoctu
	MSG_UNKNOWN
} mpi_tag_e;

/**
 * Stavy MPI
 */
typedef enum {
	STATE_BUSY = 0,                 // pracuje
	STATE_IDLE,                     // ceka
	STATE_TOKEN                     // ocekava pesek
} mpi_state_e;

/**
 * Polozky pesku.
 */
typedef enum {
	TOKEN_COLOR = 0,
	TOKEN_PENALTY = 1,
	TOKEN_SOLVER = 2
} mpi_token_e;

/**
 * Barvy MPI
 */
typedef enum {
	COLOR_WHITE = 0,                // bila
	COLOR_BLACK = 1                 // cerna
} mpi_color_e;

// konstanty
const int MPI_MSG_MAX = 150;
const int MPI_IDLE_MAX = 1000;

mpi_state_e mpi_idle = 1;           // stav procesu
unsigned int mpi_best_p = -1;       // nejlepsi znama penalizace (orez)
unsigned int mpi_best_p_flag = 0;   // odesilat nejlepsi znamou penalizaci
                                    // pri dalsim mpi_handle() ?
int mpi_token_flag = 0;             // uzel > 0 pouziva pole pro indikaci ze
                                    // ma pesek
int mpi_token[3];

mpi_color_e mpi_color = COLOR_WHITE;// barva procesoru
int node = NONE;                    // mpi node (rank)
int node_count = NONE;              // pocet vypocetnich uzlu
int node_solver = 0;                // 1 pokud je proces viteznym resitelem

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

void finalize() {
	// vypsat statistiky

	if(node == 0) {
		srpprintf(node, "-----");
		srpprintf(node, "uloha:");
		dump_task(stdout, t);
	}

	// sesynchronizovat pozici v programu
	// rezie uklidu a vypisu uz me nezajima
	MPI_Barrier(MPI_COMM_WORLD);
	tend = MPI_Wtime();

	if(node_solver) {
		// vypis reseni
		srpprintf(node, "prohledano stavu: %d (orezano: %d)", cc, co);

		if(!solution) {
			srpprintf(node, "reseni nebylo nalezeno!");
		} else {
			srpprintf(node, "reseni nalezeno p=%d", solution->p);
			dump_hist(stdout, solution->h);
		}

		srpprintf(node, "spotrebovany cas: %f", tend-tbeg);

		// uklid
		stack_item_destroy(solution);
		stack_destroy(s);
		task_destroy(t);
		task_destroy(tf);
	}

	// finalizace MPI
	srpdebug("mpi", node, "finalizace <uzel=%d/%d>", node, node_count);
	MPI_Finalize();

	// konec
	exit(EXIT_SUCCESS);
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
			"nejlepsi p=%d>", it->p, mpi_best_p);
		stack_item_destroy(it);

		co++;
		return;
	}

	// maximalni hloubka stavoveho stromu
	// FIXME zeptat se p.Simecka jestli je tohle legalni
	if(it->d == t->q) {
		srpdebug("core", node, "expanze <dosazena max. hloubka stav. "
			"prostoru q=%d>",
			t->q);
		stack_item_destroy(it);

		co++;
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
		/*
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
		*/
		// mpi_best_p muzu bud ziskat sam, nebo ho dostat od jineho stroje
		// FIXME tohle lze vylepsit lepsi integraci mpi_best_p
		// nejdriv si ulozim lokalni nejlepsi reseni
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
		// ted poresim mpi_best_p
		if(mpi_best_p < 0 || mpi_best_p > solution->p) {
			mpi_best_p = solution->p;
			mpi_best_p_flag = 1;
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
		srpdebug("mpi", node, "prijeti zadani ulohy <n=%d, k=%d, q=%d>",
			t->n, t->k, t->q);
	}
	free(b);

	return 1;
}

/**
 * MPI odeslani pozadavku na dalsi praci.
 */
void mpi_send_request() {
	assert(s);
	assert(t);
	int dest;       // cilovy uzel
	int v = 1;

	assert(stack_empty(s));

	// vygenerovat nahodneho prijemce (krome me sameho)
	dest = node;
	while(dest == node)
		dest = rand() % node_count;

	srpdebug("mpi", node, "odeslani zadost o praci <dest=%d>", dest);
	MPI_Send(&v, 1, MPI_UNSIGNED, dest, MSG_REQUEST, MPI_COMM_WORLD);
}

/**
 * MPI odeslani n-tiny vlastniho zasobniku prijemci dest.
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

	// ADUV
	if(dest < node)
		mpi_color = 1; // cerna

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

	srpdebug("mpi", node, "prijeti zasobnik <l=%db, src=%d>",
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

	srpdebug("mpi", node, "prijeti zasobnik <s=%d>", sn->s);

	// sloucit zasobniky
	stack_merge(s, sn);
}

/**
 * MPI odeslani negativni odpovedi.
 */
void mpi_send_nostack(const int dest) {
	assert(s);
	assert(t);
	int v = 1;

	srpdebug("mpi", node, "odeslani negativni odpovedi <dest=%d>", dest);
	MPI_Send(&v, 1, MPI_UNSIGNED, dest, MSG_NOSTACK, MPI_COMM_WORLD);
}

/**
 * MPI prijeti negativni odpovedi.
 */
void mpi_recv_nostack(const int src) {
	MPI_Status mpi_status;
	unsigned int v;

	MPI_Recv(&v, 1, MPI_UNSIGNED, src, MSG_NOSTACK,
		MPI_COMM_WORLD, &mpi_status);

	srpdebug("mpi", node, "prijeti negativni odpovedi <src=%d>",
		mpi_status.MPI_SOURCE);
}

/**
 * MPI obesilani uzlu lokalne dosazenou hodnotou p (optimalizace orezavani).
 */
void mpi_send_penalty() {
	assert(s);
	assert(t);
	int i;

	if(mpi_best_p >= 0 && mpi_best_p_flag == 1) {
		srpdebug("mpi", node, "odeslani p <p=%d>",
			mpi_best_p);
		mpi_best_p_flag = 0;

		// odeslat standardni zpravu vsem krome me
		for(i = 0; i < node_count; i++) {
			if(i != node)
				MPI_Send(&mpi_best_p, 1, MPI_UNSIGNED, i,
					MSG_PENALTY, MPI_COMM_WORLD);
		}
	}
}

/**
 * MPI prijem lepsi hodnoty penalizace (optimalizace orezavani).
 */
void mpi_recv_penalty(const int src) {
	assert(t);
	assert(s);
	MPI_Status mpi_status;
	unsigned int p;

	MPI_Recv(&p, 1, MPI_UNSIGNED, src, MSG_PENALTY,
		MPI_COMM_WORLD, &mpi_status);

	if(p < mpi_best_p) {
		srpdebug("mpi", node, "prijeti p <p=%d, src=%d>",
			p, mpi_status.MPI_SOURCE);
		mpi_best_p = p;
		// nechci posilat pokud jsem chtel
		mpi_best_p_flag = 0;
	}
}

/**
 * MPI odeslani informace o ukonceni vypoctu
 */
void mpi_send_finalize(int solver) {
	assert(s);
	assert(t);
	int i;

	// odeslat standardni zpravu vsem krome me
	for(i = 0; i < node_count; i++) {
		if(i != node) {
			srpdebug("mpi", node, "odeslani konec vypoctu "
				"<src=%d, dest=%d, solver=%d>",
				node, i, solver);
			MPI_Send(&solver, 1, MPI_INT, i, MSG_FINALIZE,
				MPI_COMM_WORLD);
		}
	}
}

/**
 * MPI prijeti informace o ukonceni vypoctu odpovedi.
 */
void mpi_recv_finalize(const int src) {
	MPI_Status mpi_status;
	int solver;

	MPI_Recv(&solver, 1, MPI_INT, src, MSG_FINALIZE,
		MPI_COMM_WORLD, &mpi_status);

	srpdebug("mpi", node, "prijeti konec vypoctu <src=%d, solver=%d>",
		mpi_status.MPI_SOURCE, solver);

	if(solver == node) {
		srpdebug("mpi", node, "uzel=%d je nejlepsim resitelem ulohy",
			node);
		// jsem vitezny resitel ulohy
		node_solver = 1;
	}

}

/**
 * MPI odeslani pesku, signalizace ukonceni vypoctu.
 */
void mpi_send_token(const mpi_color_e color, const int solver,
	const unsigned int p) {
	int token[3];

	srpdebug("mpi", node, "odeslani pesek <src=%d, dest=%d, solver=%d, "
		"p=%d, color=%d>",
		node, (node + 1) % node_count, solver, p, color);

	// nastavime token
	token[TOKEN_COLOR] = color;
	token[TOKEN_SOLVER] = solver;
	token[TOKEN_PENALTY] = p;

	MPI_Send(&token, 3, MPI_INT, (node + 1) % node_count, MSG_TOKEN,
		MPI_COMM_WORLD);
}

/**
 * MPI prijem pesku.
 */
void mpi_recv_token(const int src) {
	assert(s);
	MPI_Status mpi_status;
	int token[3];

	MPI_Recv(&token, 3, MPI_INT, src, MSG_TOKEN,
		MPI_COMM_WORLD, &mpi_status);
	srpdebug("mpi", node, "prijeti pesek <src=%d, solver=%d, p=%d color=%d>",
		src, token[TOKEN_SOLVER], token[TOKEN_PENALTY], token[TOKEN_COLOR]);

	if(node == 0) {
		// pesek dorazil zpet k uzlu 0
		if(token[TOKEN_COLOR] == COLOR_WHITE) {
			// je bily
			// poslu zpravu o ukonceni vypoctu vsem, tedy i vitezi
			mpi_send_finalize(token[TOKEN_SOLVER] != -1 ?
				token[TOKEN_SOLVER] : 0);
			// protoze zpravu neposilam sobe, musim si v pripade, ze jsem
			// vyhral, nebo nenasel reseni pomoci nastavenim node_solver
			if(token[TOKEN_SOLVER] == -1 || token[TOKEN_SOLVER] == 0)
				node_solver = 1;

			finalize();
		} else {
			// je cerny (dalsi kolecko)
			mpi_idle = STATE_IDLE;
		}
	} else {
		// pripravit se na odesilani pesku
		// nelze to udelat primo, protoze nemam jistotu ze jsem STATE_IDLE
		mpi_token_flag = 1;
		mpi_token[TOKEN_COLOR] = token[TOKEN_COLOR];
		mpi_token[TOKEN_SOLVER] = token[TOKEN_SOLVER];
		mpi_token[TOKEN_PENALTY] = token[TOKEN_PENALTY];
	}
}

/**
 * MPI odeslani reseni. Reseni odesleme jen tehdy, mame-li lepsi p nez je
 * mpi_best_p.
 */
/*
void mpi_send_solution(const int dest) {
	assert(t);
	unsigned int v = 1;
	unsigned int l;
	char *b = NULL;

	if(solution && solution->p <= mpi_best_p) {
		b = stack_item_mpipack(&solution, t, &l);

		srpdebug("mpi", node, "odeslani reseni <p=%d, l=%db>",
			solution->p, l);

		// odeslat delku bufferu
		MPI_Send(&l, 1, MPI_UNSIGNED, 0, MSG_SOLUTION, MPI_COMM_WORLD);

		// poslat serializovane reseni
		MPI_Send(b, l, MPI_PACKED, 0, MSG_SOLUTION_DATA, MPI_COMM_WORLD);
	}

	srpdebug("mpi", node, "odeslani zadosti o vypis reseni <dest=%d>",
		dest);

	// odeslat delku bufferu
	MPI_Send(&v, 1, MPI_UNSIGNED, dest, MSG_SOLUTION, MPI_COMM_WORLD);
}
*/

/**
 * MPI prijeti zadosti o vypis reseni.
 */
/*
void mpi_recv_solution(const int src) {
	assert(t);
	assert(node = 0);
	MPI_Status mpi_status;
	unsigned int l;
	char *b = NULL;

	MPI_Recv(&l, 1, MPI_UNSIGNED, src, MSG_SOLUTION,
		MPI_COMM_WORLD, &mpi_status);

	srpdebug("mpi", node, "prijeti reseni <l=%db, src=%d>",
		l, mpi_status.MPI_SOURCE);

	// naalokuju buffer a zahajim blokujici cekani na MSG_SOLUTION_DATA
	srpdebug("mpi", node, "cekani na zpravu <delka=%db>", l);
	MPI_Probe(src, MSG_SOLUTION_DATA, MPI_COMM_WORLD,
		&mpi_status);
	b = (char *)utils_malloc(l * sizeof(char));
	MPI_Recv(b, l, MPI_PACKED, src, MSG_SOLUTION_DATA,
		MPI_COMM_WORLD, &mpi_status);

	stack_item *it = stack_item_mpiunpack(b, t, l);
	free(b);

	srpdebug("mpi", node, "prijeti reseni <p=%d>", it->s);

	// porovnani a prirazeni do lokalniho reseni
	if(!solution) {
		// prvni reseni
		solution = it;
	} else if(solution->p > it->p) {
		// lepsi reseni
		stack_item_destroy(solution);
		solution = it;
	}
}
*/

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
	while(s->s_real < node_count) {
		srpdebug("mpi", node, "zasobnik je MELKY <s=%d uzlu=%d>",
			s->s_real, node_count);

		mpi_solve_step();
		if(stack_empty(s))
			break;
	}

	// rozeslat zpravy
	if(!stack_empty(s)) {
		srpdebug("mpi", node, "zasobnik je OK, "
			"distribuce <s=%d uzlu=%d>", s->s_real, node_count);
		for(i = 1; i < node_count; i++) {
			mpi_send_stack(node_count - i + 1, i);
		}
	}
}

/**
 * Kontrola a zpracovani zprav pomoci MPI.
 */
void mpi_handle() {
	MPI_Status mpi_status;
	int mpi_flag;
	unsigned int l;
	unsigned int v = 1;
	char *b = NULL;

	MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &mpi_flag,
		&mpi_status);

	// odeslu nejnizsi nalezenou penalizaci (podmineno viz. telo funkce)
	mpi_send_penalty();

	if(mpi_flag) {
		srpdebug("msg", node, "zprava <tag=%d>", mpi_status.MPI_TAG);

		switch(mpi_status.MPI_TAG) {
		case MSG_REQUEST:
			srpdebug("msg", node, "MSG_REQUEST");
			MPI_Recv(&v, 1, MPI_UNSIGNED, mpi_status.MPI_SOURCE, MSG_REQUEST,
				MPI_COMM_WORLD, &mpi_status);

			srpdebug("mpi", node, "prijata zadost o praci <src=%d>",
				mpi_status.MPI_SOURCE);

			// TODO rozdelit praci a odpovedet MSG_STACK nebo MSG_NOSTACK
			if(s->s_real > 1) {
				mpi_send_stack(2, mpi_status.MPI_SOURCE);
			} else {
				mpi_send_nostack(mpi_status.MPI_SOURCE);
			}
			break;

		case MSG_STACK:
			srpdebug("msg", node, "MSG_STACK");
			mpi_recv_stack(mpi_status.MPI_SOURCE);
			break;

		case MSG_NOSTACK:
			srpdebug("msg", node, "MSG_NOSTACK");
			mpi_recv_nostack(mpi_status.MPI_SOURCE);
			break;

		case MSG_PENALTY:
			srpdebug("msg", node, "MSG_PENALTY");
			mpi_recv_penalty(mpi_status.MPI_SOURCE);
			break;

		case MSG_TOKEN:
			srpdebug("msg", node, "MSG_TOKEN");
			mpi_recv_token(mpi_status.MPI_SOURCE);
			break;

		case MSG_FINALIZE:
			srpdebug("msg", node, "MSG_FINALIZE");
			mpi_recv_finalize(mpi_status.MPI_SOURCE);
			finalize();
			break;

		default:
			// neznama zprava
			srpprintf(stderr, "msg", node, "neznama zprava");
			exit(EXIT_FAILURE);
		}
	} /* else {
		srpdebug("mpi", node, "zadne zpravy");
	} */
}

/**
 * Resici algoritmus.
 */
void mpi_solve() {
	assert(s);
	assert(t);
	int msg_c = 0;
	int idle_c = 0;

	while(1) {
		// zasobnik neni prazdny
		while(!stack_empty(s)) {
			mpi_idle = STATE_BUSY;
			//srpdebug("mpi", node, "uzel=%d PRACUJE", node);

			// provadet reseni
			mpi_solve_step();

			// osetrit pozadavky
			msg_c++;
			if(msg_c == MPI_MSG_MAX) {
				msg_c = 0;
				mpi_handle();
			}
		}

		// zasobnik je prazdny
		if(mpi_idle == STATE_BUSY)
			mpi_idle = STATE_IDLE;
		//srpdebug("mpi", node, "uzel=%d CEKA", node);

		// ADUV
		if(node == 0 && mpi_idle == STATE_IDLE) {
			// odeslani pesku a zahajeni cekani
			mpi_color = COLOR_WHITE;
			mpi_send_token(COLOR_WHITE, 
				mpi_token[TOKEN_SOLVER],
				mpi_token[TOKEN_PENALTY]);
			mpi_idle = STATE_TOKEN;
		}

		// ADUV
		if(node > 0 && mpi_token_flag == 1) {
			// pokud mam lepsi reseni (NE mpi_best_p pro OREZ!), nahradim je v
			// pesku vlastnimi hodnotami
			if(solution)
				if(mpi_token[TOKEN_PENALTY] > solution->p
					|| mpi_token[TOKEN_PENALTY] == -1) {
					mpi_token[TOKEN_SOLVER] = node;
					mpi_token[TOKEN_PENALTY] = solution->p;
				}

			// odeslat modifikovany pesek
			mpi_send_token(mpi_token[TOKEN_COLOR],
				mpi_token[TOKEN_SOLVER],
				mpi_token[TOKEN_PENALTY]);

			mpi_color = COLOR_WHITE;
			mpi_token_flag = 0;
		}

		if(mpi_idle == STATE_IDLE) {
			// odeslani pozadavku na dalsi praci
			idle_c++;
			if(idle_c == MPI_IDLE_MAX) {
				idle_c = 0;
				mpi_send_request();
			}
		}

		mpi_handle();
	}
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

	// vsichni si poznamenaji cas spusteni tbeg
	MPI_Barrier(MPI_COMM_WORLD);
	tbeg = MPI_Wtime();

	// inicializace random seminka
	srand(time(NULL) + getpid() + node);

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

	// inicializace uzlu pocatecni praci
	cc = 0; co = 0;

	// inicializace pesku
	mpi_token[TOKEN_SOLVER] = -1;
	mpi_token[TOKEN_PENALTY] = -1;

	if(node == 0) {
		mpi_solve_init();
	}

	// zaruci ze vsichni dostanou praci
	MPI_Barrier(MPI_COMM_WORLD);
	if(node > 0)
		mpi_handle();

	// resit ulohu
	mpi_solve();

	// v mpi_solve je nekonecna smycka, konec se resi volanim funkce
	// finalize();
	// this is another brick in the -Wall
	return EXIT_FAILURE;
}
