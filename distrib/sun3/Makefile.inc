#	$NetBSD: Makefile.inc,v 1.5 1997/05/10 15:42:56 lukem Exp $

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s

LDSTATIC=	-static

# Hack needed to find kernel images...
BSDSRCDIR?=	/usr/src
SRCSYSDIR?=	${BSDSRCDIR}/sys
