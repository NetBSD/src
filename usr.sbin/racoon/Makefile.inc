#	$NetBSD: Makefile.inc,v 1.4 2001/06/27 05:17:32 enami Exp $

# point top of your KAME tree by ${KAMEROOT}
LIBPFKEYSRCDIR=	${.CURDIR}/../../../crypto/dist/kame/libipsec
RACOONSRCDIR=	${.CURDIR}/../../../crypto/dist/kame/racoon
TCPDUMPSRCDIR=	${.CURDIR}/../../../dist/tcpdump

ETCDIR?=	/etc/racoon
FILESDIR?=	/usr/share/examples/racoon

.if defined(SBIN_RACOON) && ${SBIN_RACOON} == "yes"
BINDIR?=        /sbin
LDSTATIC?=      -static
.endif

DBG=	-g

.include "../Makefile.inc"
