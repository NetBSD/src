#	$NetBSD: Makefile,v 1.4 2004/03/19 10:36:55 skrll Exp $

NOMAN=		# defined

PROG=		mbrtowc
CPPFLAGS+=	-DUSE_MBSTATE
.PATH:		${.CURDIR}/../ctype2

TESTS=		C en_US.UTF-8 ja_JP.ISO2022-JP2 ja_JP.SJIS ja_JP.eucJP

INPUTS=		${TESTS:S/$/.in/}
OUTPUTS=	${TESTS:S/$/.out/}

UUDECODE_FILES=	${INPUTS}

CLEANFILES+=	${OUTPUTS} ${INPUTS}

regress: ${PROG} ${TESTS}

.PHONY:		${TESTS}
.for L in ${TESTS}
$L: $L.in
	@env LC_CTYPE=$L LC_ALL= LANG= ./${PROG} `cat ${.ALLSRC}` > $L.out
.endfor

.include <bsd.files.mk>
.include <bsd.prog.mk>
