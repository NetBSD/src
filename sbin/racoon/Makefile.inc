#	$NetBSD: Makefile.inc,v 1.2 2000/10/04 00:27:48 itojun Exp $

# point top of your KAME tree by ${KAMEROOT}
#KAMEROOT=	/home/itojun/k
#LIBPFKEYSRCDIR=	${KAMEROOT}/kame/kame/kame/libipsec
#RACOONSRCDIR=	${KAMEROOT}/kame/kame/kame/racoon
#TCPDUMPSRCDIR=	${KAMEROOT}/kame/kame/kame/tcpdump

# this may or may not work.
##TCPDUMPSRCDIR=	${.CURDIR}/../../../usr.sbin/tcpdump

ETCDIR?=	/etc/racoon

.include "../Makefile.inc"
