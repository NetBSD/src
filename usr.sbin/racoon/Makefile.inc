#	$NetBSD: Makefile.inc,v 1.12 2003/08/26 03:31:51 itojun Exp $

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

PKGVERSION=	netbsd-20030826

.include "../Makefile.inc"
