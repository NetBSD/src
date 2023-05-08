#	$NetBSD: Makefile,v 1.14 2023/05/08 20:56:19 skrll Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

LIBISPRIVATE=	yes

LIB=		bfd

.include "${.CURDIR}/arch/${GDB_MACHINE_ARCH}/defs.mk"

GCPPFLAGS=	${G_archdefs} ${G_DEFS} ${G_INCLUDES} ${G_TDEFAULTS}
CPPFLAGS+=	-I${.CURDIR}/arch/${GDB_MACHINE_ARCH} -I${DIST}/include -I. \
		-I${DIST}/bfd ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*} \
		-DDEBUGDIR=\"/usr/libdata/debug\" -DBINDIR=\"${BINDIR}\" \
		-DLIBDIR=\"/usr/libexec\"

GSRCS=		${G_libbfd_la_OBJECTS:libbfd.lo=lib_bfd.lo} \
		${G_libbfd_la_DEPENDENCIES:M*.lo}
SRCS=		${GSRCS:.lo=.c}

DPSRCS+=	elf32-target.h elf64-target.h targmatch.h \
		elf32-aarch64.c elf64-aarch64.c \
		elf32-ia64.c elf64-ia64.c \
		elf32-riscv.c elf64-riscv.c \
		peigen.c pex64igen.c
CLEANFILES+=	elf32-target.h elf64-target.h targmatch.h \
		elf32-aarch64.c elf64-aarch64.c \
		elf32-ia64.c elf64-ia64.c \
		elf32-riscv.c elf64-riscv.c \
		peigen.c pex64igen.c

.PATH: ${DIST}/bfd

.include <bsd.lib.mk>

COPTS+=         -Wno-stack-protector

${OBJS} ${SOBJS}: elf32-target.h elf64-target.h
targets.o targets.pico: targmatch.h Makefile

targmatch.h: config.bfd targmatch.sed
	${_MKTARGET_CREATE}
	${TOOL_SED} -f ${.ALLSRC:M*.sed} ${.ALLSRC:M*.bfd} >$@

elf32-target.h: elfxx-target.h
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/NN/32/g < $> > $@

elf64-target.h: elfxx-target.h
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/NN/64/g < $> > $@

elf64-aarch64.c: elfnn-aarch64.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/NN/64/g < $> > $@

elf32-aarch64.c: elfnn-aarch64.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/NN/32/g < $> > $@

elf32-ia64.c: elfxx-ia64.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/NN/32/g < $> > $@

elf64-ia64.c: elfxx-ia64.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/NN/64/g < $> > $@

elf32-riscv.c: elfnn-riscv.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/NN/32/g < $> > $@

elf64-riscv.c: elfnn-riscv.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/NN/64/g < $> > $@

peigen.c: peXXigen.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/XX/pe/g < $> > $@

pex64igen.c: peXXigen.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/XX/pex64/g < $> > $@
