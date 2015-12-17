#	$NetBSD: Makefile,v 1.8 2015/12/17 22:31:49 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}mandoc
HOST_SRCDIR=	external/bsd/mdocml/bin/mandoc

NOMAN=		# defined

.include <bsd.own.mk>

NETBSD_MINOR!=	${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh -n
NETBSD_OSNAME=	"NetBSD ${NETBSD_MINOR}"

HOST_CPPFLAGS+=	-DOSNAME=${NETBSD_OSNAME:Q} -DHAVE_CONFIG_H -I.

DPSRCS+=	config.h

CLEANFILES+=	config.log config.h

config.h: configure 
	SRCDIR=${DISTDIR} ${HOST_SH} ${DISTDIR}/configure

.include "${.CURDIR}/../Makefile.host"
