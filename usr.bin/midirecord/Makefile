#	$NetBSD: Makefile,v 1.1.2.2 2015/01/12 10:22:22 martin Exp $

PROG=	midirecord

.include <bsd.own.mk>

LIBAUDIO != cd ${.CURDIR}/../audio/common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../audio/common
DPADD+=	${LIBAUDIO}/libaudio.a
LDADD+=	-L${LIBAUDIO} -laudio

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
