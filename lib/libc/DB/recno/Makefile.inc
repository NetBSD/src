#       @(#)Makefile.inc	5.1 (Berkeley) 9/11/91

.PATH: ${.CURDIR}/DB/recno

SRCS+=	rec_close.c rec_delete.c rec_get.c rec_open.c rec_put.c rec_search.c \
	rec_seq.c rec_utils.c
