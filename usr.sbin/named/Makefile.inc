#	$NetBSD: Makefile.inc,v 1.3 1997/10/18 04:05:38 lukem Exp $
# from: $Id: Makefile.inc,v 8.3 1995/12/31 23:28:00 vixie Exp 

WARNS=		0
BIND_DIR=	${.CURDIR}/..

VER!=		awk -F' *= *' '$$1 == "VER" { print $$2 ; exit }' \
		${BIND_DIR}/Makefile

PIDDIR=		/var/run
PS=		ps
IOT=		ABRT

CONFIG?=	-DUSE_OPTIONS_H
INCLUDE?=	-I. -I${BIND_DIR} -I${BIND_DIR}/include
