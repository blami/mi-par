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


void    dump_task(FILE *f, task_t *t);
void    dump_board(FILE *f, task_t *t);
void    dump_serialize(FILE *f, task_t *t);

#endif /* __SRPDUMP_H */
