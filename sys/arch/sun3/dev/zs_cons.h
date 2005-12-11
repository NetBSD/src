/*	$NetBSD: zs_cons.h,v 1.5 2005/12/11 12:19:21 christos Exp $	*/


struct consdev;

extern void *zs_conschan;

extern void nullcnprobe(struct consdev *);

extern int  zs_getc(void *);
extern void zs_putc(void *, int);

struct zschan *zs_get_chan_addr(int, int);

#ifdef	KGDB
void zs_kgdb_init(void);
#endif
