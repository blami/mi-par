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
#ifdef MPI
#include <mpi.h>
#endif /* MPI */
#include "srpprint.h"
#include "srpstack.h"
#include "srptask.h"
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
void stack_resize(stack_t *s, unsigned int sn) {
	assert(s);

	s->it = (stack_item_t *)utils_realloc(s->it,
		sizeof(stack_item_t) * (s->st + sn));
	s->st += sn;

	srpdebug("stack", node, "realokace stacku s=%d st=%d", s->s, s->st);
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

/**
 * Spocte delku zasobniku v bytech.
 */
inline size_t stack_sizeof(const stack_t *s, const task_t *t) {
	assert(s);
	assert(t);
	size_t l = 0;
	int i,j;

	l += sizeof(unsigned int);          // s->s
	// zaznamy zasobniku
	for(i = 0; i < s->s; i++) {
		l += sizeof(unsigned int) +     // s->it[i]->d
			sizeof(unsigned int);       // s->it[i]->p

		// historie tahu
		l += sizeof(unsigned int);      // s->it[i]->h->l
		l += s->it[i].h->l * 
			(4 * sizeof(int));          // s->it[i]->h->l * 2*{x,y}

		// konfigurace sachovnice
		l += t->k * (2 * sizeof(int));  // t->k * {x,y}
	}

	return l;
}

/**
 * Rozdeleni zasobniku.
 * \param n     v novem zasobniku bude prave 1/n-tina zaznamu, v zbyde tedy
 *              n-1/n zaznamu.
 * \returns     nove vzniklou n-tinu (mensi) zasobniku s. Ten je "ochuzen" o
 *              zaznamy presunute na nove vznikly zasobnik
 */
stack_t * stack_divide(stack_t *s, int n) {
	// TODO
	return NULL;
}

#ifdef MPI
/**
 * Serializovat zasobnik pro odeslani pomoci MPI. Posila se jen velikost
 * zasobniku a jednotlive zaznamy vcetne historii. Velikost celeho zasobniku do
 * dalsi realokace neni potreba.
 */
char * stack_mpipack(const stack_t *s, const task_t *t, int *l) {
	assert(s);
	assert(t);
	char *b = NULL;
	int pos = 0;
	int i,j;

	*l = (int)stack_sizeof(s, t);
	b = (char *)utils_malloc((size_t)(*l) * sizeof(char));

	srpdebug("stack", node, "zasobnik s=%d", s->s);

	// sestavit zpravu
	// s
	MPI_Pack((void *)&s->s, 1, MPI_UNSIGNED, b, *l, &pos, MPI_COMM_WORLD);

	// zaznamy ve tvaru d,p,historie<l,tahy<xz,yz,xdo,ydo>>,B<x,y>
	for(i = 0; i < s->s; i++) {
		// it->d, it->p
		MPI_Pack((void *)&s->it[i].d, 1, MPI_UNSIGNED, b, *l, &pos,
			MPI_COMM_WORLD);
		MPI_Pack((void *)&s->it[i].p, 1, MPI_UNSIGNED, b, *l, &pos,
			MPI_COMM_WORLD);

		// it->h->l
		MPI_Pack((void *)&s->it[i].h->l, 1, MPI_UNSIGNED, b, *l, &pos,
			MPI_COMM_WORLD);

		// it->h->h
		for(j = 0; j < s->it[i].h->l; j++) {
			MPI_Pack((void *)&s->it[i].h->h[j][FROM].x, 1, MPI_INT, b,
				*l, &pos, MPI_COMM_WORLD);
			MPI_Pack((void *)&s->it[i].h->h[j][FROM].y, 1, MPI_INT, b,
				*l, &pos, MPI_COMM_WORLD);
			MPI_Pack((void *)&s->it[i].h->h[j][TO].x, 1, MPI_INT, b,
				*l, &pos, MPI_COMM_WORLD);
			MPI_Pack((void *)&s->it[i].h->h[j][TO].y, 1, MPI_INT, b,
				*l, &pos, MPI_COMM_WORLD);
		}

		// it->B
		for(j = 0; j < t->k; j++) {
			MPI_Pack((void *)&s->it[i].B[j].x, 1, MPI_INT, b,
				*l, &pos, MPI_COMM_WORLD);
			MPI_Pack((void *)&s->it[i].B[j].y, 1, MPI_INT, b,
				*l, &pos, MPI_COMM_WORLD);
		}
	}

	srpdebug("stack", node, "delka bufferu: %db <zapsano %db>", *l, pos);
	assert(pos == *l);

	return b;
}

/**
 * De-serializovat zasobnik po prijmu pomoci MPI.
 */
stack_t * stack_mpiunpack(const char *b, const task_t *t, const int l) {
	assert(b);
	assert(t);
	stack_t *s;
	int pos = 0;
	int i,j;
	unsigned int ss, hl;
	move_t m;
	stack_item_t it;

	s = stack_init();

	// s->s
	MPI_Unpack((void *)b, l, &pos, &ss, 1, MPI_UNSIGNED, MPI_COMM_WORLD);

	srpdebug("stack", node, "zasobnik v bufferu s=%d", ss);

	// s->it[]
	for(i = 0; i < ss; i++) {
		// it->d, it->p
		MPI_Unpack((void *)b, l, &pos, &it.d, 1, MPI_UNSIGNED, MPI_COMM_WORLD);
		MPI_Unpack((void *)b, l, &pos, &it.p, 1, MPI_UNSIGNED, MPI_COMM_WORLD);

		// it->h
		MPI_Unpack((void *)b, l, &pos, &hl, 1, MPI_UNSIGNED, MPI_COMM_WORLD);
		it.h = hist_init(NULL);
		for(j = 0; j < hl; j++) {
			MPI_Unpack((void *)b, l, &pos, &m[FROM].x, 1, MPI_INT,
				MPI_COMM_WORLD);
			MPI_Unpack((void *)b, l, &pos, &m[FROM].y, 1, MPI_INT,
				MPI_COMM_WORLD);
			MPI_Unpack((void *)b, l, &pos, &m[TO].x, 1, MPI_INT,
				MPI_COMM_WORLD);
			MPI_Unpack((void *)b, l, &pos, &m[TO].y, 1, MPI_INT,
				MPI_COMM_WORLD);

			hist_append_move(it.h, m);
		}

		// it->B
		it.B = (coords_t *)utils_malloc((t->k) * sizeof(coords_t));
		for(j = 0; j < t->k; j++) {
			MPI_Unpack((void *)b, l, &pos, &it.B[j].x, 1, MPI_INT,
				MPI_COMM_WORLD);
			MPI_Unpack((void *)b, l, &pos, &it.B[j].y, 1, MPI_INT,
				MPI_COMM_WORLD);
		}

		stack_push(s, it);
	}

	srpdebug("stack", node, "delka bufferu: %db <precteno %db>", l, pos);

	return s;
}
#endif /* MPI */
