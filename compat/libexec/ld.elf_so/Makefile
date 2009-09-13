#	$NetBSD: Makefile,v 1.1.2.1.4.1 2009/09/13 21:21:17 matt Exp $

NOMAN=	# defined

.include <bsd.own.mk>
.include <bsd.obj.mk>

.if ${MKPIC} != "no"

SHLINKINSTALLDIR=	/usr/libexec

#CPPFLAGS+=	-DDEBUG
#CPPFLAGS+=	-DRTLD_DEBUG
#CPPFLAGS+=	-DRTLD_DEBUG_RELOC
#DBG=		-g

.include "${NETBSDSRCDIR}/compat/Makefile.compat"

.PATH: ${NETBSDSRCDIR}/libexec/ld.elf_so
.CURDIR:=${NETBSDSRCDIR}/libexec/ld.elf_so

# XXX
.if ${MACHINE} == "sparc64"
PROG=		ld.elf_so-sparc
ARCHSUBDIR=	sparc
.elif ${MACHINE} == "amd64"
PROG=		ld.elf_so-i386
ARCHSUBDIR=	i386
.elif ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
PROG=		ld.elf_so-64
ARCHSUBDIR=	mips
ABI64=		-mabi=64
.endif

# XXX why do i need this?
dependall: ${PROG}

.include "${.CURDIR}/Makefile"

.endif
