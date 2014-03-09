#	$NetBSD: Makefile,v 1.5 2014/03/09 17:05:23 christos Exp $

# $FreeBSD: src/cddl/usr.bin/ctfdump/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $

.include	"../../Makefile.inc"
.include	"../../Makefile.ctf"

PROG=		ctfdump

SRCS=		dump.c \
		symbol.c \
		utils.c

LDADD+=-lpthread
DPADD+=${LIBPTHREAD}

.include <bsd.prog.mk>
