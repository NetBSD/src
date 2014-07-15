#	$NetBSD: Makefile,v 1.1 2014/07/15 13:19:13 christos Exp $

HOSTPROGNAME=	nbvfontedpr
HOST_BINDIR=	${TOOLDIR}/libexec
HOST_SRCDIR=	usr.bin/vgrind
VGDEFDIR=	${TOOLDIR}/share/misc
HOST_CPPFLAGS=	-D_PATH_VGRINDEFS=\"${VGDEFDIR}/vgrindefs\"
FILESDIR=        ${VGDEFDIR}
FILESDIR_tmac.vgrind=${VGDEFDIR}

.include "${.CURDIR}/../Makefile.host"
