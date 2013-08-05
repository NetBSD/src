#	$NetBSD: Makefile.inc,v 1.1 2013/08/05 14:41:57 reinoud Exp $
#

SYSFS=	${NETBSDSRCDIR}/sys/fs
UDF=	${SYSFS}/udf
UDF_NEWFS=	${NETBSDSRCDIR}/sbin/newfs_udf
FSCK=	${NETBSDSRCDIR}/sbin/fsck	# use progress meter.

.PATH:	${.CURDIR}/udf ${UDF} ${UDF_NEWFS} ${FSCK}

CPPFLAGS+= -I${UDF} -I${UDF_NEWFS} -I${FSCK}

SRCS += udf_create.c udf_write.c udf_osta.c

#SRCS += main.c		# newfs
#.if !defined(HOSTPROG)
#SRCS += progress.c	# progress bar (fsck)
#.endif

#SRCS += v7fs_estimate.c v7fs_populate.c
