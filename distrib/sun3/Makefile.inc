#	$NetBSD: Makefile.inc,v 1.3 1996/10/09 00:13:36 jtc Exp $

# Revision is 1.2
REV=	12

LDSTATIC=	-static

# Hack needed to find kernel images...
BSDSRCDIR?=	/usr/src
SRCSYSDIR?=	${BSDSRCDIR}/sys
