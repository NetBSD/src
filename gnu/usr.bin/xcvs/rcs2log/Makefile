#	$NetBSD: Makefile,v 1.1 2006/02/04 16:22:54 christos Exp $

NOMAN=	# defined

.include "${.CURDIR}/../Makefile.inc"

SCRIPTS=	rcs2log
CLEANFILES=	${SCRIPTS}

rcs2log: rcs2log.sh
	sed -e "s/@MKTEMP_SH_FUNCTION@//" -e "s/@MKTEMP@/mktemp/" \
		${.ALLSRC} > ${.TARGET}

DIST=${IDIST}/contrib
.PATH: ${DIST}

.include <bsd.prog.mk>
