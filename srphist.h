/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srphist: funkce a struktury pro historii tahu
 */
#ifndef __SRPHIST_H
#define __SRPHIST_H


#define FROM    0
#define TO      1

/**
 * Datovy typ tah.
 */
typedef coords_t move_t[2];     // indexy: 0 je z, 1 je do

/**
 * Struktura historie tahu.
 */
typedef struct {
	move_t *h;                  // tahy v historii
	int l;                      // delka historie tahu
} hist_t;

hist_t *    hist_init(const hist_t *ho);
void        hist_destroy(hist_t *h);
void        hist_move(hist_t *h, move_t m);

#endif /* __SRPHIST_H */
