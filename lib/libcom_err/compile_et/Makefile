# $NetBSD: Makefile,v 1.15 2002/09/13 18:30:43 thorpej Exp $

NOMAN=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist

.PATH: ${DIST}/heimdal/lib/com_err ${DIST}/heimdal/lib/roken ${DIST}/heimdal/lib/vers

HOSTPROG= compile_et

SRCS+=	compile_et.c parse.y lex.l \
	getarg.c print_version.c warnerr.c strupr.c get_window_size.c

YHEADER=	# defined

BINDIR=		/usr/bin

LIBROKEN!=	cd ${NETBSDSRCDIR}/lib/libroken && ${PRINTOBJDIR}

HOST_CPPFLAGS+= -I.				\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${LIBROKEN}				\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DKRB4=1 -DKRB5=1			\
	 -DHAVE_CONFIG_H

DPSRCS=		print_version.h

make-print-version.lo: ${NETBSDSRCDIR}/include/heimdal/version.h

make-print-version: make-print-version.lo
	${HOST_CC} ${HOST_LDFLAGS} -o ${.TARGET} ${.ALLSRC}

print_version.h: make-print-version
	./make-print-version print_version.h

CLEANFILES+=	print_version.h make-print-version make-print-version.lo

.include <bsd.hostprog.mk>
