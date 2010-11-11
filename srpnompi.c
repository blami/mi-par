/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srpcore: sekvencni (ne-OpenMPI) resitel ulohy
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
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
unsigned int co;            // pocitadlo orezanych stavu

int node = -1;

/**
 * Zpracovat prepinace vcetne overeni spravnosti.
 */
void parse_args(int argc, char **argv)
{
	int a;
	while((a = getopt(argc, argv, "vh")) != -1){
		switch(a) {
		case 'h':
			printf("srpcore: sekvencni (ne-OpenMPI) resitel ulohy SRP\n\n"
				"Pouziti: srpcore [prepinace] <soubor>\n"
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
	if(solution != NULL) 
		if(it->p >= solution->p) {
			srpdebug("core", node, "expanze <orez, p=%d je horsi nez "
				"nejlepsi p=%d>",
				it->p, solution->p);
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
 * Resici algoritmus.
 * a/ inicializace
 *      1. priprav referencni reseni (zde main())
 *      2. nastav pocitadlo stavu na 0 (cc)
 *      3. uloz na zasobnik pocatecni stav (task)
 * b/ beh
 *      1. porovnej stav na vrcholu zasobniku (c/)
 *      2. pokud neni zasobnik prazdny, expanduj stav (d/) na jeho vrcholu,
 *         jinak konec (nejlepsi reseni je v solution)
 *      3. opakuj krok b/-1.
 * c/ porovnani stavu na vrcholu zasobniku
 *      1. pokud je stav na vrcholu zasobniku reseni (porovnani s tf je OK)
 *          1-1. porovnej penalizaci s obsahem solution 
 *          1-2. pokud je penalizace mensi nahrad solution
 *          1-3. zahod reseni ze zasobniku (lepsi uz nemuze byt, expanze nema
 *          vyznam)
 *      2. pokud neni stav na zasobniku reseni skonci porovnani
 * d/ expanze stavu
 */
void solve() {
	assert(s);
	assert(t);
	cc = 0;
	co = 0;

	srpdebug("core", node, "hledani reseni");

	// pocatecni stav
	stack_item_t it;
	it.d = 0;
	it.p = 0;
	it.h = hist_init(NULL);
	it.B = task_bdcpy(t, NULL);

	stack_push(s, it);

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
}

/**
 * Vstupni bod programu.
 */
int main(int argc, char **argv) {
	parse_args(argc, argv);

	// nacteni ulohy
	if(!read_file(filename)) {
		srpfprintf(stderr, node, "chyba: nelze nacist zadani ulohy SRP `%s''",
			filename);
		exit(EXIT_FAILURE);
	}

	if(filename)
		free(filename);

	// pripravit zasobnik
	s = stack_init();

	// referencni reseni pro porovnavani
	tf = task_init(t->n, t->k, t->q);
	task_setup(tf);

	solve();

	// vypsat statistiky
	srpprintf(node, "-----");
	srpprintf(node, "uloha:");
	dump_task(stdout,t);

	srpprintf(node, "prohledano stavu: %d (orezano: %d)", cc, co);

	if(!solution) {
		srpprintf(node, "reseni nebylo nalezeno!");
	} else {
		srpprintf(node, "reseni nalezeno p=%d", solution->p);
		dump_hist(stdout, solution->h);
	}

	stack_item_destroy(solution);

	stack_destroy(s);
	task_destroy(t);
	task_destroy(tf);

	return EXIT_SUCCESS;
}
