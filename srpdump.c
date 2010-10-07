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
#include "srpdump.h"


/**
 * Vypsat strukturu ulohy ve formatu pro parsovani.
 */
void dump_serialize(FILE *f, task_t *t) {
	assert(t);
	assert(f);
	int i;

	// zakladni udaje ve tvaru N;
	fprintf(f, "%d;\n", t->n);
	fprintf(f, "%d;\n", t->k);
	fprintf(f, "%d;\n", t->q);

	// pozice figurek ve tvaru X1:Y1;X2:Y2;...Xn:Yn;
	for(i = 0; i < t->k; i++) {
		fprintf(f, "%d:%d;", t->B[i].x, t->B[i].y);
	}
	fprintf(f,"\n");

	// penalizace ve tvaru N1;N2;...Nn;
	for(i = 0; i < (t->n * t->n); i++) {
		fprintf(f, "%d;", t->P[i]);
	}
	fprintf(f,"\n");
}

/**
 * Nacist jedno N; cislo z aktualni pozice v souboru.
 */
int dump_read_int(FILE *f) {
	assert(f);
	int v;

	if(!fscanf(f, "%d;", &v)) {
		fprintf(stderr, "chyba: neplatna vstupni data (int)\n");
		exit(EXIT_FAILURE);
	}

	return v;
}

/**
 * Nacist X:Y; dvojici cisel z aktualni pozice v souboru.
 */
coords_t dump_read_coords(FILE *f) {
	assert(f);
	coords_t c;
	int st;

	if(!fscanf(f, "%d:%d;", &c.x, &c.y)) {
		fprintf(stderr, "chyba: neplatna vstupni data (coords_t)\n");
		exit(EXIT_FAILURE);
	}

	return c;
}

/**
 * Nacist serializovana data ulohy a na jejich zaklade
 * vytvorit instanci.
 */
task_t * dump_unserialize(FILE *f) {
	assert(f);
	int n;
	int k;
	int q;
	task_t *t;
	int i;

	n = dump_read_int(f);
	k = dump_read_int(f);
	q = dump_read_int(f);

	t = task_init(n, k, q);

	// nacist souradnice figurek
	for(i = 0; i < t->k; i++) {
		t->B[i] = dump_read_coords(f);
	}

	// nacist penalizace poli
	for(i = 0; i < (t->n*t->n); i++) {
		t->P[i] = dump_read_int(f);
	}

	return t;
}

/**
 * Vypsat strukturu ulohy v lidsky citelne podobe.
 */
void dump_task(FILE *f, task_t *t) {
	assert(t);
	assert(f);
	coords_t c;
	int i;

	// zakladni udaje
	fprintf(f, "n=%d\n"
		"k=%d\n"
		"q=%d\n",
		t->n, t->k, t->q);

	// vypsat souradnice figurek
	fprintf(f, "B=");
	for(i = 0; i < t->k; i++) {
		fprintf(f, "%d:%d", t->B[i].x, t->B[i].y);

		if(i != t->k - 1)
			fprintf(f, ",");
	}
	fprintf(f, "\n");

	// vypsat obraz sachovnice
	fprintf(f, "S=\n");
	dump_board(f, t, NULL);

	// vypsat penalizace v matici jako sachovnici
	fprintf(f, "P=\n");
	for(c.y = 0; c.y < t->n; c.y++) {
		for(c.x = 0; c.x < t->n; c.x++) {
			// spocitat index v t->P
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
void dump_board(FILE *f, task_t *t, coords_t *B) {
	assert(t);
	assert(f);
	coords_t c;

	for(c.y = 0; c.y < t->n; c.y++) {
		for(c.x = 0; c.x < t->n; c.x++) {
			if(task_get_pos(t, B, c) == 0)
				fprintf(f, ".");
			else
				fprintf(f, "#");
		}
		fprintf(f, "\n");
	}
}

/**
 * Vypsat historii tahu.
 */
void dump_hist(FILE *f, hist_t *h) {
	assert(h);
	assert(f);
	int i;

	fprintf(f, "Tahy: ");

	if(h->l == 0) {
		fprintf(f, "-nic-\n");
		return;
	}

	for(i = 0; i < h->l; i++) {
		fprintf(f, "%d:%d->%d:%d", h->h[i][FROM].x, h->h[i][FROM].y,
			h->h[i][TO].x, h->h[i][TO].y);
		if(i != h->l - 1)
			fprintf(f, ",");
	}
	fprintf(f, "\n");
}
