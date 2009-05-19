#	$NetBSD: Makefile,v 1.7 2009/05/19 20:27:22 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>
.include <bsd.obj.mk>

.if ${MKPIC} != "no"

LDFLAGS+=		-m32

#CPPFLAGS+=	-DDEBUG
#CPPFLAGS+=	-DRTLD_DEBUG
#CPPFLAGS+=	-DRTLD_DEBUG_RELOC
#DBG=		-g

.include "${NETBSDSRCDIR}/compat/Makefile.compat"

.PATH: ${NETBSDSRCDIR}/libexec/ld.elf_so
.CURDIR:=${NETBSDSRCDIR}/libexec/ld.elf_so

# XXX
.if ${MACHINE_ARCH} == "sparc64"
PROG=		ld.elf_so-sparc
ARCHSUBDIR=	sparc
.elif ${MACHINE_ARCH} == "x86_64"
PROG=		ld.elf_so-i386
ARCHSUBDIR=	i386
.endif

# XXX why do i need this?
dependall: ${PROG}

.include "${.CURDIR}/Makefile"

.if ${MACHINE_ARCH} == "sparc64"
# XXX: Turn on big pic for sparc, because we somehow are generating GOT13
# relocations. Why?!?
CPPFLAGS+=-fPIC
.endif

.endif
