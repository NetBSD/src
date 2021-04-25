#	$NetBSD: bsd.n32.mk,v 1.1 2021/04/25 15:18:23 christos Exp $

.if !empty(MACHINE_ARCH:M*eb)
LD+=		-m elf32btsmipn32
.else
LD+=		-m elf32ltsmipn32
.endif
.ifndef MLIBDIR
MLIBDIR=	n32

LIBC_MACHINE_ARCH=	${MACHINE_ARCH:S/mipsn/mips/:S/64//}
LIBGCC_MACHINE_ARCH=	${LIBC_MACHINE_ARCH}
GOMP_MACHINE_ARCH=	${LIBC_MACHINE_ARCH}
XORG_MACHINE_ARCH=	${LIBC_MACHINE_ARCH}

COPTS+=		-mabi=n32
CPUFLAGS+=	-mabi=n32
LDADD+=		-mabi=n32
LDFLAGS+=	-mabi=n32
MKDEPFLAGS+=	-mabi=n32
.endif

.include "${.PARSEDIR}/../../Makefile.compat"
