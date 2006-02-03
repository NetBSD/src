#	$NetBSD: Makefile,v 1.41 2006/02/03 16:29:34 christos Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOMAN=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

# If the platform does not support shared libraries, we need to supply
# *something* for BFD-using programs to link against.  Also provide an
# empty libinstall target so that libbfd.a does not get installed.
.if ${MKPIC} == "no"
MKLINKLIB=	yes
libinstall:	# do nothing
.endif

LIB=		bfd

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/binutils

SHLIB_MAJOR=	8
SHLIB_MINOR=	0

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

TEXINFO=	bfd.texinfo
INFOFLAGS=	-I${DIST}/bfd/doc

.PATH: ${DIST}/bfd ${DIST}/bfd/doc

.include <bsd.lib.mk>
.include <bsd.info.mk>

# XXX
.if ${MACHINE_CPU} == "sh5"
COPTS.elf32.c=                -O1
COPTS.elf64.c=                -O1
.endif

${OBJS} ${SOBJS}: elf32-target.h elf64-target.h
targets.o targets.so: targmatch.h Makefile
.else
.include <bsd.prog.mk> # do nothing
.endif

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
