#	$NetBSD: Makefile,v 1.6 2002/08/27 14:46:16 lukem Exp $

NOMAN=	   # defined

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=       bzip2recover

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?= -static
.endif

DIST=	   ${.CURDIR}/../../dist/bzip2
.PATH:	   ${DIST}

.include <bsd.prog.mk>
