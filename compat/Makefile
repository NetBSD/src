#	$NetBSD: Makefile,v 1.1.4.1 2009/05/13 19:15:51 jym Exp $

# Build 32-bit compat versions of:
#	src/gnu/lib/libgcc4 into ${DESTDIR}/usr/lib/<32arch>
#	src/lib/libc into ${DESTDIR}/usr/lib/<32arch>
#	src/gnu/lib/ into ${DESTDIR}/usr/lib/<32arch>
#	src/lib/ into ${DESTDIR}/usr/lib/<32arch>
#	src/libexec/ld.elf_so into ${DESTDIR}/usr/libexec/ld.elf_so-<32arch>

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no" && \
    (${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "x86_64")

.if !make(includes)
SUBDIR=	gnu/lib/crtstuff4 .WAIT \
	lib/csu .WAIT \
	gnu/lib/libgcc4 .WAIT \
	lib/libc .WAIT \
	lib/libutil .WAIT \
	lib gnu/lib .WAIT \
	external/lib .WAIT \
	libexec/ld.elf_so
.endif

.endif

.include <bsd.subdir.mk>
