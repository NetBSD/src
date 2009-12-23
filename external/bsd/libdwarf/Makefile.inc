#	$NetBSD: Makefile.inc,v 1.1 2009/12/23 00:10:15 darran Exp $

.include <bsd.own.mk>

LIBDWARF_DIR=	${NETBSDSRCDIR}/external/bsd/libdwarf/dist

CPPFLAGS+=	-I${LIBDWARF_DIR}

WARNS?=		4

.PATH:		${LIBDWARF_DIR}
