#	$NetBSD: Makefile,v 1.8 2000/07/06 19:30:11 jhawk Exp $

.include <bsd.own.mk>

PROG=	msgc
SRCS=	msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c
.PATH: ${.CURDIR}/../menuc
CPPFLAGS+=-I. -I${.CURDIR}
YHEADER=1
LDADD+=	-ll
DPADD+=	${LIBL}

MLINKS+=	msgc.1 msg_window.1 \
		msgc.1 msg_window.1 \
		msgc.1 msg_string.1 \
		msgc.1 msg_clear.1 \
		msgc.1 msg_standout.1 \
		msgc.1 msg_standend.1 \
		msgc.1 msg_display.1 \
		msgc.1 msg_display_add.1 \
		msgc.1 msg_prompt.1 \
		msgc.1 msg_prompt_add.1 \
		msgc.1 msg_prompt_noecho.1 \
		msgc.1 msg_table_add.1

.if ${MKSHARE} != "no"
FILES= msg_sys.def
FILESDIR= /usr/share/misc
.endif

.include <bsd.prog.mk>
