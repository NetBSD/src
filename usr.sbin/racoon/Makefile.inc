#	$NetBSD: Makefile.inc,v 1.2 2000/12/29 03:16:40 itojun Exp $

# point top of your KAME tree by ${KAMEROOT}
LIBPFKEYSRCDIR=	${.CURDIR}/../../../crypto/dist/kame/libipsec
RACOONSRCDIR=	${.CURDIR}/../../../crypto/dist/kame/racoon
TCPDUMPSRCDIR=	${.CURDIR}/../../../usr.sbin/tcpdump

ETCDIR?=	/etc/racoon
FILESDIR?=	/usr/share/examples/racoon

.if defined(SBIN_RACOON) && ${SBIN_RACOON} == "yes"
BINDIR?=        /sbin
LDSTATIC?=      -static
.endif

.include "../Makefile.inc"
