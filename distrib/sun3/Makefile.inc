#	$NetBSD: Makefile.inc,v 1.7 1999/05/16 08:21:52 gwr Exp $

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s

LDSTATIC=	-static		# only static compilation makes sense here

# Where to find kernel images...
BSDSRCDIR?=	/usr/src
SRCSYSDIR?=	${BSDSRCDIR}/sys
KERNOBJDIR?=	${SRCSYSDIR}/arch/sun3/compile
