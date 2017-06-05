#	$NetBSD: Makefile,v 1.1 2017/06/05 22:44:40 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROGNAME=	${_TOOL_PREFIX}dtc
HOST_SRCDIR=	external/gpl2/dtc/usr.bin/dtc

FDTOBJ!=	cd ${.CURDIR}/../libfdt && ${PRINTOBJDIR}
LDADD+=		-L${FDTOBJ} -lfdt

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
