#	$NetBSD: Makefile.inc,v 1.2 1998/03/02 19:58:19 cgd Exp $

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s

LDSTATIC=	-static		# only static compilation makes sense here

# Hack needed to find kernel images...
BSDSRCDIR?=	/usr/src
SRCSYSDIR?=	${BSDSRCDIR}/sys
