#	$NetBSD: Makefile,v 1.3 2004/01/03 15:05:37 lukem Exp $

NOMAN=		# defined
WARNS=2

.include <bsd.own.mk>

PROG=		symbolcheck

.if (${MKSKEY} != "no")
CPPFLAGS+=-DHAVE_SKEY_H=1
.endif

.include <bsd.prog.mk>
