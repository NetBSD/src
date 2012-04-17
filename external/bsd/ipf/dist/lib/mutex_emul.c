/*	$NetBSD: mutex_emul.c,v 1.1.1.1.2.2 2012/04/17 00:03:18 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */

#include "ipf.h"

#define	EMM_MAGIC	0x9d7adba3

static int mutex_debug = 0;

void eMmutex_enter(mtx, file, line)
	eMmutex_t *mtx;
	char *file;
	int line;
{
	if (mutex_debug & 2)
		fprintf(stderr, "%s:%d:eMmutex_enter(%s)\n", file, line,
		       mtx->eMm_owner);
	if (mtx->eMm_magic != EMM_MAGIC) {
		fprintf(stderr, "%s:eMmutex_enter(%p): bad magic: %#x\n",
			mtx->eMm_owner, mtx, mtx->eMm_magic);
		abort();
	}
	if (mtx->eMm_held != 0) {
		fprintf(stderr, "%s:eMmutex_enter(%p): already locked: %d\n",
			mtx->eMm_owner, mtx, mtx->eMm_held);
		abort();
	}
	mtx->eMm_held++;
	mtx->eMm_heldin = file;
	mtx->eMm_heldat = line;
}


void eMmutex_exit(mtx, file, line)
	eMmutex_t *mtx;
	char *file;
	int line;
{
	if (mutex_debug & 2)
		fprintf(stderr, "%s:%d:eMmutex_exit(%s)\n", file, line,
		       mtx->eMm_owner);
	if (mtx->eMm_magic != EMM_MAGIC) {
		fprintf(stderr, "%s:eMmutex_exit(%p): bad magic: %#x\n",
			mtx->eMm_owner, mtx, mtx->eMm_magic);
		abort();
	}
	if (mtx->eMm_held != 1) {
		fprintf(stderr, "%s:eMmutex_exit(%p): not locked: %d\n",
			mtx->eMm_owner, mtx, mtx->eMm_held);
		abort();
	}
	mtx->eMm_held--;
	mtx->eMm_heldin = NULL;
	mtx->eMm_heldat = 0;
}


static int initcount = 0;


void eMmutex_init(mtx, who, file, line)
	eMmutex_t *mtx;
	char *who;
	char *file;
	int line;
{
	if (mutex_debug & 1)
		fprintf(stderr, "%s:%d:eMmutex_init(%p,%s)\n",
			file, line, mtx, who);
	if (mtx->eMm_magic == EMM_MAGIC) {	/* safe bet ? */
		fprintf(stderr,
			"%s:eMmutex_init(%p): already initialised?: %#x\n",
			mtx->eMm_owner, mtx, mtx->eMm_magic);
		abort();
	}
	mtx->eMm_magic = EMM_MAGIC;
	mtx->eMm_held = 0;
	if (who != NULL)
		mtx->eMm_owner = strdup(who);
	else
		mtx->eMm_owner = NULL;
	initcount++;
}


void eMmutex_destroy(mtx, file, line)
	eMmutex_t *mtx;
	char *file;
	int line;
{
	if (mutex_debug & 1)
		fprintf(stderr, "%s:%d:eMmutex_destroy(%p,%s)\n", file, line,
		       mtx, mtx->eMm_owner);
	if (mtx->eMm_magic != EMM_MAGIC) {
		fprintf(stderr, "%s:eMmutex_destroy(%p): bad magic: %#x\n",
			mtx->eMm_owner, mtx, mtx->eMm_magic);
		abort();
	}
	if (mtx->eMm_held != 0) {
		fprintf(stderr, "%s:eMmutex_enter(%p): still locked: %d\n",
			mtx->eMm_owner, mtx, mtx->eMm_held);
		abort();
	}
	if (mtx->eMm_owner != NULL)
		free(mtx->eMm_owner);
	memset(mtx, 0xa5, sizeof(*mtx));
	initcount--;
}


void ipf_mutex_clean()
{
	if (initcount != 0)
		abort();
}
