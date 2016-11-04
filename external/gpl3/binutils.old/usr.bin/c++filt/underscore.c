/*	$NetBSD: underscore.c,v 1.1.1.1.2.1 2016/11/04 14:44:59 pgoyette Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
