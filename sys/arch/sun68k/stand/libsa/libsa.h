/*	$NetBSD: libsa.h,v 1.1.8.3 2002/06/20 03:42:00 nathanw Exp $	*/

/*
 * This file defines the API for libsa.a
 * as used by the various boot programs.
 */

/*
 * Standard Sun3 PROM load address.
 * i.e. this is where the PROM loads
 * programs, like it or not!
 */
#define KERN_LOADADDR	0x4000

/* SRT0.S */
void	ICIA __P((void));
void **	getvbr __P((void));

/* SRT1.c */
extern int _is3x;
extern int _is2;
void _start __P((void));
void breakpoint __P((void));
void chain_to __P((void *func));

/* clock.c */
extern int hz;
long getsecs __P((void));
long getticks __P((void));

/* exec_sun.c */
int exec_sun __P((char *file, char *loadaddr));
int load_sun __P((int io, char *loadaddr, char **entry));

/* promboot.c */
extern int debug;
extern char prom_bootdev[];
extern char *prom_bootfile;
extern int prom_boothow;

/* sun2.c */
void sun2_getidprom __P((u_char *ea));
u_long sun2_map_mem_load __P((void));
void *sun2_map_mem_run __P((void *));

/* sun3.c */
void sun3_getidprom __P((u_char *ea));

/* vers.c */
extern const char bootprog_rev[];
extern const char bootprog_name[];
