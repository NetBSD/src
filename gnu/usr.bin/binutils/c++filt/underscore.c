/*	$NetBSD: underscore.c,v 1.1.2.2 1998/11/07 00:57:04 cgd Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
