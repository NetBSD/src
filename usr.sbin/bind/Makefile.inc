# $NetBSD: Makefile.inc,v 1.8 2001/01/07 08:00:57 enami Exp $

.include <bsd.own.mk>

WARNS=0
WFORMAT=2
BIND_DIR=	${.CURDIR}/..
BIND_DIST_DIR=	${.CURDIR}/../../../dist/bind

VER!=		awk '$$1 == "VER=" { print $$2 ; exit }' ${BIND_DIR}/Makefile

NAMED=		named
PIDFILE=	/var/run/named.pid
PS=		ps

LIB_BIND!=	cd ${BIND_DIR}/lib && ${PRINTOBJDIR}
INCLUDE?=	-I${LIB_BIND} -I${.OBJDIR} -I${BIND_DIST_DIR}/include
LIBRARY?=	-L${LIB_BIND} -lbind
