#	$NetBSD: Makefile.inc,v 1.2.6.2 1997/03/13 00:49:46 mycroft Exp $

# Revision is 1.2.1
REV=	121

LDSTATIC=	-static

# Hack needed to find kernel images...
BSDSRCDIR?=	/usr/src
SRCSYSDIR?=	${BSDSRCDIR}/sys
