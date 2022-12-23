/*	$NetBSD: underscore.c,v 1.7 2022/12/23 17:09:41 christos Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
