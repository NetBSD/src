#	$NetBSD: Makefile.inc,v 1.1 1998/03/25 04:12:32 mhitch Exp $

SRCS+=	rtld_start.S mips_reloc.c

CPPFLAGS+= -mabicalls -G0 -fPIC -DELFSIZE=32 -I${.CURDIR}

.if (${MACHINE} == "newsmips")
# UGH!  ld.so.script specifies little-endian; need to override that here
LDFLAGS+= -EB
.endif

LDFLAGS+= -Bshareable -Bsymbolic -e .rtld_start --script mips/ld.so.script

# Link ld.so for backward compatibility with old binaries
LINKS+=	${DESTDIR}/usr/libexec/${PROG} ${DESTDIR}/usr/libexec/ld.so
