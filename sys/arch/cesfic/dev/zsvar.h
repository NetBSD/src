/* $NetBSD: zsvar.h,v 1.1 2001/05/14 18:23:08 drochner Exp $ */

#define ZS_DELAY()

int zscngetc __P((dev_t));
void zscnputc __P((dev_t, int));
void zscnpollc __P((dev_t, int));

void zs_cnattach __P((void *));
void zs_cninit __P((void *));

void zs_config __P((struct zsc_softc *, char*));

int zshard __P((void*));

#define ZSHARD_PRI 4
