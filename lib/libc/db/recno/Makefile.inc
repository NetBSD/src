#	$NetBSD: Makefile.inc,v 1.6 2026/10/04 03:28:42 mrg Exp $
#       @(#)Makefile.inc	8.1 (Berkeley) 6/4/93

.PATH: ${.PARSEDIR}

SRCS+=	rec_close.c rec_delete.c rec_get.c rec_open.c rec_put.c rec_search.c \
	rec_seq.c rec_utils.c
