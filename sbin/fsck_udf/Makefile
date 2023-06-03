#	$NetBSD: Makefile,v 1.3 2023/06/03 21:26:27 lukem Exp $

.include <bsd.own.mk>

PROG=	fsck_udf
MAN=	fsck_udf.8
SRCS=	main.c udf_core.c \
	udf_osta.c fattr.c fsutil.c

FSCK=	${NETBSDSRCDIR}/sbin/fsck
NEWFS=	${NETBSDSRCDIR}/sbin/newfs_udf
MOUNT=	${NETBSDSRCDIR}/sbin/mount
KUDF=	${NETBSDSRCDIR}/sys/fs/udf
CPPFLAGS+= -I${FSCK} -I${KUDF} -I${NEWFS} -I${NETBSDSRCDIR}/sys
.PATH:	${FSCK} ${NEWFS} ${MOUNT} ${KUDF}

DPADD+=${LIBUTIL}
LDADD+=-lutil -lprop

CWARNFLAGS+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
