#	$NetBSD: Makefile,v 1.10 1998/07/23 17:56:00 phil Exp $

MENUC!=cd $(.CURDIR)/..; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/menuc\n" | ${MAKE} -s -f-
${MENUC}:
	cd ${.CURDIR}/..; ${MAKE}

PROG=	testm
SRCS=	msg_defs.c menu_defs.c main.c
CFLAGS+=-g
CPPFLAGS+=-I.
WARNS=	1
LDADD+=	-lcurses -ltermcap
DPADD+=	${LIBCURSES}
NOMAN=	noman

CLEANFILES+= menu_defs.c menu_defs.h msg_defs.c msg_defs.h

menu_defs.c menu_defs.h: menus.mc ${MENUC} ${.CURDIR}/../menu_sys.def
	MENUDEF=${.CURDIR}/.. ${MENUC} ${.CURDIR}/menus.mc

msg_defs.c msg_defs.h: menus.msg
	msgc ${.CURDIR}/menus.msg

.include <bsd.prog.mk>
