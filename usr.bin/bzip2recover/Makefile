#	$NetBSD: Makefile,v 1.7 2002/09/18 14:00:35 lukem Exp $

NOMAN=	   # defined

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=       bzip2recover

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?= -static
.endif

DIST=	   ${NETBSDSRCDIR}/dist/bzip2
.PATH:	   ${DIST}

.include <bsd.prog.mk>
