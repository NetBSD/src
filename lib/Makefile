#	$NetBSD: Makefile,v 1.18 1996/03/16 10:23:29 leo Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

SUBDIR=	csu libarch libc libcompat libcrypt libcurses libedit libl \
	libm libpcap libresolv librpcsvc libskey libterm libutil liby

# XXX Temporarely until all ports are able to use libkvm (leo)
.if	(${MACHINE} == "atari")
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
