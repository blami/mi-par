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
	it->dirty = 0;

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
 * Spocte delku zaznamu zasobniku v bytech.
 */
inline size_t stack_item_sizeof(const stack_item_t *it, const task_t *t) {
	assert(it);
	assert(t);
	size_t l = 0;
	int i,j;

	// zaznamy zasobniku
	l += sizeof(unsigned int) +     // it->d
		sizeof(unsigned int);       // it->p

	// historie tahu
	l += sizeof(unsigned int);      // it->h->l
	l += it->h->l * 
	(4 * sizeof(int));              // it->h->l * 2*{x,y}

	// konfigurace sachovnice
	l += t->k * (2 * sizeof(int));  // t->k * {x,y}

	return l;
}


#ifdef MPI
/**
 * Serializovat zaznam zasobniku
 */
char * stack_item_mpipack(const stack_item_t *it, const task_t *t, int *l) {
	assert(it);
	assert(t);
	char *b = NULL;
	int pos = 0;
	int i,j;

	*l = (int)stack_item_sizeof(it, t);
	b = (char *)utils_malloc((size_t)(*l) * sizeof(char));

	srpdebug("stack", node, "serializace zaznamu zasobniku <s=%d>");

	// hloubka ve stavovem prostoru, celkova penalizace
	// it->d, it->p
	MPI_Pack((void *)&it->d, 1, MPI_UNSIGNED, b, *l, &pos,
		MPI_COMM_WORLD);
	MPI_Pack((void *)&it->p, 1, MPI_UNSIGNED, b, *l, &pos,
		MPI_COMM_WORLD);

	// historie tahu
	// delka historie tahu
	// it->h->l
	MPI_Pack((void *)&it->h->l, 1, MPI_UNSIGNED, b, *l, &pos,
		MPI_COMM_WORLD);

	// jednotlive tahy v ramci historie tahu
	// it->h->h
	for(j = 0; j < it->h->l; j++) {
		MPI_Pack((void *)&it->h->h[j][FROM].x, 1, MPI_INT, b, *l, &pos,
			MPI_COMM_WORLD);
		MPI_Pack((void *)&it->h->h[j][FROM].y, 1, MPI_INT, b, *l, &pos,
			MPI_COMM_WORLD);
		MPI_Pack((void *)&it->h->h[j][TO].x, 1, MPI_INT, b, *l, &pos,
			MPI_COMM_WORLD);
		MPI_Pack((void *)&it->h->h[j][TO].y, 1, MPI_INT, b, *l, &pos,
			MPI_COMM_WORLD);
	}

	// konfigurace sachovnice
	// it->B
	for(j = 0; j < t->k; j++) {
		MPI_Pack((void *)&it->B[j].x, 1, MPI_INT, b, *l, &pos,
			MPI_COMM_WORLD);
		MPI_Pack((void *)&it->B[j].y, 1, MPI_INT, b, *l, &pos,
		MPI_COMM_WORLD);
	}

	srpdebug("stack", node, "delka bufferu: %db <zapsano %db>", *l, pos);
	assert(pos == *l);

	return b;
}

/**
 * De-serializovat zaznam zasobniku po prijmu pomoci MPI.
 */
/*
stack_item_t * stack_mpiunpack(const char *b, const task_t *t, const int l) {
	assert(b);
	assert(t);
	stack_item_t *it;
	int pos = 0;
	int i,j;
	move_t m;
	stack_item_t it;

	unsigned int

	it = stack_item_init();

	// delka zasobniku s->s
	MPI_Unpack((void *)b, l, &pos, &ss, 1, MPI_UNSIGNED, MPI_COMM_WORLD);

	srpdebug("stack", node, "deserializace zasobniku <s=%d>", ss);

	// zaznamy zasobniku s->it[]
	for(i = 0; i < ss; i++) {
		// zaznam
		// it->d, it->p
		MPI_Unpack((void *)b, l, &pos, &it.d, 1, MPI_UNSIGNED, MPI_COMM_WORLD);
		MPI_Unpack((void *)b, l, &pos, &it.p, 1, MPI_UNSIGNED, MPI_COMM_WORLD);

		// historie tahu vedouci ke stavu reprezentovaneho zaznamem
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

		// konfigurace sachovnice
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
*/
#endif /* MPI */


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
	s->s_real = 0;
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

	// nelze pouzit stack_item_destroy (alokovane pole)
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

	srpdebug("stack", node, "realokace zasobniku s=%d st=%d", s->s, s->st);
}

