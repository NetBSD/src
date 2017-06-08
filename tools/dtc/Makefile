#	$NetBSD: Makefile,v 1.2 2017/06/08 17:16:44 skrll Exp $

NOMAN=	# defined

.include <bsd.own.mk>

LIBFDT_DIR=	${.CURDIR}/../../sys/external/bsd/libfdt/dist
HOST_CPPFLAGS+=	-I${LIBFDT_DIR}

HOSTPROGNAME=	${_TOOL_PREFIX}dtc
HOST_SRCDIR=	external/gpl2/dtc/usr.bin/dtc

FDTOBJ!=	cd ${.CURDIR}/../libfdt && ${PRINTOBJDIR}
LDADD+=		-L${FDTOBJ} -lfdt

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
