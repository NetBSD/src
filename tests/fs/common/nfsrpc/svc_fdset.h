/*	$NetBSD: svc_fdset.h,v 1.1 2010/07/26 15:56:45 pooka Exp $	*/

void init_fdsets(void);
void alloc_fdset(void);
fd_set *get_fdset(void);
int *get_fdsetmax(void);
