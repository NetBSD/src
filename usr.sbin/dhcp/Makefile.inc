# $NetBSD: Makefile.inc,v 1.10.4.3 2001/08/16 18:53:26 tv Exp $

WARNS?=	0

.include <bsd.own.mk>

DIST=	${.CURDIR}/../../../dist/dhcp
.PATH:	${DIST}/${DHCPSRCDIR}

COBJDIR!=cd $(.CURDIR)/../common && ${PRINTOBJDIR}
OMOBJDIR!=cd $(.CURDIR)/../omapip && ${PRINTOBJDIR}
MROBJDIR!=cd $(.CURDIR)/../minires && ${PRINTOBJDIR}
DSTOBJDIR!=cd $(.CURDIR)/../dst && ${PRINTOBJDIR}

CPPFLAGS+= -I${DIST} -I${DIST}/includes
LDADD+= ${COBJDIR}/libdhcp.a ${MROBJDIR}/libminires.a
LDADD+= ${OMOBJDIR}/libomapi.a ${DSTOBJDIR}/libdst.a
DPADD+= ${COBJDIR}/libdhcp.a ${MROBJDIR}/libminires.a
DPADD+= ${OMOBJDIR}/libomapi.a ${DSTOBJDIR}/libdst.a

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
