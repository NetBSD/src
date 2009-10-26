#	$NetBSD: Makefile,v 1.2 2009/10/26 21:17:19 joerg Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}mandoc
HOST_SRCDIR=	external/bsd/mdocml/bin/mandoc

.include <bsd.own.mk>

NETBSD_MINOR!=	${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh -n
NETBSD_OSNAME=	"NetBSD ${NETBSD_MINOR}"

HOST_CPPFLAGS+=	-DOSNAME=${NETBSD_OSNAME:Q}

.include "${.CURDIR}/../Makefile.host"
