/*	$NetBSD: cons.h,v 1.2 1998/01/05 07:03:20 perry Exp $	*/


struct consdev;
struct zs_chanstate;

extern void *zs_conschan;

extern void nullcnprobe __P((struct consdev *));

extern int  zs_getc __P((void *arg));
extern void zs_putc __P((void *arg, int c));

struct zschan *
zs_get_chan_addr __P((int zsc_unit, int channel));

#ifdef	KGDB
void zs_kgdb_init __P((void));
void zskgdb __P((struct zs_chanstate *));
#endif
