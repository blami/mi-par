/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srpstack: funkce a struktury zasobniku
 */
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "srpstack.h"
#include "srputils.h"
#include "srphist.h"


/**
 * Inicializace struktury zaznamu na zasobniku.
 */
stack_item_t * stack_item_init(const unsigned int d, const unsigned int p,
	hist_t *h, coords_t *B) {
	assert(h);
	assert(B);
	stack_item_t *it;

	it = (stack_item_t *)utils_malloc(sizeof(stack_item_t));

	it->d = d;
	it->p = p;
	it->h = h;
	it->B = B;

	return it;
}

/**
 * Destrukce struktury zasobniku.
 */
void stack_item_destroy(stack_item_t *it) {
	if(!it)
		return;

	if(it->h)
		hist_destroy(it->h);
	if(it->B)
		free(it->B);

	free(it);
}

/**
 * Inicializace struktury zasobniku.
 */
stack_t * stack_init() {
	stack_t *s;
	s = (stack_t *)utils_malloc(sizeof(stack_t));

	// inicializace mist pro zaznamy
	s->it = (stack_item_t *)utils_malloc(sizeof(stack_item_t)
		* STACK_N);
	s->it = memset(s->it, 0, sizeof(stack_item_t) * STACK_N);

	// nastaveni velikosti
	s->s = 0;
	s->st = STACK_N;

	/*
	printf("stack: init s=%d st=%d\n", s->s, s->st);
	*/
	return s;
}

/**
 * Destrukce struktury zasobniku.
 */
void stack_destroy(stack_t *s) {
	int i;

	if(!s)
		return;

	// nelze pouzit stack_item_destroy (alokovane pole).
	if(s->it) {
		for(i = 0; i < s->st; i++) {
			// FIXME duplicitni kod, sjednotit do stack_item_clear()
			if(s->it[i].h)
				hist_destroy(s->it[i].h);
			if(s->it[i].B)
				free(s->it[i].B);
		}
		free(s->it);
	}

	free(s);
}

/**
 * Zmena velikosti zasobniku.
 * \param s     velikost o kterou ma byt zvetsena stavajici velikost zasobniku
 */
void stack_resize(stack_t *s, size_t sn) {
	assert(s);

	s->it = (stack_item_t *)utils_realloc(s->it, sizeof(stack_item_t) * (s->st + sn));
	s->st += sn;
	/*
	printf("stack: resize s=%d st=%d\n", s->s, s->st);
	*/
}

/**
 * Kontrola plnosti zasobniku.
 */
int stack_empty(stack_t *s)
{
	assert(s);
	return s->s == 0 ? 1 : 0;
}


/**
 * Vlozeni na vrchol zasobniku.
 * \returns     nova velikost zasobniku
 */
int stack_push(stack_t *s, stack_item_t it)
{
	assert(s);

	// doalokovat pamet je-li treba
	if(s->s == s->st - 1) {
		stack_resize(s, STACK_N);
	}

	s->s++;

	s->it[s->s - 1].d = it.d;
	s->it[s->s - 1].p = it.p;
	s->it[s->s - 1].h = it.h;
	s->it[s->s - 1].B = it.B;

	/*
	printf("stack: push s=%d st=%d\n", s->s, s->st);
	printf("stack: push %d=(%d, %d, %x, %x)\n", s->s-1, s->it[s->s-1].d, s->it[s->s-1].p, s->it[s->s-1].h, s->it[s->s-1].B);
	*/

	return s->s;
}

/**
 * Ukazatel na vrchol zasobniku.
 * \returns     ukazatel na vrchol zasobniku.
 */
stack_item_t * stack_top(stack_t *s)
{
	assert(s);

	if(s->s > 0)
		return &s->it[s->s - 1];

	return NULL;
}

/**
 * Vyjmuti vrcholu zasobniku (vrati novou strukturu kterou je treba patricne
 * uvolnit!)
 * \returns     vyjmuty zaznam z vrcholu zasobniku
 */
stack_item_t * stack_pop(stack_t *s)
{
	assert(s);
	stack_item_t *it = NULL, *itp;

	if(s->s > 0) {
		itp = &s->it[s->s - 1];
		it = stack_item_init(itp->d, itp->p, itp->h, itp->B);

		// uklid po zaznamu
		itp->d = 0;
		itp->p = 0;
		itp->h = NULL;
		itp->B = NULL;
		s->s--;
	}

	return it;
}

