# $NetBSD: Makefile,v 1.8 2000/12/30 17:21:46 sommerfeld Exp $

NOMAN=	noman

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../crypto/dist

# uncommon library functions
.PATH: ${.CURDIR}/../../libc/string
SRCS+=	strlcpy.c strlcat.c

.PATH: ${DIST}/heimdal/lib/com_err ${DIST}/heimdal/lib/roken

HOSTPROG= compile_et

SRCS+=	compile_et.c parse.y lex.l \
	getarg.c print_version.c warnerr.c strupr.c get_window_size.c

YHEADER=	1

BINDIR=	/usr/bin

LIBROKEN!=	cd ${.CURDIR}/../../libroken && ${PRINTOBJDIR}

HOST_CPPFLAGS+= -I.				\
	 -I${.CURDIR}/../../../include/heimdal	\
	 -I${LIBROKEN}				\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

DPSRCS=	print_version.h

make-print-version.lo: ${.CURDIR}/../../../include/heimdal/version.h

make-print-version: make-print-version.lo
	${HOST_CC} ${HOST_LDFLAGS} -o ${.TARGET} ${.ALLSRC}

print_version.h: make-print-version
	./make-print-version print_version.h

CLEANFILES+=	print_version.h make-print-version make-print-version.lo

.include <bsd.hostprog.mk>
