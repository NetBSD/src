#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/4/93
#	$Id: Makefile.inc,v 1.3 1993/08/01 05:41:25 mycroft Exp $

.PATH: ${.CURDIR}/db/btree

SRCS+=	bt_close.c bt_conv.c bt_debug.c bt_delete.c bt_get.c bt_open.c \
	bt_overflow.c bt_page.c bt_put.c bt_search.c bt_seq.c bt_split.c \
	bt_stack.c bt_utils.c
