# $NetBSD: Makefile.inc,v 1.1 2016/01/14 04:22:38 christos Exp $

.include <bsd.own.mk>

BINDIR=/usr/bin
DIST=${NETBSDSRCDIR}/external/gpl2/rcs
IDIST=${DIST}/dist

PROGDPLIBS+=	rcs ${DIST}/lib
CPPFLAGS+=	-I${DIST}/include

.PATH: ${IDIST}/src ${IDIST}/man
