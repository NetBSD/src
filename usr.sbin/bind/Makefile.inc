# $NetBSD: Makefile.inc,v 1.1.1.1 1998/10/05 18:01:57 tron Exp $

BIND_DIR=	${.CURDIR}/..

VER!=		awk '$$1 == "VER=" { print $$2 ; exit }' ${BIND_DIR}/Makefile

NAMED=		named
PIDFILE=	/var/run/named.pid
PS=		ps

INCLUDE?=	-I. -I${BIND_DIR}/include
LIBRARY?=	-L${BIND_DIR}/lib -lbind
