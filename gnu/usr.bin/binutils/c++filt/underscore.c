/*	$NetBSD: underscore.c,v 1.1 1998/08/27 20:32:09 tv Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
