#	$NetBSD: Makefile,v 1.32.6.2 2014/05/22 11:43:07 yamt Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/27/95

.include <bsd.own.mk>

PROG=	mtree
#CPPFLAGS+=-DDEBUG
CPPFLAGS+= -DMTREE
MAN=	mtree.8
SRCS=	compare.c crc.c create.c excludes.c misc.c mtree.c spec.c specspec.c \
	verify.c getid.c pack_dev.c only.c
.if (${HOSTPROG:U} == "")
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

CPPFLAGS+=	-I${NETBSDSRCDIR}/sbin/mknod
.PATH:		${NETBSDSRCDIR}/sbin/mknod

.include <bsd.prog.mk>
