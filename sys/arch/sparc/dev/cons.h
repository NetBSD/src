/*	$NetBSD: cons.h,v 1.7 2026/11/08 19:34:00 thorpej Exp $	*/


/*
 * PROM I/O nodes and arguments are prepared by consinit().
 * Drivers can examine these when looking for a console device match.
 */
extern int prom_stdin_node;
extern int prom_stdout_node;
extern char prom_stdin_args[];
extern char prom_stdout_args[];
extern struct consdev consdev_prom;

#ifdef	KGDB
struct zs_chanstate;
void zs_kgdb_init(void);
void zskgdb(void);
#endif
