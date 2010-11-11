/*
 * MI-PAR: Paralelni systemy a algoritmy
 * Semestralni uloha: SRP
 * Autori: Ondrej Balaz <balazond@fit.cvut.cz>
 *         Jan Mares <maresj16@fit.cvut.cz>
 */

/*
 * srptask: funkce a struktury pro praci s ulohou.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#ifdef MPI
#include <mpi.h>
#endif /* MPI */
#include "srpprint.h"
#include "srptask.h"
#include "srputils.h"
#include "srphist.h"


/**
 * Inicializace ulohy.
 */
task_t * task_init(const unsigned int n, const unsigned int k,
	const unsigned int q) {
	task_t *t = NULL;
	unsigned int i;

	t = (task_t *)utils_malloc(sizeof(task_t));
	t->n = n;
	t->k = k;
	t->q = q;

	t->P = (unsigned int *)utils_malloc(sizeof(unsigned int) * (n*n));
	memset(t->P, 0, sizeof(unsigned int) * (n*n));
	/*
	for(i = 0; i < n*n; i++)
		t->P[i] = 0;
	*/

	t->B = (coords_t *)utils_malloc(sizeof(coords_t) * k);
	memset(t->B, 0, sizeof(coords_t) * k);

	// inicializovat vsechny figurky mimo sachovnici
	task_clean(t);

	return t;
}

/**
 * Destrukce ulohy.
 */
void task_destroy(task_t *t) {
	if(!t) return;

	if(t->P)
		free(t->P);

	if(t->B)
		free(t->B);

	free(t);
}

/**
 * Odstraneni figurek ze sachovnice.
 * \returns     pocet figurek odstranenych ze sachovnice
 */
unsigned int task_clean(task_t *t) {
	assert(t);
	unsigned int c = 0;
	unsigned int i;

	for(i = 0; i < t->k; i++) {
		if(t->B[i].x > -1 && t->B[i].y > -1)
			c++;

		t->B[i].x = -1; t->B[i].y = -1;
	}

	return c;
}

/**
 * Umisteni figurek do horniho trojuhelniku sachovnice.
 */
void task_setup(task_t *t) {
	assert(t);
	unsigned int x = 0, y = 0; // horni levy roh
	unsigned int edge = t->n;
	unsigned int count = t->k;
	unsigned int i;

	task_clean(t);

	for(i = 0; i < count; i++) {
		t->B[i].x = x; t->B[i].y = y;
		x++;
		if(x >= edge) {
			x = 0;
			y++;
			edge--;
		}
	}
}

/**
 * Kontrola pozice X,Y na sachovnici.
 * \param t     zadani ulohy
 * \param B     sachovnice (pro NULL se pouzije primo t->B)
 * \returns     1 pokud je pozice obsazena, 0 pokud ne
 */
int task_get_pos(task_t *t, coords_t *B, const coords_t c) {
	assert(t);
	unsigned int i;
	coords_t *BB = t->B;

	if(B != NULL)
		BB = B;

	// tah mimo nemuze byt aserce, protoze tah mimo je validni pokus
	if(c.x < 0 || c.x >= t->n || c.y < 0 || c.y >= t->n)
		return 1; // policko je neplatne (neexistuje)

	for(i = 0; i < t->k; i++) {
		if(BB[i].x == c.x && BB[i].y == c.y)
			return 1;
	}

	return 0;
}

/**
 * Nastaveni pozice X,Y na sachovnici. Kontroluje obsazenost ciloveho pole.
 * \param t     zadani ulohy
 * \param B     sachovnice (pro NULL se pouzije primo t->B)
 * \param i     index figurky jejiz pozice ma byt nastavena
 * \param c     souradnice cilove pozice figurky
 * \returns     1 pokud doslo k presunu, 0 bylo-li pole obsazeno
 */
int task_set_pos(task_t *t, coords_t *B, const unsigned int i, 
	const coords_t c) {
	assert(t);
	assert(i < t->k);
	coords_t *BB = t->B;

	if(B != NULL)
		BB = B;

	BB[i].x = c.x; BB[i].y = c.y;
	return 1;
}

