#	$NetBSD: Makefile.inc,v 1.2.8.2 2014/08/20 00:05:09 tls Exp $
#

UDF=	${NETBSDSRCDIR}/sys/fs/udf
UDF_NEWFS=	${NETBSDSRCDIR}/sbin/newfs_udf
FSCK=	${NETBSDSRCDIR}/sbin/fsck	# use progress meter.

.PATH:	${.CURDIR}/udf ${UDF} ${UDF_NEWFS} ${FSCK}

CPPFLAGS+= -I${UDF} -I${UDF_NEWFS} -I${FSCK}

SRCS += udf_create.c udf_write.c udf_osta.c

