# $NetBSD: Makefile,v 1.2 2009/01/14 20:38:53 he Exp $

.include <bsd.own.mk>

WARNS=	4
PROG=	newfs_udf
MAN=	newfs_udf.8
SRCS=	newfs_udf.c udf_create.c udf_osta.c fattr.c

MOUNT=  ${NETBSDSRCDIR}/sbin/mount
KUDF=	${NETBSDSRCDIR}/sys/fs/udf
CPPFLAGS+= -I${MOUNT} -I${KUDF} -I${NETBSDSRCDIR}/sys
.PATH:  ${MOUNT} ${KUDF}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
