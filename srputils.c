/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srputils: pomocne funkce
 */
#include "srputils.h"


/**
 * Mapovaci funkce vrati index v linearnim poli pro souradnice x,y v ramci
 * matice o velikosti n*n.
 */
inline int utils_map(const coords_t c, const int n) {
	return (c.y * n) + c.x;
}

/**
 * Un-mapovaci funkce vrati souradnice pro index v linearnim poli
 * predstavujicim matici o velikosti n*n.
 */
inline coords_t utils_unmap(const int i, const int n) {
	coords_t c;
	c.x = i % n;
	c.y = i / n;
	return c;
}
