# $NetBSD: Makefile,v 1.4 2001/12/12 00:51:06 tv Exp $

DIST=		${.CURDIR}/../../crypto/dist
.PATH:		${DIST}/heimdal/lib/vers

MKLINT=		# defined

LIB=		vers

NOLINKLIB=	# defined
NOPIC=		# defined
NOPROFILE=	# defined

includes: print_version.h

SRCS=		print_version.c
DPSRCS=		print_version.h

CPPFLAGS+= -I. \
	-I${.CURDIR}/../../include/heimdal \
	-I${DIST}/heimdal/lib/roken \
	-DHAVE_CONFIG_H
 
HOST_CPPFLAGS+=	-I${.CURDIR}/../../include/heimdal -DHAVE_CONFIG_H

make-print-version.lo: ${.CURDIR}/../../include/heimdal/version.h

make-print-version: make-print-version.lo
	${HOST_CC} ${HOST_LDFLAGS} -o ${.TARGET} ${.ALLSRC}

print_version.h: make-print-version
	./make-print-version print_version.h

print_version.o print_version.ln: print_version.h

CLEANFILES+=	print_version.h make-print-version make-print-version.lo
 
.include <bsd.lib.mk>
