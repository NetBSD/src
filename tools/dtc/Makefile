#	$NetBSD: Makefile,v 1.4 2018/05/01 12:38:39 christos Exp $

.include <bsd.init.mk>

LIBFDT_DIR=	${.CURDIR}/../../sys/external/bsd/libfdt/dist
HOST_CPPFLAGS+=	-I${LIBFDT_DIR}

HOSTPROGNAME=	${_TOOL_PREFIX}dtc
HOST_SRCDIR=	external/gpl2/dtc/usr.bin/dtc

FDTOBJ!=	cd ${.CURDIR}/../libfdt && ${PRINTOBJDIR}
LDADD+=		-L${FDTOBJ} -lfdt

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
