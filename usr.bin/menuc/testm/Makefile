#	$NetBSD: Makefile,v 1.5 1998/02/18 22:49:45 jtc Exp $

MENUC!=cd $(.CURDIR)/..; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/menuc\n" | ${MAKE} -s -f-
${MENUC}:
	cd ${.CURDIR}/..; ${MAKE}

PROG=	testm
SRCS=	menu_defs.c main.c
CPPFLAGS+=-I.
WARNS=	1
LDADD+=	-lcurses
DPADD+=	${LIBCURSES}
NOMAN=	noman

CLEANFILES+= menu_defs.c menu_defs.h

menu_defs.c menu_defs.h: menus.mc ${MENUC}
	MENUDEF=${.CURDIR}/.. ${MENUC} ${.CURDIR}/menus.mc

.include <bsd.prog.mk>
