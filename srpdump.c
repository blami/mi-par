/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srpdump: pomocne funkce pro vypis a nacteni struktury problemu
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "srputils.h"
#include "srptask.h"
#include "srpdump.h"


/**
 * Vypsat strukturu ulohy v lidsky citelne podobe.
 */
void dump_print_task(FILE *f, task_t *t) {
	assert(t);
	assert(f);
	coords_t c;
	int i;

	fprintf(f, "n=%d\n"
		"k=%d\n"
		"q=%d\n",
		t->n, t->k, t->q);

	dump_print_board(f, t);

	// vypsat penalizace
	fprintf(f, "P=\n");
	for(c.y = 0; c.y < t->n; c.y++) {
		for(c.x = 0; c.x < t->n; c.x++) {
			i = utils_map(c, t->n);
			fprintf(f, "%d", t->P[i]);

			if(c.x != t->n - 1)
				fprintf(f, ",");
		}
		fprintf(f, "\n");
	}
}

/**
 * Vypsat pouze stav sachovnice v lidsky citelne podobe.
 */
void dump_print_board(FILE *f, task_t *t) {
	assert(t);
	assert(f);
	coords_t c;

	fprintf(f, "B=\n");
	for(c.y = 0; c.y < t->n; c.y++) {
		for(c.x = 0; c.x < t->n; c.x++) {
			if(task_get_pos(t, c) == 0)
				fprintf(f, ".");
			else
				fprintf(f, "#");
		}
		fprintf(f, "\n");
	}

}
