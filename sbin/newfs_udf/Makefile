# $NetBSD: Makefile,v 1.3.16.1 2013/07/23 21:07:31 riastradh Exp $

.include <bsd.own.mk>

PROG=	newfs_udf
MAN=	newfs_udf.8
SRCS=	newfs_udf.c udf_create.c udf_write.c udf_osta.c fattr.c

MOUNT=  ${NETBSDSRCDIR}/sbin/mount
KUDF=	${NETBSDSRCDIR}/sys/fs/udf
CPPFLAGS+= -I${MOUNT} -I${KUDF} -I${NETBSDSRCDIR}/sys
.PATH:  ${MOUNT} ${KUDF}

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
