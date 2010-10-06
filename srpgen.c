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
#include <string.h>
#include "srputils.h"
#include "srphist.h"
#include "srptask.h"
#include "srpdump.h"


unsigned int n = -1;
unsigned int k = -1;
unsigned int q = -1;
unsigned int p = 10;
task_t* t;
hist_t* h;
move_t m;
char* filename = NULL;
int verbose = 0;

/**
 * Zpracovat prepinace vcetne overeni spravnosti.
 */
void parse_args(int argc, char **argv)
{
	int a;
	while((a = getopt(argc, argv, "n:k:q:p:vh")) != -1){
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
		case 'v':
			verbose = 1;
			break;
		case 'h':
			printf("srpgen: generator nahodnych reseni pro ulohu SRP\n\n"
				"Pouziti: srpgen [prepinace] [<soubor>]\n"
				" -n ARG        strana sachovnice (ARG>=5)\n"
				" -k ARG        pocet jezdcu (n<=ARG<=(n*(n+1)/2)\n"
				" -q ARG        pocet nahodnych zpetnych tahu  "
					"(n<=ARG<=(n*n))\n"
				" -p ARG        maximalni hodnota penalizace na policku "
					"(0<=ARG<=100)\n"
				" -v            vypsat ulohu na obrazovku v lidsky citelne "
					"forme\n"
				" -h            zobrazi tuto napovedu\n\n"
				"Pokud chybi <soubor> program vypise vystup na stdout.\n\n");
			exit(EXIT_SUCCESS);
			break;
		}
	}

	if(optind < argc) {
		if(optind + 1 < argc) {
			fprintf(stderr, "chyba: neocekavany argument\n");
			exit(EXIT_FAILURE);
		}

		filename = strndup(argv[optind], strlen(argv[optind]));
	}

	if(n == -1 || k == -1 || q == -1) {
		fprintf(stderr, "chyba: povinne parametry jsou -n,-k a -q\n");
		exit(EXIT_FAILURE);
	}
}

/**
 * Vstupni bod programu.
 */
int main(int argc, char **argv) {
	unsigned int i;
	unsigned int qq;
	unsigned int ki, di;
	FILE *f = stdout;

	parse_args(argc, argv);

	// inicializace zdroje nahodnych cisel
	srand(time(NULL));

	// inicializace do stavu reseni
	t = task_init(n, k, q);
	task_setup(t);

	h = hist_init(NULL);

	for(i = 0; i < n*n; i++) {
		t->P[i] = ((rand() % p) + (rand() % 1));
	}

	// provedeni nahodnych tahu nahodnymi jezdci
	qq = q;
	while(qq != 0) {
		ki = rand() % t->k;         // vyber figurky
		di = rand() % LLD;          // vyber smeru

		if(task_move(t, ki, di, &m, NULL) == 1) {
			hist_move(h, m);
			qq--;
		}
	}

	if(filename != NULL) {
		if(!(f = fopen(filename, "w"))) {
			fprintf(stderr, "chyba: nelze zapisovat do souboru `%s'",
				filename);
			exit(EXIT_FAILURE);
		}
	}

	dump_serialize(f, t);
	if(verbose) {
		dump_task(stdout, t);
		dump_hist(stdout, h);
	}

	if(filename != NULL) {
		free(filename);
		fclose(f);
	}

	hist_destroy(h);
	task_destroy(t);

	return EXIT_SUCCESS;
}
