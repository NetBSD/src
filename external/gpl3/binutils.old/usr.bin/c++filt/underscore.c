/*	$NetBSD: underscore.c,v 1.8 2024/06/29 16:36:04 christos Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
