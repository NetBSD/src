# $NetBSD: Makefile,v 1.1.1.1 2000/06/16 19:27:31 thorpej Exp $
DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/heimdal/lib/com_err ${DIST}/heimdal/lib/roken

HOSTPROG= compile_et

SRCS=	compile_et.c parse.y lex.l \
	getarg.c print_version.c warnerr.c strupr.c get_window_size.c

YHEADER=	1

BINDIR=	/usr/bin

LIBROKEN!=	cd ${.CURDIR}/../../libroken && ${MAKE} print-objdir

HOST_CPPFLAGS+= -I.				\
	 -I${.CURDIR}/../../../include/heimdal	\
	 -I${LIBROKEN}				\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

NOMAN=	noman

.include <bsd.hostprog.mk>
