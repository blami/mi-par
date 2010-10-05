/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srptask: funkce a struktury pro praci s ulohou.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "srputils.h"
#include "srptask.h"


/**
 * Inicializace ulohy.
 */
task_t * task_init(const unsigned int n, const unsigned int k,
	const unsigned int q) {
	task_t *t = NULL;
	int i;

	t = (task_t *)utils_malloc(sizeof(task_t));
	t->n = n;
	t->k = k;
	t->q = q;

	t->P = (unsigned int *)utils_malloc(sizeof(unsigned int) * (n*n));
	for(i = 0; i < n*n; i++)
		t->P[i] = 0;

	t->B = (coords_t *)utils_malloc(sizeof(coords_t) * k);
	// inicializovat vsechny figurky mimo sachovnici
	task_clean(t);

	return t;
}

/**
 * Finalizace ulohy.
 */
void task_destroy(task_t *t) {
	int i;

	if(!t) return;

	if(t->P)
		free(t->P);

	if(t->B)
		free(t->B);

	free(t);
}

/**
 * Odstraneni figurek ze sachovnice.
 * \returns     pocet figurek odstranenych ze sachovnice
 */
int task_clean(task_t *t) {
	assert(t);
	int c = 0;
	int i;

	for(i = 0; i < t->k; i++) {
		if(t->B[i].x > -1 && t->B[i].y > -1)
			c++;

		t->B[i].x = -1; t->B[i].y = -1;
	}

	return c;
}

/**
 * Umisteni figurek do horniho trojuhelniku sachovnice.
 */
int task_setup(task_t *t) {
	assert(t);
	unsigned int x = 0, y = 0; // horni levy roh
	int edge = t->n;
	int count = t->k;
	int i;

	task_clean(t);

	for(i = 0; i < count; i++) {
		t->B[i].x = x; t->B[i].y = y;
		x++;
		if(x >= edge) {
			x = 0;
			y++;
			edge--;
		}
	}
}

/**
 * Kontrola pozice X,Y na sachovnici.
 * \returns     1 pokud je pozice obsazena, 0 pokud ne
 */
int task_get_pos(task_t *t, const coords_t c) {
	assert(t);
	assert(c.x < t->n && c.y < t->n);
	int i;

	for(i = 0; i < t->k; i++) {
		if(t->B[i].x == c.x && t->B[i].y == c.y)
			return 1;
	}

	return 0;
}

/**
 * Nastaveni pozice X,Y na sachovnici. Kontroluje obsazenost ciloveho pole.
 * \returns     1 pokud doslo k presunu, 0 bylo-li pole obsazeno
 */
int task_set_pos(task_t *t, const int i, const coords_t c) {
	assert(t);
	assert(i < t->k);

	// tah mimo nemuze byt aserce, protoze tah mimo je validni pokus
	if(c.x < 0 || c.x >= t->n || c.y < 0 || c.y >= t->n)
		return 0;

	if(task_get_pos(t, c) == 0) {
		t->B[i].x = c.x; t->B[i].y = c.y;
		return 1;
	}

	return 0;
}

/**
 * Regulerni tah figurkou jezdce po sachovnici.
 * \returns     1 tah byl spravny (proveden), 0 tah byl nespravny (neproveden)
 */
// TODO sem zadratovat jeste ukladani do historie tahu pres argument + pocitani 
// penale - tj. asi v ramci historie tahu
int task_move(task_t *t, const int i, dir_t d)  {
	assert(t);
	assert(i < t->k);
	assert(t->B[i].x >= 0 && t->B[i].y >= 0); // kun musi byt umisten
	assert(0 < d < 8);

	coords_t c, c_old; // nove souradnice

	c_old.x = t->B[i].x;
	c_old.y = t->B[i].y;

	switch(d) {
	case RUU:
		c.x = t->B[i].x + 1; c.y = t->B[i].y - 2;
		break;
	case RRU:
		c.x = t->B[i].x + 2; c.y = t->B[i].y - 1;
		break;
	case RDD:
		c.x = t->B[i].x + 1; c.y = t->B[i].y + 2;
		break;
	case RRD:
		c.x = t->B[i].x + 2; c.y = t->B[i].y + 1;
		break;
	case LUU:
		c.x = t->B[i].x - 1; c.y = t->B[i].y - 2;
		break;
	case LLU:
		c.x = t->B[i].x - 2; c.y = t->B[i].y - 1;
		break;
	case LDD:
		c.x = t->B[i].x - 1; c.y = t->B[i].y + 2;
		break;
	case LLD:
		c.x = t->B[i].x - 2; c.y = t->B[i].y + 1;
		break;
	default:
		// neznamy smer tahu, prazdny tah
		return 0;
	}

	// zkusit tahnout
	if(task_set_pos(t, i, c)) {
		// TODO penalizace a historie
		/*
		printf("tah %d %d:%d -> %d:%d\n", i, c_old.x, c_old.y,
			c.x, c.y);
		dump_print_board(stdout, t);
		*/
		return 1;
	}

	return 0;
}
