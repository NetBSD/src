#	$NetBSD: Makefile,v 1.6 1998/06/24 14:46:54 phil Exp $

MENUC!=cd $(.CURDIR)/..; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/menuc\n" | ${MAKE} -s -f-
${MENUC}:
	cd ${.CURDIR}/..; ${MAKE}

PROG=	testm
SRCS=	menu_defs.c main.c
CPPFLAGS+=-I.
WARNS=	1
LDADD+=	-lcurses -ltermcap
DPADD+=	${LIBCURSES}
NOMAN=	noman

CLEANFILES+= menu_defs.c menu_defs.h

menu_defs.c menu_defs.h: menus.mc ${MENUC}
	MENUDEF=${.CURDIR}/.. ${MENUC} ${.CURDIR}/menus.mc

.include <bsd.prog.mk>
