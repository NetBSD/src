#	$NetBSD: Makefile,v 1.15 2025/03/26 18:41:53 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}mandoc
HOST_SRCDIR=	external/bsd/mdocml/bin/mandoc

.include <bsd.hostinit.mk>

NETBSD_MINOR!=	${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh -n
NETBSD_OSNAME=	"NetBSD ${NETBSD_MINOR}"

HOST_CPPFLAGS+=	-DOSNAME=${NETBSD_OSNAME:Q} -DHAVE_CONFIG_H -I.

DPSRCS+=	config.h

CLEANFILES+=	config.log config.h Makefile.local

config.h: configure 
	SRCDIR=${DISTDIR} CC=${HOST_CC} ${HOST_SH} ${DISTDIR}/configure

.include "${.CURDIR}/../Makefile.host"
