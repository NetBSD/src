#	$NetBSD: Makefile,v 1.7 1997/03/24 21:59:59 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/9/93

PROG=	vfontedpr
SRCS=	regexp.c vfontedpr.c
MAN=	vgrind.1 vgrindefs.5
BINDIR=	/usr/libexec
CLEANFILES+=vgrindefs.src.db

FILES=				tmac.vgrind vgrindefs.src vgrindefs.src.db
FILESNAME_vgrindefs.src=	vgrindefs
FILESNAME_vgrindefs.src.db=	vgrindefs.db
FILESDIR=			/usr/share/misc
FILESDIR_tmac.vgrind=		/usr/share/tmac

SCRIPTS=vgrind.sh

all: vfontedpr vgrindefs.src.db

vgrindefs.src.db: vgrindefs.src
	cap_mkdb -f vgrindefs.src ${.CURDIR}/vgrindefs.src

.include <bsd.prog.mk>
