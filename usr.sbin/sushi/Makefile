#	$NetBSD: Makefile,v 1.6 2001/01/16 02:34:14 cgd Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=	sushi
SRCS=	sushi.c scandir.c menutree.c scanform.c run.c functions.c \
	handlers.c blabel.c

MAN=	sushi.8

LIBINSTALL != cd ${.CURDIR}/../pkg_install/lib && ${PRINTOBJDIR}

LDADD+=	-lutil -lcdk -lform -lcurses -lm -L${LIBINSTALL} -linstall
DPADD+= ${LIBUTIL} ${LIBCDK} ${LIBFORM} ${LIBCURSES} ${LIBM} \
	${LIBINSTALL}/libinstall.a

NLS=	C.msg

.include <bsd.prog.mk>
