/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srpdump: pomocne funkce pro vypis a nacteni struktury problemu
 */
#ifndef __SRPDUMP_H
#define __SRPDUMP_H
#include "srptask.h"
#include "srputils.h"
#include "srphist.h"

extern int node;    /* srpnompi.c/srpmpi.c */

extern void         dump_serialize(FILE *f, task_t *t);
extern task_t *     dump_unserialize(FILE *f);
extern void         dump_task(FILE *f, task_t *t);
extern void         dump_board(FILE *f, task_t *t, coords_t *B);
extern void         dump_hist(FILE *f, hist_t *h);

#endif /* __SRPDUMP_H */
