#	$NetBSD: Makefile.inc,v 1.3 2004/02/20 20:58:24 skrll Exp $

SRCS+=		rtld_start.S hppa_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+=	-fpic

CPPFLAGS+=	-DELFSIZE=32
#CPPFLAGS+=	-DRTLD_DEBUG
#CPPFLAGS+=	-DRTLD_DEBUG_RELOC
#CPPFLAGS+=	-DRTLD_DEBUG_HPPA

LDFLAGS+=	-Bshareable -Bsymbolic -e '$$rtld_start'
LDFLAGS+=	--script ${.CURDIR}/arch/hppa/ld.so.script

# XXX fredette - the libgcc millicode is needed here:
LDADD=		`$(CC) -print-libgcc-file-name`
