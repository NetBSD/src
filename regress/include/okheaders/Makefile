#	$NetBSD: Makefile,v 1.4 2004/01/04 00:25:52 lukem Exp $

NOMAN=		# defined
WARNS=2

.include <bsd.own.mk>

PROG=		symbolcheck

.if (${MKCRYPTO} != "no")
CPPFLAGS+=-DHAVE_DES_H=1
.endif

.if (${MKSKEY} != "no")
CPPFLAGS+=-DHAVE_SKEY_H=1
.endif

.include <bsd.prog.mk>
