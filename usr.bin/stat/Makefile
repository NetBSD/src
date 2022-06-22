#	$NetBSD: Makefile,v 1.12 2022/06/22 18:02:43 kre Exp $

PROG=	stat

.if !defined(HOSTPROG)
LINKS=	${BINDIR}/stat ${BINDIR}/readlink
MAN=	stat.1 readlink.1
.endif

.include <bsd.own.mk>

# Has fallthru comment that is missed
.if !defined(HOSTPROG) && defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.stat.c+=	-Wno-error=implicit-fallthrough
.endif

COPTS.stat.c += -Wno-format-nonliteral

.include <bsd.prog.mk>
