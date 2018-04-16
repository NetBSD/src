/*	$NetBSD: underscore.c,v 1.4.8.1 2018/04/16 01:59:27 pgoyette Exp $	*/

#ifdef __ELF__
int prepends_underscore = 0;
#else
int prepends_underscore = 1;
#endif
