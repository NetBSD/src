#	$NetBSD: Makefile,v 1.19 1996/05/05 04:37:22 gwr Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

SUBDIR=	csu libarch libc libcompat libcrypt libcurses libedit libl \
	libm libpcap libresolv librpcsvc libskey libterm libutil liby

# XXX Temporarely until all ports are able to use libkvm (leo)
.if	(${MACHINE} == "atari") || \
	(${MACHINE} == "sun3")
SUBDIR+=        libkvm
.else
SUBDIR+=        libkvm.old
.endif

# XXX Keep telnet(1) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/lib/libtelnet) || defined(EXPORTABLE_SYSTEM)
SUBDIR+=libtelnet
.endif

.include <bsd.subdir.mk>