/**
 * Regulerni tah figurkou jezdce po sachovnici.
 * \param t     zadani ulohy
 * \param B     sachovnice (pro NULL se pouzije primo t->B)
 * \param i     index figurky se kterou ma byt proveden tah
 * \param d     smer tahu (\see srptask.h)
 * \param m     struktura tahu, ktera ma byt naplnena v pripade, ze tah je
 *      platny a byl proveden
 * \param p     celkova penalizace, kam ma byt pripoctena dilci penalizace za
 *      platny provedeny tah
 * \param dry_run 1 pokud nechci tah skutecne provest
 * \returns     1 tah byl spravny (proveden), 0 tah byl nespravny (neproveden)
 */
int task_move(task_t *t, coords_t *B, const unsigned int i, const dir_t d,
	move_t *m, unsigned int *p, int dry_run)  {
	assert(t);
	assert(i < t->k);
	assert(0 < d < 8);
	int j;
	coords_t* BB = t->B;

	if(B != NULL)
		BB = B;
	assert(BB[i].x >= 0 && BB[i].y >= 0); // kun musi byt umisten

	coords_t c, c_old; // nove a puvodni souradnice

	c_old.x = BB[i].x;
	c_old.y = BB[i].y;

	switch(d) {
	case RUU:
		c.x = BB[i].x + 1; c.y = BB[i].y - 2;
		break;
	case RRU:
		c.x = BB[i].x + 2; c.y = BB[i].y - 1;
		break;
	case RDD:
		c.x = BB[i].x + 1; c.y = BB[i].y + 2;
		break;
	case RRD:
		c.x = BB[i].x + 2; c.y = BB[i].y + 1;
		break;
	case LUU:
		c.x = BB[i].x - 1; c.y = BB[i].y - 2;
		break;
	case LLU:
		c.x = BB[i].x - 2; c.y = BB[i].y - 1;
		break;
	case LDD:
		c.x = BB[i].x - 1; c.y = BB[i].y + 2;
		break;
	case LLD:
		c.x = BB[i].x - 2; c.y = BB[i].y + 1;
		break;
	default:
		// neznamy smer tahu, prazdny tah
		return 0;
	}

	// overit jestli je tah mozny
	if(task_get_pos(t, B, c) == 1) {
		// na cilovem policku je figurka, nebo policko neexistuje
		return 0;
	}

	// vratime tah pres parametr pro ucely historie tahu
	if(m != NULL) {
		(*m)[TO].x = c.x;
		(*m)[TO].y = c.y;
		(*m)[FROM].x = c_old.x;
		(*m)[FROM].y = c_old.y;
	}
	// zvysime penalizaci
	if(p != NULL) {
		j = utils_map(c, t->n);
		*p += t->P[j];
	}

	if(dry_run != 1) {
		task_set_pos(t, B, i, c);
	}

	return 1;
}

/**
 * Zkopiruje pole souradnic figurek na sachovnici.
 */
inline coords_t * task_bdcpy(const task_t *t, coords_t *B) {
	assert(t);
	coords_t *Bd = NULL;
	coords_t *BB = t->B;
	int i;

	if(B != NULL)
		BB = B;

	Bd = (coords_t *)utils_malloc(sizeof(coords_t) * t->k);
	for(i = 0; i < t->k; i++) {
		Bd[i] = BB[i];
	}

	return Bd;
}

/**
 * Spocte delku ulohy v bytech.
 * \param include_board     pro 0 nezahrne velikost t->B (viz. poznamka 
 *                          v mpipack)
 */
inline size_t task_sizeof(const task_t *t, const int include_board) {
	assert(t);
	size_t l;

	l = sizeof(unsigned int) +                      // t->n
		sizeof(unsigned int) +                      // t->k
		sizeof(unsigned int) +                      // t->q
		(include_board == 1 ?
			((t->k) * sizeof(coords_t)) : 0) +      // t->B
		((t->n * t->n) * sizeof(unsigned int));     // t->P

	return l;
}

