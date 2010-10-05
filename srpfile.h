/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srpfile: metody pro praci se souborem zadani (definice sachovnice)
 */
#ifndef __SRPFILE_H
#define __SRPFILE_H


typedef struct {
	int n;          // checkerboard side
	int *P;         // penalty array (n*n linear mapping)
	int k;          // knights count
	int *B;         // 

} srpfile;

int      srpfile_save(char *filename, srpfile *data);
srpfile* srpfile_load(char *filename);
int      srpfile_verify(srpfile *data);

#endif /* __SRPFILE_H */
