#	$NetBSD: Makefile,v 1.7 2004/12/09 17:35:38 wiz Exp $

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