/**
 * Preskocit vsechny spinave zaznamy a snizit o ne s->s. Pri vhodnem pouziti
 * teto inline funkce snizime cas vypoctu o defragmentaci zasobniku.
 */
inline int stack_overleap(stack_t *s) {
	assert(s);

	int n = 0;

	while(s->s > 0 && s->it[s->s - 1].dirty != 0) {
		// pokud preskakuju musim zaznam vycistit
		s->it[s->s - 1].dirty = 0;
		s->it[s->s - 1].d = 0;
		s->it[s->s - 1].p = 0;
		s->it[s->s - 1].h = NULL;
		s->it[s->s - 1].B = NULL;

		s->s--;
		n++;
	}

#ifdef DEBUG
	if(n > 0)
		srpdebug("stack", node, "preskoceno n=%d polozek zasobniku", n);
#endif /* DEBUG */
}

/**
 * Kontrola plnosti zasobniku.
 */
int stack_empty(stack_t *s)
{
	assert(s);

	/*
	stack_overleap(s);
	return s->s == 0 ? 1 : 0;
	*/

	return s->s_real == 0 ? 1 : 0;
}

/**
 * Vlozeni na vrchol zasobniku.
 * \returns     nova velikost zasobniku
 */
int stack_push(stack_t *s, stack_item_t it)
{
	assert(s);

	stack_overleap(s);

	// doalokovat pamet je-li treba
	if(s->s == s->st - 1) {
		stack_resize(s, STACK_N);
	}
	s->s++;
	s->s_real++; // realna velikost

	// pri pushi nikdy neni zaznam spinavy
	s->it[s->s - 1].dirty = 0;
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

	stack_overleap(s);

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

	stack_overleap(s);

	if(s->s > 0) {
		itp = &s->it[s->s - 1];
		it = stack_item_init(itp->d, itp->p, itp->h, itp->B);

		// uklid po zaznamu
		itp->dirty = 0;
		itp->d = 0;
		itp->p = 0;
		itp->h = NULL;
		itp->B = NULL;

		// uprava velikosti
		s->s--;
		s->s_real--;
	}

	return it;
}

/**
 * Tato funkce "ukradne" ze zasobniku i-ty prvek. Pouziva se pouze pri deleni
 * zasobniku. Zpusobi nekonzistenci struktury (dira nema zadne vlastnosti
 * typicke pro stack_item_t a je oznacena jako dirty, aby ji funkce
 * stack_defrag mohla odstranit.
 * Odstraneni nelze ucinit rovnou, protoze by to zpusobilo nekonzistence
 * v indexech.
 * \param  i    index kradeneho prvku ve skutecnem poli (tedy mezi 0 a s->s)
 * \returns     vyjmuty zaznam ze zasobniku
 * \see stack_defrag()
 */
stack_item_t * stack_steal(stack_t *s, const int i)
{
	assert(s);
	stack_item_t *it = NULL, *itp;

	if(s->s > i && s->it[i].dirty != 1) {
		itp = &s->it[i];
		it = stack_item_init(itp->d, itp->p, itp->h, itp->B);

		// uklid po zaznamu
		itp->dirty = 1;
		itp->d = 0;
		itp->p = 0;
		itp->h = NULL;
		itp->B = NULL;

		// zmensim pocet zaznamu
		s->s_real--;
	}

	return it;
}

/**
 * Spocte delku zasobniku v bytech (pouze nespinave zaznamy).
 */
