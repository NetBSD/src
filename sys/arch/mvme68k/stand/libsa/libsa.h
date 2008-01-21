/*	$NetBSD: libsa.h,v 1.4.40.1 2008/01/21 09:37:49 yamt Exp $	*/

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
time_t getsecs(void);

/* exec_mvme.c */
void exec_mvme(char *, int, int);

/* parse_args.c */
void parse_args(char **, int *, int *);

