# $NetBSD: Makefile,v 1.2 2007/11/26 16:09:50 tnozaki Exp $

NOMAN=	#defined

PROG=	mbtowc_test
WARNS?=	4
TESTCASES=	en_US.UTF-8 \
		ja_JP.ISO2022-JP ja_JP.SJIS ja_JP.eucJP \
		zh_CN.GB18030 \
		zh_TW.Big5 zh_TW.eucTW
CPPFLAGS+=	-DFILEDIR=\"${.CURDIR}\"

regress: ${PROG}
	./${PROG} ${TESTCASES}

.include <bsd.prog.mk>
