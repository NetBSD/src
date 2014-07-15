#	$NetBSD: Makefile,v 1.22 2014/07/15 13:17:15 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/9/93

WARNS=6
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
FILESDIR?=			/usr/share/misc
FILESDIR_tmac.vgrind?=		/usr/share/tmac
FILESNAME_tmac.vgrind=		vgrind.tmac
.endif

SCRIPTS=	vgrind.sh
SCRIPTSDIR=	/usr/bin

realall: vfontedpr vgrindefs.src.db

.include <bsd.endian.mk>
.if ${TARGET_ENDIANNESS} == "1234"
CAP_MKDB_ENDIAN=	-l
.elif ${TARGET_ENDIANNESS} == "4321"
CAP_MKDB_ENDIAN=	-b
.else
CAP_MKDB_ENDIAN=
.endif

.if ${MKSHARE} != "no"
vgrindefs.src.db: vgrindefs.src
	${_MKTARGET_CREATE}
	${TOOL_CAP_MKDB} ${CAP_MKDB_ENDIAN} -f vgrindefs.src ${.ALLSRC}
.else
vgrindefs.src.db:
.endif

.include <bsd.prog.mk>
