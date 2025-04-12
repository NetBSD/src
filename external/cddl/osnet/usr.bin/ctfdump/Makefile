#	$NetBSD: Makefile,v 1.10 2025/04/12 10:07:53 rin Exp $

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

.if (${HOSTPROG:U} == "")
LDADD+=-lpthread
DPADD+=${LIBPTHREAD}
.endif

.include <bsd.prog.mk>
