# $NetBSD: Makefile.inc,v 1.6.4.2 2002/07/01 17:17:05 he Exp $

.include <bsd.own.mk>

WARNS=0
WFORMAT=2
BIND_DIR=	${.CURDIR}/..
BIND_DIST_DIR=	${.CURDIR}/../../../dist/bind
BIND_CRYPTO_DIST_DIR=	${.CURDIR}/../../../crypto/dist/bind

VER0!=		cat ${BIND_DIST_DIR}/Version
.if 0 # ${MKCRYPTO} == "yes"
VER=		${VER0}
.else
VER=		${VER0}-NOESW
.endif

NAMED=		named
PIDFILE=	/var/run/named.pid
PS=		ps

LIB_BIND!=	cd ${BIND_DIR}/lib && ${PRINTOBJDIR}
INCLUDE?=	-I${LIB_BIND} -I${.OBJDIR} -I${BIND_DIST_DIR}/include
LIBRARY?=	-L${LIB_BIND} -lbind
