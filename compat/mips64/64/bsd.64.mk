#	$NetBSD: bsd.64.mk,v 1.14 2024/07/09 15:11:28 rin Exp $

.if !empty(MACHINE_ARCH:M*eb)
LD+=		-m elf64btsmip
.else
LD+=		-m elf64ltsmip
.endif
.ifndef MLIBDIR
MLIBDIR=	64

LIBC_MACHINE_ARCH=	${MACHINE_ARCH:S/mips/mipsn/}
LIBGCC_MACHINE_ARCH=	${LIBC_MACHINE_ARCH}
GOMP_MACHINE_ARCH=	${LIBC_MACHINE_ARCH}
XORG_MACHINE_ARCH=	${LIBC_MACHINE_ARCH}
BFD_MACHINE_ARCH=	${LIBC_MACHINE_ARCH}

# GCC 5/6 libgomp for n64 needs files we don't generate yet.
NO_LIBGOMP=	1

COPTS+=		-mabi=64
CPUFLAGS+=	-mabi=64
LDADD+=		-mabi=64
LDFLAGS+=	-mabi=64
MKDEPFLAGS+=	-mabi=64
.endif

.include "${.PARSEDIR}/../../Makefile.compat"
