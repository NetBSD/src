# $NetBSD: Makefile.inc,v 1.19 2003/09/25 01:02:03 mycroft Exp $

WARNS?=	1

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist/dhcp
.PATH:	${DIST}/${DHCPSRCDIR}

COBJDIR!=cd $(.CURDIR)/../common && ${PRINTOBJDIR}
OMOBJDIR!=cd $(.CURDIR)/../omapip && ${PRINTOBJDIR}
MROBJDIR!=cd $(.CURDIR)/../minires && ${PRINTOBJDIR}
DSTOBJDIR!=cd $(.CURDIR)/../dst && ${PRINTOBJDIR}

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
