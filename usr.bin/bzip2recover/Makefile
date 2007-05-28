#	$NetBSD: Makefile,v 1.8 2007/05/28 12:06:25 tls Exp $

USE_FORT?= yes	# data-driven bugs?
NOMAN=	   # defined

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=       bzip2recover

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?= -static
.endif

DIST=	   ${NETBSDSRCDIR}/dist/bzip2
.PATH:	   ${DIST}

.include <bsd.prog.mk>
