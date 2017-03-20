#	$NetBSD: Makefile.inc,v 1.2.12.1 2017/03/20 06:58:08 pgoyette Exp $
#

UDF=	${NETBSDSRCDIR}/sys/fs/udf
UDF_NEWFS=	${NETBSDSRCDIR}/sbin/newfs_udf
FSCK=	${NETBSDSRCDIR}/sbin/fsck	# use progress meter.

.PATH:	${.CURDIR}/udf ${UDF} ${UDF_NEWFS} ${FSCK}

CPPFLAGS+= -I${UDF} -I${UDF_NEWFS} -I${FSCK}

SRCS += udf_create.c udf_write.c udf_osta.c

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member -Wno-error=constant-conversion
