#	$NetBSD: Makefile,v 1.17 1996/03/10 05:45:38 thorpej Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

SUBDIR=	csu libarch libc libcompat libcrypt libcurses libedit libkvm libl \
	libm libpcap libresolv librpcsvc libskey libterm libutil liby

# XXX Keep telnet(1) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/lib/libtelnet) || defined(EXPORTABLE_SYSTEM)
SUBDIR+=libtelnet
.endif

.include <bsd.subdir.mk>
