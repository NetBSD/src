#	$NetBSD: Makefile,v 1.7 1996/08/13 06:33:25 explorer Exp $

SUBDIR=		libgroff libdriver libbib\
		groff troff nroff tbl pic eqn grog grops grotty grodvi\
		grolj4 \
		refer lookbib indxbib lkbib \
		hpftodit tfmtodit addftinfo pfbtops psbb \
		devices tmac mm man

# don't do xditview; we don't want to have to deal w/X not being installed
#SUBDIR+=	xditview

# BSD already provides soelim and afmtodit requires perl...
MISC=		soelim afmtodit

.include <bsd.subdir.mk>
