#	$NetBSD: Makefile,v 1.8 2001/01/11 13:15:06 enami Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/named-xfer ${BIND_DIST_DIR}/bin/named \
	${BIND_DIST_DIR}/man

NAMEDOBJ!=cd ${.CURDIR}/../named && ${PRINTOBJDIR}

PROG=	named-xfer
SRCS=	${PROG}.c db_glue.c ns_glue.c tmp_version.c
CPPFLAGS+= ${INCLUDE} -I${NAMEDOBJ}
LDADD=	${LIBRARY}
CLEANFILES+=	tmp_version.c
MAN=	${PROG}.8
BINDIR=	/usr/libexec

tmp_version.c: ${BIND_DIST_DIR}/bin/named/version.c Makefile ../Makefile
	(u=$${USER-root} d=`pwd` h=`${HOSTNAMECMD}` t=`date`; \
	sed -e "s|%WHEN%|$${t}|" -e "s|%VERSION%|"${VER}"|" \
	    -e "s|%WHOANDWHERE%|$${u}@$${h}:$${d}|" \
	    < ${BIND_DIST_DIR}/bin/named/version.c > tmp_version.c)

.include "../../Makefile.inc"
.include <bsd.prog.mk>
