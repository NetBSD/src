# $NetBSD: Makefile,v 1.3 2023/06/20 17:23:03 christos Exp $

LIBISPRIVATE=	yes
NOLINKLIB=	# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/lib/vers

LIB=		vers

includes: print_version.h

SRCS=		print_version.c
DPSRCS=		print_version.h
 
HOST_CPPFLAGS+=	-I${HEIMBASE}/include -DHAVE_CONFIG_H

make-print-version.lo: ${HEIMBASE}/include/version.h

make-print-version: make-print-version.lo
	${HOST_CC} ${HOST_LDFLAGS} -o ${.TARGET} ${.ALLSRC}

print_version.h: make-print-version
	${_MKTARGET_CREATE}
	./make-print-version print_version.h

print_version.o print_version.ln: print_version.h

CLEANFILES+=	print_version.h make-print-version make-print-version.lo
 
.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.lib.mk>
