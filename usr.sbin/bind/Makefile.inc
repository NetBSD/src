# $NetBSD: Makefile.inc,v 1.10 2001/02/11 06:45:59 itojun Exp $

.include <bsd.own.mk>

WARNS=0
WFORMAT=2
BIND_DIR=	${.CURDIR}/..
BIND_DIST_DIR=	${.CURDIR}/../../../dist/bind
BIND_CRYPTO_DIST_DIR=	${.CURDIR}/../../../crypto/dist/bind

VER!=		cat ${BIND_DIST_DIR}/Version

NAMED=		named
PIDFILE=	/var/run/named.pid
PS=		ps

LIB_BIND!=	cd ${BIND_DIR}/lib && ${PRINTOBJDIR}
INCLUDE?=	-I${LIB_BIND} -I${.OBJDIR} -I${BIND_DIST_DIR}/include
LIBRARY?=	-L${LIB_BIND} -lbind
