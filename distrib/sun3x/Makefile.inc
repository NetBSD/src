#	$NetBSD: Makefile.inc,v 1.1.1.1.2.2 1997/12/25 20:31:41 perry Exp $

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s

LDSTATIC=	-static

# Hack needed to find kernel images...
BSDSRCDIR?=	/usr/src
SRCSYSDIR?=	${BSDSRCDIR}/sys
