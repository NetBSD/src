#	$NetBSD: Makefile,v 1.2 2002/09/18 05:41:40 lukem Exp $

NOMAN=		# defined

PROG=		ctype1

TESTS=		en_US.UTF-8 ja_JP.ISO2022-JP ja_JP.SJIS ja_JP.eucJP

INPUTS=		${TESTS:S/$/.in/}
EXPECTS=	${TESTS:S/$/.exp/}
OUTPUTS=	${TESTS:S/$/.out/}

CLEANFILES+=	${OUTPUTS} ${INPUTS} ${EXPECTS}

regress: ${PROG} ${INPUTS} ${EXPECTS}
.for L in ${TESTS}
	@env LC_CTYPE=$L LC_ALL= LANG= ./${PROG} < $L.in > $L.out
	@cmp $L.exp $L.out && echo "ok." 1>&2
.endfor

# paranoia: protect against non-ASCII characters
.for L in ${TESTS}
$L.in: $L.in.uu
	uudecode ${.ALLSRC}
$L.exp: $L.exp.uu
	uudecode ${.ALLSRC}
.endfor

.include <bsd.prog.mk>
