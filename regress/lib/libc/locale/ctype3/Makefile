#	$NetBSD: Makefile,v 1.5 2009/01/02 00:20:24 tnozaki Exp $

NOMAN=		# defined

PROG=		mbrtowc
CPPFLAGS+=	-DUSE_MBSTATE
.PATH:		${.CURDIR}/../ctype2

TESTS=		C en_US.UTF-8 ja_JP.ISO-2022-JP-2 ja_JP.SJIS ja_JP.eucJP

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
