/*	$NetBSD: underscore.c,v 1.2.6.2 2012/11/20 18:52:38 matt Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
