# $NetBSD: Makefile.inc,v 1.22.6.1 2009/05/13 19:20:21 jym Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server

DIST=	${NETBSDSRCDIR}/dist/dhcp
.PATH:	${DIST}/${DHCPSRCDIR}

COBJDIR!=cd $(.CURDIR)/../common && ${PRINTOBJDIR}
OMOBJDIR!=cd $(.CURDIR)/../omapip && ${PRINTOBJDIR}
MROBJDIR!=cd $(.CURDIR)/../minires && ${PRINTOBJDIR}
DSTOBJDIR!=cd $(.CURDIR)/../dst && ${PRINTOBJDIR}

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif

CPPFLAGS+= -I${DIST} -I${DIST}/includes -Wno-unused
LDADD+= ${COBJDIR}/libdhcp.a ${MROBJDIR}/libminires.a
LDADD+= ${OMOBJDIR}/libomapi.a ${DSTOBJDIR}/libdst.a
LDADD+= -lipsec
DPADD+= ${COBJDIR}/libdhcp.a ${MROBJDIR}/libminires.a
DPADD+= ${OMOBJDIR}/libomapi.a ${DSTOBJDIR}/libdst.a
DPADD+= ${LIBIPSEC}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
