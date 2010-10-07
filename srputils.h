/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srputils: pomocne funkce
 */
#ifndef __SRPUTILS_H
#define __SRPUTILS_H


typedef struct {
	int x;
	int y;
} coords_t;

inline int      utils_map(const coords_t, const int n);
inline coords_t utils_unmap(const int i, const int n);
void *          utils_malloc(size_t s);
void *          utils_realloc(void *ptr, size_t s);

#endif /* __SRPUTILS_H */
