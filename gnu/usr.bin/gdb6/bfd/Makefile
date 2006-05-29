#	$NetBSD: Makefile,v 1.2 2006/05/29 19:18:56 nathanw Exp $

.include <bsd.own.mk>

MKPRIVATELIB=	yes

LIB=		bfd

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gdb6

GCPPFLAGS=	${G_archdefs} ${G_DEFS} ${G_INCLUDES} ${G_TDEFAULTS}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} -I${DIST}/include -I. \
		-I${DIST}/bfd ${GCPPFLAGS:M-D*} ${GCPPFLAGS:M-I*:N-I.*}

GSRCS=		${G_libbfd_la_OBJECTS:libbfd.lo=lib_bfd.lo} \
		${G_libbfd_la_DEPENDENCIES:M*.lo}
SRCS=		${GSRCS:.lo=.c}

DPSRCS+=	elf32-target.h elf64-target.h targmatch.h \
		elf32-ia64.c elf64-ia64.c peigen.c pepigen.c
CLEANFILES+=	elf32-target.h elf64-target.h targmatch.h \
		elf32-ia64.c elf64-ia64.c peigen.c pepigen.c

.PATH: ${DIST}/bfd

.include <bsd.lib.mk>

# XXX
.if ${MACHINE_CPU} == "sh5"
COPTS.elf32.c=                -O1
COPTS.elf64.c=                -O1
.endif

${OBJS} ${SOBJS}: elf32-target.h elf64-target.h
targets.o targets.so: targmatch.h Makefile

targmatch.h: config.bfd targmatch.sed
	${_MKTARGET_CREATE}
	sed -f ${.ALLSRC:M*.sed} ${.ALLSRC:M*.bfd} >$@

elf32-target.h: elfxx-target.h
	${_MKTARGET_CREATE}
	sed -e s/NN/32/g < $> > $@

elf64-target.h: elfxx-target.h
	${_MKTARGET_CREATE}
	sed -e s/NN/64/g < $> > $@

elf32-ia64.c: elfxx-ia64.c
	${_MKTARGET_CREATE}
	sed -e s/NN/32/g < $> > $@

elf64-ia64.c: elfxx-ia64.c
	${_MKTARGET_CREATE}
	sed -e s/NN/64/g < $> > $@

peigen.c: peXXigen.c
	${_MKTARGET_CREATE}
	sed -e s/XX/pe/g < $> > $@

pepigen.c: peXXigen.c
	${_MKTARGET_CREATE}
	sed -e s/XX/pep/g < $> > $@
