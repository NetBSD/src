#	$NetBSD: Makefile,v 1.9 2019/02/04 09:51:47 mrg Exp $

PROG=	stat

.if !defined(HOSTPROG)
LINKS=	${BINDIR}/stat ${BINDIR}/readlink
MLINKS=	stat.1 readlink.1
.endif

.include <bsd.own.mk>

# Has fallthru comment that is missed
.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.stat.c+=	-Wno-error=implicit-fallthrough
.endif

COPTS.stat.c += -Wno-format-nonliteral

.include <bsd.prog.mk>
