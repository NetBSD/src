#	$NetBSD: Makefile,v 1.1 2001/01/05 01:28:33 garbled Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	sushi
SRCS=	sushi.c scandir.c menutree.c scanform.c run.c functions.c \
	handlers.c blabel.c

MKMAN=	no

LIBINSTALL != cd ${.CURDIR}/../pkg_install/lib;\
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

LDADD+=	-lutil -lcurses -lcdk -lform -lm -L${LIBINSTALL} -linstall
DPADD+= ${LIBUTIL} ${LIBCURSES} ${LIBCDK} ${LIBFORM} ${LIBM} \
	${LIBINSTALL}/libinstall.a

.include <bsd.prog.mk>
