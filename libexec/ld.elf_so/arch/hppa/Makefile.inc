#	$NetBSD: Makefile.inc,v 1.4 2004/02/21 15:56:03 skrll Exp $

SRCS+=		rtld_start.S hppa_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
#CPPFLAGS+=	-DRTLD_DEBUG
#CPPFLAGS+=	-DRTLD_DEBUG_RELOC
#CPPFLAGS+=	-DRTLD_DEBUG_HPPA

LDFLAGS+=	-e '$$rtld_start'
LDFLAGS+=	--script ${.CURDIR}/arch/hppa/ld.so.script

# XXX fredette - the libgcc millicode is needed here:
LDADD=		`$(CC) -print-libgcc-file-name`
