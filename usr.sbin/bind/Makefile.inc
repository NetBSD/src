# $NetBSD: Makefile.inc,v 1.3 1999/01/04 00:04:04 tron Exp $

BIND_DIR=	${.CURDIR}/..

VER!=		awk '$$1 == "VER=" { print $$2 ; exit }' ${BIND_DIR}/Makefile

NAMED=		named
PIDFILE=	/var/run/named.pid
PS=		ps

LIB_BIND!=	cd ${BIND_DIR}/lib; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | \
		${MAKE} -s -f -
INCLUDE?=	-I. -I${BIND_DIR}/include
LIBRARY?=	-L${LIB_BIND} -lbind
