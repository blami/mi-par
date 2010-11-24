/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srpstack: funkce a struktury zasobniku
 */
#ifndef __SRPSTACK_H
#define __SRPSTACK_H
#include "srphist.h"
#include "srptask.h"
#include "srputils.h"

extern int node;    /* srpnompi.c/srpmpi.c */

// konstanty
#define STACK_N 1000            // velikost zasobniku do dalsi realokace

/**
 * Struktura predstavujici jeden zaznam na zasobniku.
 */
typedef struct {
	unsigned int d;             // hloubka ve stromu reseni
	unsigned int p;             // celkova penalizace
	hist_t *h;                  // historie tahu
	coords_t *B;                // stav sachovnice
	int dirty;                  // spinavy zaznam, odstranit pri defragmentaci pokud 1
} stack_item_t;

/**
 * Struktura zasobniku.
 */
typedef struct {
	stack_item_t *it;           // pole zaznamu
	unsigned int s;             // velikost pouziteho zasobniku (VCETNE FRAGMENTACE)
	unsigned int s_real;        // pocet skutecnych polozek (BEZ FRAGMENTACE)
	unsigned int st;            // celkova velikost zasobniku
} stack_t;

extern stack_item_t *   stack_item_init(const unsigned int d,
	const unsigned int p, hist_t *h, coords_t *B);
extern void             stack_item_destroy(stack_item_t *it);
extern stack_t *        stack_init();
extern void             stack_destroy(stack_t *s);
extern int              stack_empty(stack_t *s);
extern int              stack_push(stack_t *s, stack_item_t it);
extern stack_item_t *   stack_top(stack_t *s); // vraci ukazatel na zasobnik
extern stack_item_t *   stack_pop(stack_t *s); // vraci ukazatel na novou instanci (zkopiruje)
extern inline size_t    stack_sizeof(const stack_t *s, const task_t *t);
extern stack_t *        stack_divide(stack_t *ss, const int n);
extern stack_t *        stack_merge(stack_t *s, stack_t *sa);

#ifdef MPI
extern char *           stack_mpipack(const stack_t *s, const task_t *t, int *l);
extern stack_t *        stack_mpiunpack(const char *b, const task_t *t, int l);
#endif /* MPI */

#endif /* __SRPSTACK_H */
