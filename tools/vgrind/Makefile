#	$NetBSD: Makefile,v 1.2 2014/07/15 16:04:35 christos Exp $

HOSTPROGNAME=	nbvfontedpr
HOST_BINDIR=	${TOOLDIR}/libexec
HOST_SRCDIR=	usr.bin/vgrind
HOST_FILESDIR=	${TOOLDIR}/share/misc
HOST_CPPFLAGS=	-D_PATH_VGRINDEFS=\"${HOST_FILESDIR}/vgrindefs\"
HOSTFILES=	vgrindefs vgrindefs.db
NOSHARE=# defined
CLEANFILES+= ${HOSTFILES}

.include "${.CURDIR}/../Makefile.host"

vgrindefs: vgrindefs.src
	cp ${.ALLSRC} ${.TARGET}

vgrindefs.db: vgrindefs
	${TOOL_CAP_MKDB} -f ${.ALLSRC} ${.ALLSRC}
