#	$NetBSD: Makefile.inc,v 1.4 1997/03/16 11:26:29 veego Exp $

# Revision is 1.2D
REV=	12D

LDSTATIC=	-static

# Hack needed to find kernel images...
BSDSRCDIR?=	/usr/src
SRCSYSDIR?=	${BSDSRCDIR}/sys
