#	$NetBSD: Makefile.inc,v 1.1.1.1 1995/10/08 23:07:45 gwr Exp $

# Revision is 1.0
REV=	10

LDSTATIC=	-static

# Hack needed to find kernel images...
BSDSRCDIR?=	/usr/src
SRCSYSDIR?=	${BSDSRCDIR}/sys
