#	$NetBSD: Makefile.inc,v 1.5 2022/04/06 13:29:16 reinoud Exp $
#

UDF=	${NETBSDSRCDIR}/sys/fs/udf
UDF_NEWFS=	${NETBSDSRCDIR}/sbin/newfs_udf
FSCK=	${NETBSDSRCDIR}/sbin/fsck	# use progress meter.

.PATH:	${.CURDIR}/udf ${UDF} ${UDF_NEWFS} ${FSCK}

CPPFLAGS+= -I${UDF} -I${UDF_NEWFS} -I${FSCK}

SRCS += udf_core.c udf_osta.c

DPADD+=${LIBUTIL}
LDADD+=-lutil -lprop

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member -Wno-error=constant-conversion
CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}
