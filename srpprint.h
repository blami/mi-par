/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srpprint: vypisovaci makra pro lazeni/sledovani vystupu
 */
#ifndef __SRPPRINT_H
#define __SRPPRINT_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * Rizeni ladeni.
 */
#define DEBUG
//#define DEBUG_COMP "dump hist task stack utils mpi nompi core msg gen"
#define DEBUG_COMP "mpi msg"

#define NONE -1                             /* don't use node */

/*
 * Funkce vypise zformatovany radek fmt z node n do souboru f.
 */
#define srpfprintf(f, n, fmt, ...) {                            \
	if(n >= 0) {                                                \
		fprintf(f, "[%d]:%d %s:%03d %s(): " fmt "\n",           \
			n, getpid(), __FILE__, __LINE__, __func__,          \
			##__VA_ARGS__);                                     \
	} else {                                                    \
		fprintf(f, "%d %s:%03d %s(): " fmt "\n",                \
			getpid(), __FILE__, __LINE__, __func__,             \
			##__VA_ARGS__);                                     \
	}                                                           \
}

/**
 * Funkce vypise zformatovany radek fmt z node n na standratni vystup.
 */
#define srpprintf(n, fmt, ...) {                                \
	srpfprintf(stdout, n, fmt, ##__VA_ARGS__);                  \
}

/**
 * Ladici makro.
 */
#ifdef DEBUG
#define srpdebug(comp, n, fmt, ...) {                           \
	if(strstr(DEBUG_COMP, comp) != NULL) {                      \
		srpfprintf(stderr, n, "DEBUG-%s: " fmt,                 \
			comp, ##__VA_ARGS__);                               \
	}                                                           \
}
#else
#define srpdebug(comp, n, fmt, ...)
#endif /* DEBUG */

#endif /* __SRPPRINT_H */
