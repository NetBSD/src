#	$NetBSD: Makefile.inc,v 1.6 2002/04/26 02:25:15 itojun Exp $

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

PKGVERSION=	netbsd-20020426

.include "../Makefile.inc"
