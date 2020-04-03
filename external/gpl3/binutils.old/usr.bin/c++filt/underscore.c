/*	$NetBSD: underscore.c,v 1.6 2020/04/03 17:51:33 christos Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
