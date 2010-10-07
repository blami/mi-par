/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srphist: funkce a struktury pro historii tahu
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "srphist.h"


/**
 * Inicializace historie tahu.
 * \param ho    predchozi historie ke zkopirovani nebo NULL
 */
hist_t * hist_init(const hist_t *ho) {
	hist_t *h;

	h = (hist_t *)utils_malloc(sizeof(hist_t));

	h->l = 0;
	h->h = NULL;

	// pokud existuje predchozi historie, tak ji _zkopirovat_
	if(ho) {
		h->l = ho->l;
		h->h = (move_t *)utils_malloc(h->l * sizeof(move_t));
		memcpy(h->h, ho->h, (h->l * sizeof(move_t)));
	}

	return h;
}

/**
 * Destrukce historie tahu.
 */
void hist_destroy(hist_t *h) {
	if(!h)
		return;

	if(h->h)
		free(h->h);

	free(h);
}

/**
 * Pridani tahu na konec historie.
 */
void hist_append_move(hist_t *h, move_t m) {
	assert(h);

	if(h->h == NULL) {
		// historie tahu je prazdna, vytvorit
		h->h = (move_t *)utils_malloc(sizeof(move_t));
		h->l = 1;
	} else {
		// historie tahu existuje, rozsirit
		h->h = (move_t *)utils_realloc(h->h, (h->l + 1) * sizeof(move_t));
		h->l++;
	}

	// tyto operace jsou afaik rychlejsi nez memcpy
	h->h[h->l - 1][FROM].x = m[FROM].x;     h->h[h->l - 1][FROM].y = m[FROM].y;
	h->h[h->l - 1][TO].x = m[TO].x;         h->h[h->l - 1][TO].y = m[TO].y;
}

/**
 * Nacteni souradnic z historie tahu.
 * \param a     stari tahu (od konce, tj. posledni je stary 1)
 */
coords_t hist_lookup_move(hist_t *h, const int d, const unsigned int a) {
	assert(h);
	assert(0 <= d < 2);
	assert(a != 0);         // soucasny tah jeste neni v historii
	coords_t c;

	c.x = -1;
	c.y = -1;

	// toto nelze resit aserci!
	if((!h->h) || (h->l <= a))
		return c;

	c.x = h->h[h->l - a][d].x;
	c.y = h->h[h->l - a][d].y;

	return c;
}
