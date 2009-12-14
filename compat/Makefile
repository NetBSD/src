#	$NetBSD: Makefile,v 1.1.8.2 2009/12/14 06:20:58 mrg Exp $

# Build 32-bit compat versions of:
#	src/gnu/lib/libgcc4 into ${DESTDIR}/usr/lib/<32arch>
#	src/lib/libc into ${DESTDIR}/usr/lib/<32arch>
#	src/gnu/lib/ into ${DESTDIR}/usr/lib/<32arch>
#	src/lib/ into ${DESTDIR}/usr/lib/<32arch>
#	src/libexec/ld.elf_so into ${DESTDIR}/usr/libexec/ld.elf_so-<32arch>

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no"
.if !make(includes)

.include "Makefile.md_subdir"

SUBDIR=	dirshack .WAIT ${MD_SUBDIR}

.endif
.endif

.include <bsd.subdir.mk>
