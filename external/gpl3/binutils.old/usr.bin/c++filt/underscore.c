/*	$NetBSD: underscore.c,v 1.3 2016/10/26 17:09:38 christos Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
