#	$NetBSD: Makefile,v 1.4 1997/10/22 01:53:55 lukem Exp $

MENUC!=cd $(.CURDIR)/..; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/menuc\n" | ${MAKE} -s -f-
${MENUC}:
	cd ${.CURDIR}/..; ${MAKE}

PROG=	testm
SRCS=	menu_defs.c main.c
CPPFLAGS+=-I.
WARNS=	1
LDADD+=	-lcurses -ltermcap
DPADD+=	${LIBCURSES} ${LIBTERMCAP}
NOMAN=	noman

CLEANFILES+= menu_defs.c menu_defs.h

menu_defs.c menu_defs.h: menus.mc ${MENUC}
	MENUDEF=${.CURDIR}/.. ${MENUC} ${.CURDIR}/menus.mc

.include <bsd.prog.mk>
