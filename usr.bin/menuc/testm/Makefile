#	$NetBSD: Makefile,v 1.3 1997/10/11 11:49:36 mycroft Exp $

MENUC!=cd $(.CURDIR)/..; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/menuc\n" | ${MAKE} -s -f-
${MENUC}:
	cd ${.CURDIR}/..; ${MAKE}

PROG=	testm
SRCS=	menu_defs.c main.c
CFLAGS+=-I.
WARNS=	1
LDADD+=	-lcurses -ltermcap
DPADD+=	${LIBCURSES} ${LIBTERMCAP}
NOMAN=	noman

CLEANFILES+= menu_defs.c menu_defs.h

menu_defs.c menu_defs.h: menus.mc ${MENUC}
	MENUDEF=${.CURDIR}/.. ${MENUC} ${.CURDIR}/menus.mc

.include <bsd.prog.mk>
