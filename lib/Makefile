#	$NetBSD: Makefile,v 1.24 1996/10/01 14:38:28 cgd Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM definition

SUBDIR=	csu libarch libc libcompat libcrypt libcurses libedit libl \
	libm libpcap libresolv librmt librpcsvc libskey libterm libutil \
	liby libz

# XXX Temporary until all ports are able to use libkvm (leo)
.if	(${MACHINE} == "alpha") || \
	(${MACHINE} == "amiga") || \
	(${MACHINE} == "atari") || \
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
