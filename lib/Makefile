#	$NetBSD: Makefile,v 1.49 1998/09/14 06:39:53 ross Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM definition

SUBDIR=	csu libarch libbz2 libc libcompat libcrypt libcurses libedit libkvm \
	libl libm libntp libossaudio libpcap libposix libresolv librmt \
	librpcsvc libskey libterm libutil libwrap liby libz

# XXX Keep telnet(1) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/lib/libtelnet) || \
	defined(EXPORTABLE_SYSTEM) || make(clean) || make(cleandir) || \
	make(distclean)
SUBDIR+=	libtelnet
.endif

.include <bsd.subdir.mk>
