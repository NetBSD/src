#	$NetBSD: Makefile,v 1.10 1999/02/13 02:54:26 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	libgroff libdriver libbib\
	groff troff nroff tbl pic eqn grog grops grotty grodvi grolj4 \
	refer lookbib indxbib lkbib hpftodit tfmtodit addftinfo pfbtops psbb

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=devices tmac mm man
.endif

all-addftinfo all-eqn all-grodvi all-groff all-grolj4 all-grops all-grotty \
all-hpftodit all-indxbib all-lkbib all-lookbib all-pfbtops all-pic all-refer \
all-tbl all-tfmtodit all-troff: \
	all-libgroff all-libdriver all-libbib

# don't do xditview; we don't want to have to deal w/X not being installed
#SUBDIR+=	xditview

# BSD already provides soelim and afmtodit requires perl...
MISC=		soelim afmtodit

.include <bsd.subdir.mk>