inline size_t stack_sizeof(const stack_t *s, const task_t *t) {
	assert(s);
	assert(t);
	size_t l = 0;
	int i,j;

	l += sizeof(unsigned int);          // s->s
	// zaznamy zasobniku
	for(i = 0; i < s->s; i++) {
		if(s->it[i].dirty == 0) {
			l += sizeof(unsigned int) + // s->it[i]->d
				sizeof(unsigned int);   // s->it[i]->p

			// historie tahu
			l += sizeof(unsigned int);  // s->it[i]->h->l
			l += s->it[i].h->l *
				(4 * sizeof(int));      // s->it[i]->h->l * 2*{x,y}

			// konfigurace sachovnice
			l += t->k * (2 * sizeof(int));  // t->k * {x,y}
		}
	}

	return l;
}

/**
 * Defragmentace zasobniku.
 */
int stack_defrag(stack_t *s) {
	assert(s);
	int i;
	int sn = s->s;                  // nova delka zasobniku po defragmentaci
	int shift = 0;                  // o kolik posouvat dalsi pole

	for(i = 0; i < s->s; i++) {
		if(s->it[i].dirty == 1) {
			shift++;
			sn--;
		} else {
			assert(i >= shift);
			if(shift > 0)
				s->it[i - shift] = s->it[i];
		}
	}
	// od sn do s musim vyprazdnit polozky (presun puvodnich der nakonec)
	for(i = sn; i < s->s; i++) {
		s->it[i].dirty = 0;
		s->it[i].d = 0;
		s->it[i].p = 0;
		s->it[i].h = NULL;
		s->it[i].B = NULL;
	}

	srpdebug("stack", node, "defragmentace zasobniku <n=%d, s=%d->%d>",
		shift, s->s, sn);

	s->s = sn;
	return sn;
}

/**
 * Rozdeleni zasobniku.
 * \param n     v novem zasobniku sr bude prave 1/n-tina zaznamu zasobniku s,
 *              v s tedy zbyde n-1/n zaznamu.
 * \returns     nove vzniklou n-tinu (mensi) zasobniku s. Ten je "ochuzen" o
 *              zaznamy presunute na nove vznikly zasobnik
 */
stack_t * stack_divide(stack_t *s, const int n) {
	assert(s);
	assert(n > 0);
	stack_t *sr = NULL;
	stack_item_t *it = NULL;
	unsigned int d = -1;            // aktualne delena hloubka
	unsigned int di[2] = {0, 0};    // index prvniho a posledniho prvku hloubky d
	unsigned int l;                 // delka useku mezi di[0] a di[1]

	int init = 1;                   // pomocna promenna pro inicializaci
	int i = 0, j = 0, k = 0;

	sr = stack_init();

	// projdu cely zasobnik ss ode dna
	while(i < s->s) {
		// osetreni krajni meze (konec)
		if(i == s->s - 1)
			di[1] = i;

		if(d != s->it[i].d || i == s->s - 1) {
			// osetreni krajni meze (zacatek)
			if(!init) {
				l = di[1] - di[0] + 1; // index od 0

				// odecist spinave zaznamy
				for(j = di[0]; j < di[1]; j++) {
					if(s->it[j].dirty != 0)
						l--;
				}

				//assert(l > 0);

				srpdebug("stack", node, "delena hloubka <d=%d, %d:%d, l=%d, m=%d>",
					d, di[0], di[1], l, (l/n));

				k = di[0] - 1;  // aktualni index v ramci celeho zasobniku
				j = 0;          // pocet ukradenych prvku
				//for(j = di[0]; j < di[0]+(l/n); j++) {
				while(j < l/n) {
					k++;
					// preskocit spinavy zaznam
					if(s->it[k].dirty != 0) {
						srpdebug("stack", node, "n");
						continue;
					}

					// vlozit "ukradenou" polozku do noveho zasobniku
					it = stack_steal(s, k);
					stack_push(sr, (*it));

					// uvolnit strukturu polozky zasobniku (nikoliv jeji B a h
					// pointer, ty jsou pouzity v ramci sr)
					it->h = NULL;
					it->B = NULL;
					stack_item_destroy(it);

					// zvysit pocet ukradenych polozek
					j++;
				}
			}
			d = s->it[i].d;
			di[0] = i;
		}
		di[1] = i;
		init = 0;
		i++;
	}


	// defragmentace puvodniho zasobniku
	//stack_defrag(s);

	return sr;
}

