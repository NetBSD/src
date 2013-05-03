#	$NetBSD: Makefile.inc,v 1.18 2013/05/03 09:37:17 skrll Exp $

LDELFSO_MACHINE_ARCH?=	${MACHINE_ARCH}

SRCS+=		rtld_start.S mdreloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
.if !empty(LDELFSO_MACHINE_ARCH:Mearm*)
CPPFLAGS+=	-DHAVE_INITFINI_ARRAY
SRCS+=		find_exidx.c
.endif

LDFLAGS+=	-Wl,-e,_rtld_start
