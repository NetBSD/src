#	$NetBSD: Makefile.inc,v 1.2 2002/09/24 12:12:08 mycroft Exp $

SRCS+=	rtld_start.S hppa_reloc.c

# XXX Should not be in CPPFLAGS!
CPPFLAGS+= -fpic

CPPFLAGS+= -DELFSIZE=32
#CPPFLAGS+= -DRTLD_DEBUG
#CPPFLAGS+= -DRTLD_DEBUG_RELOC
#CPPFLAGS+= -DRTLD_DEBUG_HPPA

LDFLAGS+= -Bshareable -Bsymbolic -e '$$rtld_start'
LDFLAGS+= --script ${.CURDIR}/arch/hppa/ld.so.script

# XXX fredette - the libgcc millicode is needed here:
LDADD=`$(CC) -print-libgcc-file-name`
