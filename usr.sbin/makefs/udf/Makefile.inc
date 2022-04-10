#	$NetBSD: Makefile.inc,v 1.6 2022/04/10 09:22:35 riastradh Exp $
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
CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}
