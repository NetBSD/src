#	$NetBSD: Makefile.inc,v 1.6.2.1 1999/06/21 03:01:43 perry Exp $

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s

LDSTATIC=	-static		# only static compilation makes sense here

# Where to find kernel images...
BSDSRCDIR?=	/usr/src
SRCSYSDIR?=	${BSDSRCDIR}/sys
KERNOBJDIR?=	${SRCSYSDIR}/arch/sun3/compile
