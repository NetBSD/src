#	$NetBSD: Makefile.inc,v 1.7 2023/06/03 08:53:00 lukem Exp $
#

UDF=	${NETBSDSRCDIR}/sys/fs/udf
UDF_NEWFS=	${NETBSDSRCDIR}/sbin/newfs_udf
FSCK=	${NETBSDSRCDIR}/sbin/fsck	# use progress meter.

.PATH:	${.CURDIR}/udf ${UDF} ${UDF_NEWFS} ${FSCK}

CPPFLAGS+= -I${UDF} -I${UDF_NEWFS} -I${FSCK}

SRCS += udf_core.c udf_osta.c

.if !defined(HOSTPROG)
DPADD+=${LIBUTIL}
LDADD+=-lutil -lprop
.endif

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member -Wno-error=constant-conversion
CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
