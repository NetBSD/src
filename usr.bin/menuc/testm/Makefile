#	$NetBSD: Makefile,v 1.12 2000/12/30 14:54:40 sommerfeld Exp $

MENUCDIR!=	cd $(.CURDIR)/..; ${PRINTOBJDIR}
MENUC=	${MENUCDIR}/menuc
${MENUC}:
	cd ${.CURDIR}/..; ${MAKE}

PROG=	testm
SRCS=	msg_defs.c menu_defs.c main.c
CFLAGS+=-g
CPPFLAGS+=-I.
WARNS=	1
LDADD+=	-lcurses -ltermcap
DPADD+=	${LIBCURSES}
MKMAN=	no

CLEANFILES+= menu_defs.c menu_defs.h msg_defs.c msg_defs.h

menu_defs.c menu_defs.h: menus.mc ${MENUC} ${.CURDIR}/../menu_sys.def
	MENUDEF=${.CURDIR}/.. ${MENUC} ${.CURDIR}/menus.mc

msg_defs.c msg_defs.h: menus.msg
	msgc ${.CURDIR}/menus.msg

.include <bsd.prog.mk>
