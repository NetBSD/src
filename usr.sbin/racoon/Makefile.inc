#	$NetBSD: Makefile.inc,v 1.1 2000/12/29 02:33:39 itojun Exp $

# point top of your KAME tree by ${KAMEROOT}
LIBPFKEYSRCDIR=	${.CURDIR}/../../../crypto/dist/kame/libipsec
RACOONSRCDIR=	${.CURDIR}/../../../crypto/dist/kame/racoon
TCPDUMPSRCDIR=	${.CURDIR}/../../../usr.sbin/tcpdump

ETCDIR?=	/etc/racoon

.include "../Makefile.inc"
