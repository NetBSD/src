/*	$NetBSD: zs_cons.h,v 1.3 2005/01/10 17:07:09 chs Exp $	*/

struct consdev;
struct zs_chanstate;

extern void *zs_conschan;

extern void nullcnprobe(struct consdev *);

extern int  zs_getc(void *);
extern void zs_putc(void *, int);

struct zschan *zs_get_chan_addr(int);

#ifdef	KGDB
void zs_kgdb_init(void);
void zskgdb(struct zs_chanstate *);
#endif
