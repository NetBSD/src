#	$NetBSD: Makefile,v 1.1 2004/02/26 23:29:46 itojun Exp $

NOMAN=	# defined

PROG=	mbtest

regress: ${PROG}
	LC_CTYPE=ja_JP.eucJP LANG= ./${PROG} < ${.CURDIR}/euc-jp.txt \
		| diff ${.CURDIR}/euc-jp.output -
	LC_CTYPE=ja_JP.SJIS LANG= ./${PROG} < ${.CURDIR}/sjis.txt \
		| diff ${.CURDIR}/sjis.output -
	LC_CTYPE=ja_JP.ISO2022-JP LANG= ./${PROG} < ${.CURDIR}/iso-2022-jp.txt \
		| diff ${.CURDIR}/iso-2022-jp.output -
	LC_CTYPE=en_US.UTF-8 LANG= ./${PROG} < ${.CURDIR}/utf8.txt \
		| diff ${.CURDIR}/utf8.output -

.include <bsd.prog.mk>
