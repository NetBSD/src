/*	$NetBSD: underscore.c,v 1.5 2018/04/14 17:53:09 christos Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
