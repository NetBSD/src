#	$NetBSD: Makefile,v 1.24 2002/09/18 03:54:34 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/27/95

.include <bsd.own.mk>

PROG=	mtree
#CPPFLAGS+=-DDEBUG
MAN=	mtree.8
SRCS=	compare.c crc.c create.c excludes.c misc.c mtree.c spec.c verify.c \
	getid.c stat_flags.c pack_dev.c
WARNS?=	2

.ifndef HOSTPROG
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
.endif

CPPFLAGS+=	-I${NETBSDSRCDIR}/bin/ls -I${NETBSDSRCDIR}/sbin/mknod
.PATH:		${NETBSDSRCDIR}/bin/ls ${NETBSDSRCDIR}/sbin/mknod

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
