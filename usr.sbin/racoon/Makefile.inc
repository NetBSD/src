#	$NetBSD: Makefile.inc,v 1.9 2002/09/18 03:54:36 lukem Exp $

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

PKGVERSION=	netbsd-20020507

.include "../Makefile.inc"
