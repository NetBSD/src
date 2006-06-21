/*	$NetBSD: cons.h,v 1.4.44.1 2006/06/21 14:55:54 yamt Exp $	*/


/*
 * PROM I/O nodes and arguments are prepared by consinit().
 * Drivers can examine these when looking for a console device match.
 */
extern int prom_stdin_node;
extern int prom_stdout_node;
extern char prom_stdin_args[];
extern char prom_stdout_args[];

#ifdef	KGDB
struct zs_chanstate;
void zs_kgdb_init(void);
void zskgdb(struct zs_chanstate *);
#endif
