#	$NetBSD: Makefile,v 1.10 1997/10/08 09:07:29 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	rmail
MAN=	rmail.8
# we need the link because of all the braindead UUCPs in the world that
# assume that rmail is in /bin -- sigh...
SYMLINKS=	/usr/libexec/rmail /bin/rmail

.include <bsd.prog.mk>
