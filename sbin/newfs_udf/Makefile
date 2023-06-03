# $NetBSD: Makefile,v 1.9 2023/06/03 08:52:55 lukem Exp $

.include <bsd.own.mk>

PROG=	newfs_udf
MAN=	newfs_udf.8
SRCS=	newfs_udf.c udf_core.c \
	udf_osta.c fattr.c

MOUNT=  ${NETBSDSRCDIR}/sbin/mount
KUDF=	${NETBSDSRCDIR}/sys/fs/udf
CPPFLAGS+= -I${MOUNT} -I${KUDF} -I${NETBSDSRCDIR}/sys
.PATH:  ${MOUNT} ${KUDF}

DPADD+=${LIBUTIL}
LDADD+=-lutil -lprop

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member
CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
