#	$NetBSD: Makefile,v 1.2 2001/01/05 19:01:59 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=	sushi
SRCS=	sushi.c scandir.c menutree.c scanform.c run.c functions.c \
	handlers.c blabel.c

MKMAN=	no

LIBINSTALL != cd ${.CURDIR}/../pkg_install/lib && ${PRINTOBJDIR}

LDADD+=	-lutil -lcurses -lcdk -lform -lm -L${LIBINSTALL} -linstall
DPADD+= ${LIBUTIL} ${LIBCURSES} ${LIBCDK} ${LIBFORM} ${LIBM} \
	${LIBINSTALL}/libinstall.a

.include <bsd.prog.mk>
