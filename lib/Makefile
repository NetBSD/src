#	$NetBSD: Makefile,v 1.31 1997/04/09 21:27:15 thorpej Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM definition

SUBDIR=	csu libarch libc libcompat libcrypt libcurses libedit libl \
	libm libpcap libresolv librmt librpcsvc libskey libterm libutil \
	libwrap liby libz

.if	make(clean) || make(cleandir)
SUBDIR+=	libkvm libkvm.old libtelnet
.else

# XXX Temporary until all ports are able to use libkvm (leo)
.if	(${MACHINE} == "alpha") || \
	(${MACHINE} == "amiga") || \
	(${MACHINE} == "atari") || \
	(${MACHINE} == "hp300") || \
	(${MACHINE} == "pc532") || \
	(${MACHINE} == "sparc") || \
	(${MACHINE} == "sun3")  || \
	(${MACHINE} == "sun3x") || \
	(${MACHINE} == "powerpc")
SUBDIR+=        libkvm
.else
SUBDIR+=        libkvm.old
.endif

# XXX Keep telnet(1) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/lib/libtelnet) || defined(EXPORTABLE_SYSTEM)
SUBDIR+=	libtelnet
.endif

.endif	# make(clean) || make(cleandir)

.include <bsd.subdir.mk>
