#	$NetBSD: Makefile,v 1.9 1998/09/27 17:22:13 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/9/93

PROG=	vfontedpr
SRCS=	regexp.c vfontedpr.c
MAN=	vgrind.1 vgrindefs.5
BINDIR=	/usr/libexec
CLEANFILES+=vgrindefs.src.db

.if !defined(NOSHARE)
FILES=				tmac.vgrind vgrindefs.src vgrindefs.src.db
FILESNAME_vgrindefs.src=	vgrindefs
FILESNAME_vgrindefs.src.db=	vgrindefs.db
FILESDIR=			/usr/share/misc
FILESDIR_tmac.vgrind=		/usr/share/tmac
.endif

SCRIPTS=vgrind.sh
SCRIPTSDIR=/usr/bin

all: vfontedpr vgrindefs.src.db

.if !defined(NOSHARE)
vgrindefs.src.db: vgrindefs.src
	cap_mkdb -f vgrindefs.src ${.CURDIR}/vgrindefs.src
.else
vgrindefs.src.db:
.endif

.include <bsd.prog.mk>
