#	$NetBSD: Makefile,v 1.43 1997/10/16 20:28:49 augustss Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM definition

SUBDIR=	csu libarch libc libcompat libcrypt libcurses libedit libkvm \
	libl libm libntp libossaudio libpcap libposix libresolv librmt \
	librpcsvc libskey libterm libutil libwrap liby libz

# XXX Temporary until these ports are able to use libkvm.
.if	(${MACHINE} == "i386")   || \
	(${MACHINE} == "pica")   || \
	(${MACHINE} == "pmax")   || \
	(${MACHINE} == "x68k")
SUBDIR+=        libkvm.old
.endif

# XXX Keep telnet(1) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/lib/libtelnet) || \
	defined(EXPORTABLE_SYSTEM) || make(clean) || make(cleandir)
SUBDIR+=	libtelnet
.endif

.include <bsd.subdir.mk>
