/*	$NetBSD: libsa.h,v 1.4 2000/07/24 18:39:46 jdolecek Exp $	*/

/*
 * libsa prototypes 
 */

#include "libbug.h"

/* bugdev.c */
int bugscopen __P((struct open_file *, ...));
int bugscclose __P((struct open_file *));
int bugscioctl __P((struct open_file *, u_long, void *));
int bugscstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));

/* clock.c */
u_long chiptotime __P((int, int, int, int, int, int));
time_t getsecs __P((void));

/* exec_mvme.c */
void exec_mvme __P((char *, int, int));

/* parse_args.c */
void parse_args __P((char **, int *, int *));

