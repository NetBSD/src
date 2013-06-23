# $NetBSD: Makefile.inc,v 1.1.6.2 2013/06/23 06:26:22 tls Exp $

TOPDIR=		${NETBSDSRCDIR}/external/bsd/atf
SRCDIR=		${TOPDIR}/dist

ATF_VERSION!=	grep 'define VERSION' ${TOPDIR}/lib/libatf-c/bconfig.h \
		| cut -d '"' -f 2
