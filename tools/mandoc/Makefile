#	$NetBSD: Makefile,v 1.6 2014/01/05 19:49:57 joerg Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}mandoc
HOST_SRCDIR=	external/bsd/mdocml/bin/mandoc

NOMAN=		# defined

.include <bsd.own.mk>
.include <bsd.sys.mk>

NETBSD_MINOR!=	${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh -n
NETBSD_OSNAME=	"NetBSD ${NETBSD_MINOR}"

HOST_CPPFLAGS+=	-DOSNAME=${NETBSD_OSNAME:Q} -DHAVE_CONFIG_H -I.

DPSRCS+=	config.h

CLEANFILES+=	config.log config.h

config.h: configure config.h.pre config.h.post
	SRCDIR=${DISTDIR} ${HOST_SH} -x ${DISTDIR}/configure

.include "${.CURDIR}/../Makefile.host"
