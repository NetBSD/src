/*	$NetBSD: cons.h,v 1.3 2000/02/12 12:51:03 pk Exp $	*/


struct consdev;
struct zs_chanstate;

extern void *zs_conschan;

extern void nullcnprobe __P((struct consdev *));

extern int  zs_getc __P((void *arg));
extern void zs_putc __P((void *arg, int c));

#ifdef	KGDB
void zs_kgdb_init __P((void));
void zskgdb __P((struct zs_chanstate *));
#endif
