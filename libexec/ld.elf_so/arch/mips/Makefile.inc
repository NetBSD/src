#	$NetBSD: Makefile.inc,v 1.4 1999/01/16 14:59:03 christos Exp $

SRCS+=	rtld_start.S mips_reloc.c

CPPFLAGS+= -mabicalls -G0 -fPIC -DELFSIZE=32 -I${.CURDIR}

.if (${MACHINE} == "newsmips")
# UGH!  ld.so.script specifies little-endian; need to override that here
LDFLAGS+= -EB
.endif

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/mips/ld.so.script

# Link ld.so for backward compatibility with old binaries
LINKS+=	/usr/libexec/${PROG} /usr/libexec/ld.so
