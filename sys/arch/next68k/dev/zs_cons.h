/*	$NetBSD: zs_cons.h,v 1.5 2026/11/08 19:34:00 thorpej Exp $	*/

struct consdev;
struct zs_chanstate;

extern void *zs_conschan;

extern void nullcnprobe(struct consdev *);

extern int  zs_getc(void *);
extern void zs_putc(void *, int);

struct zschan *zs_get_chan_addr(int);

#ifdef	KGDB
void zs_kgdb_init(void);
void zskgdb(void);
#endif
