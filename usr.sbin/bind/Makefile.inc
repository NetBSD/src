# $NetBSD: Makefile.inc,v 1.2 1998/10/09 19:30:02 tron Exp $

BIND_DIR=	${.CURDIR}/..

VER!=		awk '$$1 == "VER=" { print $$2 ; exit }' ${BIND_DIR}/Makefile

NAMED=		named
PIDFILE=	/var/run/named.pid
PS=		ps

LIB_BIND!=	cd ${BIND_DIR}/lib; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | \
		${MAKE} -s -f -
INCLUDE?=	-I. -I${BIND_DIR}/include
LIBRARY?=	-L${LIB_BIND}/ -lbind
