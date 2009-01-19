/*	$NetBSD: libsa.h,v 1.5.18.1 2009/01/19 13:16:31 skrll Exp $	*/

/*
 * libsa prototypes 
 */

#include "libbug.h"

/* bugdev.c */
int bugscopen(struct open_file *, ...);
int bugscclose(struct open_file *);
int bugscioctl(struct open_file *, u_long, void *);
int bugscstrategy(void *, int, daddr_t, size_t, void *, size_t *);

/* clock.c */
u_long chiptotime(int, int, int, int, int, int);

/* exec_mvme.c */
void exec_mvme(char *, int, int);

/* parse_args.c */
void parse_args(char **, int *, int *);

