#	$NetBSD: Makefile.inc,v 1.7 2002/05/13 02:10:35 itojun Exp $

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

PKGVERSION=	netbsd-20020507

.include "../Makefile.inc"
