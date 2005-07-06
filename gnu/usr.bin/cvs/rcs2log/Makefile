#	$NetBSD: Makefile,v 1.6.6.1 2005/07/06 19:52:25 snj Exp $

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
