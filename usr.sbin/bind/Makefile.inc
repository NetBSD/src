# $NetBSD: Makefile.inc,v 1.4 1999/11/20 19:47:10 veego Exp $

LDSTATIC?=      -static
WARNS=0

BIND_DIR=	${.CURDIR}/..
BIND_DIST_DIR=	${.CURDIR}/../../../dist/bind

VER!=		awk '$$1 == "VER=" { print $$2 ; exit }' ${BIND_DIR}/Makefile

NAMED=		named
PIDFILE=	/var/run/named.pid
PS=		ps

LIB_BIND!=	cd ${BIND_DIR}/lib; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | \
		${MAKE} -s -f -
INCLUDE?=	-I${LIB_BIND} -I${.OBJDIR} -I${BIND_DIR}/include -I${BIND_DIST_DIR}/include
LIBRARY?=	-L${LIB_BIND} -lbind

