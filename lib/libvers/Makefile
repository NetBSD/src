# $NetBSD: Makefile,v 1.1 2000/08/03 03:53:40 assar Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/vers

WARNS?=  1
MKLINT= no

LIB=	vers

includes: print_version.h

SRCS=	print_version.c

DPSRCS=	print_version.h

CPPFLAGS+= -I. \
	-I${.CURDIR}/../../include/heimdal \
	-I${DIST}/heimdal/lib/roken \
	-DHAVE_CONFIG_H
 
HOST_CPPFLAGS+= -I${.CURDIR}/../../include/heimdal -DHAVE_CONFIG_H

make-print-version.lo: ${.CURDIR}/../../include/heimdal/version.h

make-print-version: make-print-version.lo
	${HOST_CC} ${HOST_LDFLAGS} -o ${.TARGET} ${.ALLSRC}

print_version.h: make-print-version
	./make-print-version print_version.h

print_version.o print_version.so print_version.po print_version.So print_version.ln: print_version.h

CLEANFILES+= print_version.h make-print-version make-print-version.lo
 
.include <bsd.lib.mk>
