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
#include "srphist.h"
#include "srputils.h"

extern int node;    /* srpnompi.c/srpmpi.c */

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
 * Struktura ulohy.
 */
typedef struct {
	unsigned int n;     // strana sachovnice
	unsigned int k;     // pocet figurek
	unsigned int q;     // maximalni pocet tahu k reseni
	coords_t *B;        // pole souradnic figurek
	unsigned int *P;    // pole penalizaci
} task_t;

extern task_t *         task_init(const unsigned int n,
	const unsigned int k, const unsigned int q);
extern void             task_destroy(task_t *t);
extern unsigned int     task_clean(task_t *t);
extern void             task_setup(task_t *t);
extern int              task_get_pos(task_t *t, coords_t *B,const coords_t c);
extern int              task_set_pos(task_t *t, coords_t *B,
	const unsigned int i, const coords_t c);
extern int              task_move(task_t *t, coords_t *B, 
	const unsigned int i, const dir_t d, move_t *m, unsigned int *p,
	int dry_run);
extern inline coords_t * task_bdcpy(const task_t *t, coords_t *B);
extern size_t           task_sizeof(const task_t *t, const int include_board);

#ifdef MPI
extern char *           task_mpipack(const task_t *t, int *l,
	const int include_board);
extern task_t *         task_mpiunpack(const char *b, const int l,
	const int include_board);
#endif /* MPI */

#endif /* __SRPTASK_H */
