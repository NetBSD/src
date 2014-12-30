#	$NetBSD: Makefile,v 1.1 2014/12/30 04:14:25 mrg Exp $

PROG=	midirecord

.include <bsd.own.mk>

LIBAUDIO != cd ${.CURDIR}/../audio/common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../audio/common
DPADD+=	${LIBAUDIO}/libaudio.a
LDADD+=	-L${LIBAUDIO} -laudio

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
