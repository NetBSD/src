#	$NetBSD: Makefile.inc,v 1.10 2002/11/20 03:35:58 itojun Exp $

.include <bsd.own.mk>		# for NETBSDSRCDIR & MKDYNAMICROOT definition

# point top of your KAME tree by ${KAMEROOT}
LIBPFKEYSRCDIR=	${NETBSDSRCDIR}/crypto/dist/kame/libipsec
RACOONSRCDIR=	${NETBSDSRCDIR}/crypto/dist/kame/racoon
TCPDUMPSRCDIR=	${NETBSDSRCDIR}/dist/tcpdump

ETCDIR?=	/etc/racoon
FILESDIR?=	/usr/share/examples/racoon

.if defined(SBIN_RACOON) && ${SBIN_RACOON} == "yes"
BINDIR?=        /sbin
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=      -static
.endif
.endif

DBG=	-g

PKGVERSION=	netbsd-20021120

.include "../Makefile.inc"