#ifdef MPI
/**
 * Serializovat ukol pro odeslani v MPI.
 * \param l                 bude obsahovat delku vracene zpravy
 * \param include_board     nezahrnovat konfiguraci sachovnice
 */
char * task_mpipack(const task_t *t, int *l, const int include_board) {
	assert(t);
	assert(l);
	char *b = NULL;
	int pos = 0;
	int i;

	*l = (int)task_sizeof(t, include_board);
	b = (char *)utils_malloc((size_t)(*l) * sizeof(char));

	srpdebug("task", node, "serializace ulohy <n=%d, k=%d, q=%d>", t->n,
		t->k, t->q);

	// sestavit zpravu
	// n,k,q
	MPI_Pack((void *)&t->n, 1, MPI_UNSIGNED, b, *l, &pos, MPI_COMM_WORLD);
	MPI_Pack((void *)&t->k, 1, MPI_UNSIGNED, b, *l, &pos, MPI_COMM_WORLD);
	MPI_Pack((void *)&t->q, 1, MPI_UNSIGNED, b, *l, &pos, MPI_COMM_WORLD);

	// konfiguraci sachovnice si neni treba vymenovat pri inicializaci vypoctu.
	// Jednotlive uzly totiz dostanou konfiguraci sachovnice i s historii od
	// zadani jako soucast STAVU pri obdrzeni prace (jak uloha vypadala
	// potrebuji vedet jen z hlediska n,k,q a P).
	if(include_board) {
		// B, struct rozeberu ve tvaru x0,y0,x1,y0 .. xk-1,yk-1
		for(i = 0; i < t->k; i++) {
			MPI_Pack((void *)&t->B[i].x, 1, MPI_INT, b, *l, &pos,
				MPI_COMM_WORLD);
			MPI_Pack((void *)&t->B[i].y, 1, MPI_INT, b, *l, &pos,
				MPI_COMM_WORLD);
		}
	}

	// P, n*n velke linearni pole
	MPI_Pack((void *)t->P, t->n*t->n, MPI_UNSIGNED, b, *l, &pos,
		MPI_COMM_WORLD);

	srpdebug("task", node, "delka bufferu: %db <zapsano %db>", *l, pos);
	assert(pos == *l);

	return b;
}

/**
 * Deserializovat ukol po prijmu v MPI.
 * \param l                 delka prijate zpravy (muze byt vetsi)
 * \param include_board     pokud je 0 nepocitat s konfiguraci sachovnice
 */
task_t * task_mpiunpack(const char *b, const int l, const int include_board) {
	assert(b);
	task_t *t = NULL;
	int pos = 0;
	int i;
	unsigned int n;
	unsigned int k;
	unsigned int q;

	// n,k,q
	MPI_Unpack((void *)b, l, &pos, &n, 1, MPI_UNSIGNED, MPI_COMM_WORLD);
	MPI_Unpack((void *)b, l, &pos, &k, 1, MPI_UNSIGNED, MPI_COMM_WORLD);
	MPI_Unpack((void *)b, l, &pos, &q, 1, MPI_UNSIGNED, MPI_COMM_WORLD);

	srpdebug("task", node, "deserializace ulohy <n=%d, k=%d, q=%d>", n, k, q);

	t = task_init(n, k, q);

	if(include_board) {
		// B
		for(i = 0; i < t->k; i++) {
			MPI_Unpack((void *)b, l, &pos, &t->B[i].x, 1, MPI_INT,
				MPI_COMM_WORLD);
			MPI_Unpack((void *)b, l, &pos, &t->B[i].y, 1, MPI_INT,
				MPI_COMM_WORLD);
		}
	}

	// P
	MPI_Unpack((void *)b, l, &pos, t->P, t->n*t->n, MPI_UNSIGNED,
		MPI_COMM_WORLD);

	srpdebug("task", node, "delka bufferu: %db <precteno %db>", l, pos);

	return t;
}
#endif /* MPI */
