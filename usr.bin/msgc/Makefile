#	$NetBSD: Makefile,v 1.14 2003/06/04 19:00:26 dsl Exp $

.include <bsd.own.mk>

PROG=		msgc
SRCS=		msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c
.PATH:		${NETBSDSRCDIR}/usr.bin/menuc
CPPFLAGS+=	-I. -I${.CURDIR}
YHEADER=

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
		msgc.1 msg_prompt_win.1 \
		msgc.1 msg_prompt_noecho.1 \
		msgc.1 msg_table_add.1

.if ${MKSHARE} != "no"
FILES=		msg_sys.def
FILESDIR=	/usr/share/misc
.endif

.ifndef HOSTPROG
LDADD+=		-ll
DPADD+=		${LIBL}
.endif

.include <bsd.prog.mk>
