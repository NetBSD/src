#	$NetBSD: Makefile,v 1.4 2001/01/08 08:24:17 garbled Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=	sushi
SRCS=	sushi.c scandir.c menutree.c scanform.c run.c functions.c \
	handlers.c blabel.c

MAN=	sushi.8

LIBINSTALL != cd ${.CURDIR}/../pkg_install/lib && ${PRINTOBJDIR}

LDADD+=	-lutil -lcurses -lcdk -lform -lm -L${LIBINSTALL} -linstall
DPADD+= ${LIBUTIL} ${LIBCURSES} ${LIBCDK} ${LIBFORM} ${LIBM} \
	${LIBINSTALL}/libinstall.a

NLS=	C.msg

.include <bsd.prog.mk>
.include <bsd.nls.mk>