/**
 * Slouceni dvou zasobniku. Vlozi zaznamy zasobniku (od vrcholu ke dnu) sa na
 * vrchol zasobniku s. Zasobnik sa bude zrusen a hodnota ukazatele bude
 * nastavena na NULL.
 * \returns     vraci adresu s
 */
stack_t * stack_merge(stack_t *s, stack_t *sa) {
	assert(s);
	assert(sa);
	int i;
	stack_item_t *it;

	//vyresit pres steal, protoze potrebuju spravny razeni
	//while(!stack_empty(sa)) {
	for(i = 0; i < sa->s; i++) {
		//it = stack_pop(sa);
		it = stack_steal(sa, i);
		stack_push(s, (*it));
		// uvolnit strukturu polozky zasobniku (nikoliv jeji B a h
		// pointer, ty jsou pouzity v ramci sr)
		it->h = NULL;
		it->B = NULL;
		stack_item_destroy(it);
	}

	// zrusit zasobnik sa
	stack_destroy(sa);
	sa = NULL;

	return s;
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

	srpdebug("stack", node, "serializace zasobniku <s=%d>", s->s);

	// sestavit zpravu
	// delka zasobniku s->s
	MPI_Pack((void *)&s->s, 1, MPI_UNSIGNED, b, *l, &pos, MPI_COMM_WORLD);

	// zaznamy zasobniku ve tvaru d,p,historie<l,tahy<xz,yz,xdo,ydo>>,B<x,y>
	for(i = 0; i < s->s; i++) {
		// zaznam (pouze cisty)
		if(s->it[i].dirty == 0) {
			// hloubka ve stavovem prostoru, celkova penalizace
			// it->d, it->p
			MPI_Pack((void *)&s->it[i].d, 1, MPI_UNSIGNED, b, *l, &pos,
				MPI_COMM_WORLD);
			MPI_Pack((void *)&s->it[i].p, 1, MPI_UNSIGNED, b, *l, &pos,
				MPI_COMM_WORLD);

			// historie tahu
			// delka historie tahu
			// it->h->l
			MPI_Pack((void *)&s->it[i].h->l, 1, MPI_UNSIGNED, b, *l, &pos,
				MPI_COMM_WORLD);

			// jednotlive tahy v ramci historie tahu
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

			// konfigurace sachovnice
			// it->B
			for(j = 0; j < t->k; j++) {
				MPI_Pack((void *)&s->it[i].B[j].x, 1, MPI_INT, b,
					*l, &pos, MPI_COMM_WORLD);
				MPI_Pack((void *)&s->it[i].B[j].y, 1, MPI_INT, b,
					*l, &pos, MPI_COMM_WORLD);
			}
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

	// delka zasobniku s->s
	MPI_Unpack((void *)b, l, &pos, &ss, 1, MPI_UNSIGNED, MPI_COMM_WORLD);

	srpdebug("stack", node, "deserializace zasobniku <s=%d>", ss);

	// zaznamy zasobniku s->it[]
	for(i = 0; i < ss; i++) {
		// zaznam
		// it->d, it->p
		MPI_Unpack((void *)b, l, &pos, &it.d, 1, MPI_UNSIGNED, MPI_COMM_WORLD);
		MPI_Unpack((void *)b, l, &pos, &it.p, 1, MPI_UNSIGNED, MPI_COMM_WORLD);

		// historie tahu vedouci ke stavu reprezentovaneho zaznamem
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

		// konfigurace sachovnice
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

#ifdef STACK_TEST
/**
 * Unit test zasobniku.
 */
#endif /* STACK_TEST */
