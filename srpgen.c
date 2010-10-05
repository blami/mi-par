/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srpgen: generator zadani
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include "srputils.h"
#include "srptask.h"
#include "srpdump.h"


unsigned int n = -1;
unsigned int k = -1;
unsigned int q = -1;
unsigned int p = 10;
task_t* t;

/**
 * Zpracovat prepinace vcetne overeni spravnosti.
 */
void parse_args(int argc, char **argv)
{
	int a;
	while((a = getopt(argc, argv, "n:k:q:p:h")) != -1){
		switch(a) {
		case 'n':
			n = atoi(optarg);
			if(n < 5) {
				fprintf(stderr, "chyba: musi platit n >= 5\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'k':
			k = atoi(optarg);
			if(n > k || k > (n*(n+1)/2)) {
				fprintf(stderr, "chyba: musi platit %d <= k <= %d\n",
					n, (n*(n+1)/2));
				exit(EXIT_FAILURE);
			}
			break;
		case 'q':
			q = atoi(optarg);
			if(n > q || q > (n*n)) {
				fprintf(stderr, "chyba: musi platit %d <= q <= %d\n",
					n, n*n);
				exit(EXIT_FAILURE);
			}
			break;
		case 'p':
			p = atoi(optarg);
			if(p < 0 || p > 100) {
				fprintf(stderr, "chyba: musi platit 0 <= p <= 100\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'h':
			printf("srpgen: generator nahodnych reseni pro ulohu SRP\n\n"
				"Pouziti:\n"
				" -n ARG        strana sachovnice (n>=5)\n"
				" -k ARG        pocet jezdcu (n<=k<=(n*(n+1)/2)\n"
				" -q ARG        pocet nahodnych zpetnych tahu (n<=q<=(n*n))\n"
				" -p ARG        maximalni hodnota penalizace na policku (0<=p<=100)\n"
				" -h            zobrazi tuto napovedu\n\n");
			exit(EXIT_SUCCESS);
			break;
		}
	}

	if(n == -1 || k == -1 || q == -1) {
		fprintf(stderr, "chyba: povinne parametry jsou -n,-k,-q.\n");
		exit(EXIT_FAILURE);
	}
}

/**
 * Vstupni bod programu.
 */
int main(int argc, char **argv) {
	int i;
	int qq;
	int ki, di;

	parse_args(argc, argv);

	// inicializace zdroje nahodnych cisel
	srand(time(NULL));

	// inicializace do stavu reseni
	t = task_init(n, k, q);
	task_setup(t);

	for(i = 0; i < n*n; i++) {
		t->P[i] = ((rand() % p) + (rand() % 1));
	}

	// provedeni nahodnych tahu nahodnymi jezdci
	qq = q;
	while(qq != 0) {
		ki = rand() % t->k;         // vyber kone
		di = rand() % LLD;          // vyber smeru

		if(task_move(t, ki, di) == 1) {
			// tah byl uspesny
			qq--;
		}
	}

	/*
	dump_task(stdout, t);
	printf("\n---\n");
	*/
	dump_serialize(stdout, t);

	task_destroy(t);

	return EXIT_SUCCESS;
}
