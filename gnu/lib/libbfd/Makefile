#	$NetBSD: Makefile,v 1.31 2002/09/17 23:18:32 thorpej Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOMAN=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

LIB=		bfd

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	4
SHLIB_MINOR=	0

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain

GCPPFLAGS=	${G_archdefs} ${G_DEFS} ${G_INCLUDES} ${G_TDEFAULTS}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} -I. \
		${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*}

GSRCS=		${G_libbfd_la_OBJECTS:libbfd.lo=lib_bfd.lo} \
		${G_libbfd_la_DEPENDENCIES:M*.lo} ${G_BFD_MACHINES}
SRCS=		${GSRCS:.lo=.c} ${G_libopcodes_la_SOURCES}

DPSRCS+=	elf32-target.h elf64-target.h targmatch.h \
		elf32-ia64.c elf64-ia64.c peigen.c pepigen.c
CLEANFILES+=	elf32-target.h elf64-target.h targmatch.h \
		elf32-ia64.c elf64-ia64.c peigen.c pepigen.c

TEXINFO=	bfd.texinfo
INFOFLAGS=	-I${DIST}/bfd/doc

.PATH: ${DIST}/bfd ${DIST}/opcodes ${DIST}/bfd/doc

.include <bsd.lib.mk>
.include <bsd.info.mk>

${OBJS} ${SOBJS}: elf32-target.h elf64-target.h
targets.o targets.so: targmatch.h Makefile
.else
.include <bsd.prog.mk> # do nothing
.endif

targmatch.h: config.bfd targmatch.sed
	sed -f ${.ALLSRC:M*.sed} ${.ALLSRC:M*.bfd} >$@

elf32-target.h: elfxx-target.h
	sed -e s/NN/32/g < $> > $@

elf64-target.h: elfxx-target.h
	sed -e s/NN/64/g < $> > $@

elf32-ia64.c: elfxx-ia64.c
	sed -e s/NN/32/g < $> > $@

elf64-ia64.c: elfxx-ia64.c
	sed -e s/NN/64/g < $> > $@

peigen.c: peXXigen.c
	sed -e s/XX/pe/g < $> > $@

pepigen.c: peXXigen.c
	sed -e s/XX/pep/g < $> > $@
