#	$NetBSD: Makefile.inc,v 1.2.2.2 1998/05/08 17:39:13 mycroft Exp $

SRCS+=	rtld_start.S mips_reloc.c

CPPFLAGS+= -mabicalls -G0 -fPIC -DELFSIZE=32 -I${.CURDIR}

.if (${MACHINE} == "newsmips")
# UGH!  ld.so.script specifies little-endian; need to override that here
LDFLAGS+= -EB
.endif

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start
LDFLAGS+= --script ${.CURDIR}/mips/ld.so.script

# Link ld.so for backward compatibility with old binaries
LINKS+=	${DESTDIR}/usr/libexec/${PROG} ${DESTDIR}/usr/libexec/ld.so
