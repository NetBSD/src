#	$NetBSD: Makefile,v 1.11 2001/01/09 03:13:40 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	grog nroff libgroff libdriver libbib .WAIT \
	addftinfo eqn grodvi groff grolj4 grops grotty hpftodit indxbib \
	lkbib lookbib pfbtops pic psbb refer tbl tfmtodit troff

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=devices tmac mm man
.endif

# don't do xditview; we don't want to have to deal w/X not being installed
#SUBDIR+=	xditview

# BSD already provides soelim and afmtodit requires perl...
MISC=		soelim afmtodit

.include <bsd.subdir.mk>
