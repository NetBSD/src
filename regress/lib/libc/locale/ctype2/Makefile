#	$NetBSD: Makefile,v 1.2 2002/09/18 05:41:40 lukem Exp $

NOMAN=		# defined

PROG=		mbrtowc

TESTS=		C en_US.UTF-8 ja_JP.ISO2022-JP2 ja_JP.SJIS ja_JP.eucJP

INPUTS=		${TESTS:S/$/.in/}
OUTPUTS=	${TESTS:S/$/.out/}

CLEANFILES+=	${OUTPUTS} ${INPUTS}

regress: ${PROG} ${TESTS}

.PHONY:		${TESTS}
.for L in ${TESTS}
$L: $L.in
	@env LC_CTYPE=$L LC_ALL= LANG= ./${PROG} `cat ${.ALLSRC}` > $L.out
.endfor

# paranoia: protect against non-ASCII characters
.for L in ${TESTS}
$L.in: $L.in.uu
	uudecode ${.ALLSRC}
.endfor

.include <bsd.prog.mk>
