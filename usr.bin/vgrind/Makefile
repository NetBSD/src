#	$NetBSD: Makefile,v 1.12 2002/01/31 19:32:11 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/9/93

.include <bsd.own.mk>

PROG=	vfontedpr
SRCS=	regexp.c vfontedpr.c
MAN=	vgrind.1 vgrindefs.5
BINDIR=	/usr/libexec
CLEANFILES+=vgrindefs.src.db

.if ${MKSHARE} != "no"
FILES=				tmac.vgrind vgrindefs.src vgrindefs.src.db
FILESNAME_vgrindefs.src=	vgrindefs
FILESNAME_vgrindefs.src.db=	vgrindefs.db
FILESDIR=			/usr/share/misc
FILESDIR_tmac.vgrind=		/usr/share/tmac
.endif

SCRIPTS=	vgrind.sh
SCRIPTSDIR=	/usr/bin

CAP_MKDB?=	cap_mkdb

realall: vfontedpr vgrindefs.src.db

.if ${MKSHARE} != "no"
vgrindefs.src.db: vgrindefs.src
	${CAP_MKDB} -f vgrindefs.src ${.CURDIR}/vgrindefs.src
.else
vgrindefs.src.db:
.endif

.include <bsd.prog.mk>
