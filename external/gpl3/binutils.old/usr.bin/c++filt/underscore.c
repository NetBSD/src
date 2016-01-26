/*	$NetBSD: underscore.c,v 1.1.1.1 2016/01/26 17:27:00 christos Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
