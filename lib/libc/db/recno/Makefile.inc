#	$NetBSD: Makefile.inc,v 1.5.132.1 2026/11/04 23:19:16 perseant Exp $
#       @(#)Makefile.inc	8.1 (Berkeley) 6/4/93

.PATH: ${.PARSEDIR}

SRCS+=	rec_close.c rec_delete.c rec_get.c rec_open.c rec_put.c rec_search.c \
	rec_seq.c rec_utils.c
