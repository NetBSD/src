#	$NetBSD: Makefile,v 1.5.6.1 2014/05/22 11:42:34 yamt Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}mandoc
HOST_SRCDIR=	external/bsd/mdocml/bin/mandoc

NOMAN=		# defined

.include <bsd.own.mk>

NETBSD_MINOR!=	${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh -n
NETBSD_OSNAME=	"NetBSD ${NETBSD_MINOR}"

HOST_CPPFLAGS+=	-DOSNAME=${NETBSD_OSNAME:Q} -DHAVE_CONFIG_H -I.

DPSRCS+=	config.h

CLEANFILES+=	config.log config.h

config.h: configure config.h.pre config.h.post
	SRCDIR=${DISTDIR} ${HOST_SH} -x ${DISTDIR}/configure

.include "${.CURDIR}/../Makefile.host"
