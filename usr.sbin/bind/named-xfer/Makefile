#	$NetBSD: Makefile,v 1.1.1.1 1998/10/05 18:02:00 tron Exp $

.PATH:	${.CURDIR}/../named \
	${.CURDIR}/../man

WARNS=	0
PROG=	named-xfer
SRCS=	${PROG}.c db_glue.c ns_glue.c tmp_version.c
CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
CLEANFILES+=	tmp_version.c
MAN=	${PROG}.8

tmp_version.c: ../named/version.c Makefile ../Makefile
	(u=$${USER-root} d=`pwd` h=`${HOSTNAMECMD}` t=`date`; \
	sed -e "s|%WHEN%|$${t}|" -e "s|%VERSION%|"${VER}"|" \
	    -e "s|%WHOANDWHERE%|$${u}@$${h}:$${d}|" \
	    < ../named/version.c > tmp_version.c)

.include "../../Makefile.inc"
.include <bsd.prog.mk>
