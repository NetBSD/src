/* $NetBSD: zsvar.h,v 1.2 2009/03/14 14:45:58 dsl Exp $ */

#define ZS_DELAY()

int zscngetc(dev_t);
void zscnputc(dev_t, int);
void zscnpollc(dev_t, int);

void zs_cnattach(void *);
void zs_cninit(void *);

void zs_config(struct zsc_softc *, char*);

int zshard(void*);

#define ZSHARD_PRI 4
