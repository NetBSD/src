/*	$NetBSD: underscore.c,v 1.1 1998/08/16 23:35:21 tv Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
