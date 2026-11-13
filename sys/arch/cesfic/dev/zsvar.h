/* $NetBSD: zsvar.h,v 1.3 2026/11/13 01:23:13 thorpej Exp $ */

#define ZS_DELAY()

int zscngetc(dev_t);
void zscnputc(dev_t, int);
void zscnpollc(dev_t, int);

void zs_cnattach(void);
void zs_cninit(void *);

void zs_config(struct zsc_softc *, char*);

int zshard(void*);

#define ZSHARD_PRI 4
