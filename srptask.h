/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srptask: funkce a struktury pro praci se sachovnici.
 */
#ifndef __SRPTASK_H
#define __SRPTASK_H
#include "srputils.h"   /* coords_t */
#include "srphist.h"    /* move_t */


/**
 * Smery tahu jezdcem.
 */
typedef enum {
	NNN = 0,            // neplatny tah
	RUU = 1,            // pravo-nahoru-nahoru
	RRU = 2,            // ...
	RDD = 3,
	RRD = 4,
	LUU = 5,
	LLU = 6,
	LDD = 7,
	LLD = 8
} dir_t;

/**
 * Struktura problemu.
 */
typedef struct {
	unsigned int n;     // strana sachovnice
	unsigned int k;     // pocet figurek
	unsigned int q;     // maximalni pocet tahu k reseni
	coords_t *B;        // pole souradnic figurek
	unsigned int *P;    // pole penalizaci
} task_t;

task_t *        task_init(const unsigned int n, const unsigned int k,
	const unsigned int q);
void            task_destroy(task_t *t);
unsigned int    task_clean(task_t *t);
void            task_setup(task_t *t);
int             task_get_pos(task_t *t, coords_t *B, const coords_t c);
int             task_set_pos(task_t *t, coords_t *B, const unsigned int i,
	const coords_t c);
int             task_move(task_t *t, coords_t *B, const unsigned int i,
	const dir_t d, move_t *m, unsigned int *p, int dry_run);

#endif /* __SRPPROBLEM_H */
