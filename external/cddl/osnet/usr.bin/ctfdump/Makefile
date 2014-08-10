#	$NetBSD: Makefile,v 1.6.2.1 2014/08/10 06:50:28 tls Exp $

# $FreeBSD: src/cddl/usr.bin/ctfdump/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include <bsd.own.mk>

.include	"../../Makefile.inc"
.include	"../../Makefile.ctf"

.PATH:		${OPENSOLARIS_USR_DISTDIR}/tools/ctf/dump

PROG=		ctfdump

SRCS=		dump.c \
		symbol.c \
		utils.c

LDADD+=-lpthread
DPADD+=${LIBPTHREAD}

.include <bsd.prog.mk>
