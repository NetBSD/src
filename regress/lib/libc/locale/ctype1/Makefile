#	$NetBSD: Makefile,v 1.3 2004/03/19 08:07:28 jmc Exp $

NOMAN=		# defined

PROG=		ctype1

TESTS=		en_US.UTF-8 ja_JP.ISO2022-JP ja_JP.SJIS ja_JP.eucJP

INPUTS=		${TESTS:S/$/.in/}
EXPECTS=	${TESTS:S/$/.exp/}
OUTPUTS=	${TESTS:S/$/.out/}

UUDECODE_FILES=	${INPUTS} ${EXPECTS}

CLEANFILES+=	${OUTPUTS} ${INPUTS} ${EXPECTS}

regress: ${PROG} ${INPUTS} ${EXPECTS}
.for L in ${TESTS}
	@env LC_CTYPE=$L LC_ALL= LANG= ./${PROG} < $L.in > $L.out
	@cmp $L.exp $L.out && echo "ok." 1>&2
.endfor

.include <bsd.files.mk>
.include <bsd.prog.mk>
