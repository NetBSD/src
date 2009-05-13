#	$NetBSD: Makefile,v 1.1.2.2 2009/05/13 18:49:11 jym Exp $

.include <bsd.own.mk>
.include "${.CURDIR}/../Makefile.inc"

.if ${MKDOC} != "no"

DIST=	${IDIST}/doc ${IDIST}/doc/arm
.PATH:	${DIST}

BINDIR=	${BIND_HTMLDIR}/arm

BIND_ARM=	Bv9ARM.ch01.html Bv9ARM.ch01.html Bv9ARM.ch02.html \
		Bv9ARM.ch03.html Bv9ARM.ch04.html Bv9ARM.ch05.html \
		Bv9ARM.ch06.html Bv9ARM.ch07.html Bv9ARM.ch08.html \
		Bv9ARM.ch09.html Bv9ARM.html

FILES=		${BIND_ARM}

.endif

.include <bsd.prog.mk>
