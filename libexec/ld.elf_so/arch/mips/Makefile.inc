#	$NetBSD: Makefile.inc,v 1.5 1999/02/27 10:44:27 pk Exp $

SRCS+=	rtld_start.S mips_reloc.c

CPPFLAGS+= -mabicalls -G0 -fPIC -DELFSIZE=32 -I${.CURDIR}
CPPFLAGS+= -DRTLD_INHIBIT_COPY_RELOCS

.if (${MACHINE} == "newsmips")
# UGH!  ld.so.script specifies little-endian; need to override that here
LDFLAGS+= -EB
.endif

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/arch/mips/ld.so.script

# Link ld.so for backward compatibility with old binaries
LINKS+=	/usr/libexec/${PROG} /usr/libexec/ld.so
