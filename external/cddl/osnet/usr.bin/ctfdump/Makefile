#	$NetBSD: Makefile,v 1.8.14.1 2018/06/25 07:25:26 pgoyette Exp $

# $FreeBSD: head/cddl/usr.bin/ctfdump/Makefile 314654 2017-03-04 11:30:04Z ngie $

.include <bsd.own.mk>

.include	"../../Makefile.inc"
.include	"../../Makefile.ctf"

.PATH:		${OPENSOLARIS_USR_DISTDIR}/tools/ctf/dump

PROG=		ctfdump

SRCS=		dump.c \
		symbol.c \
		utils.c

COPTS.utils.c +=	-Wno-format-nonliteral

LDADD+=-lpthread
DPADD+=${LIBPTHREAD}

.include <bsd.prog.mk>
