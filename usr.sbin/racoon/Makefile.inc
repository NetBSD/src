#	$NetBSD: Makefile.inc,v 1.11 2003/07/22 08:46:03 itojun Exp $

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

PKGVERSION=	netbsd-20030712

.include "../Makefile.inc"
