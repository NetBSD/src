# $Id: Makefile,v 1.13.2.1 2014/08/10 06:53:56 tls Exp $

S=		${.CURDIR}/../../../../
PROG=		bootimx23
SRCS=		bootimx23.c power_prep.c clock_prep.c emi_prep.c pinctrl_prep.c common.c args_prep.c

.include <bsd.own.mk>

CLEANFILES+=	${PROG}
CFLAGS+=	-Wall -Wno-main -ffreestanding -fno-unwind-tables
CPUFLAGS=	-march=armv5te -mtune=arm926ej-s
CPPFLAGS+=	-D_STANDALONE -DMEMSIZE=64
CPPFLAGS+=	-DKERNEL_BOOT_ARGS=\"root=ld0a\"
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -I${.OBJDIR} -I${S} -I${S}/arch
CPPFLAGS+=	${ARM_APCS_FLAGS}
.if ${HAVE_LLVM:Uno} != "no" || ${HAVE_GCC:U0} >= 48
CPPFLAGS+=	-marm
.else
CPPFLAGS+=	-mno-thumb -mno-thumb-interwork
.endif
#CPPFLAGS+=	-DDEBUG
#CPPFLAGS+=	-DDIAGNOSTIC
DBG=		-g

LIBCRT0=	# nothing
LIBCRTI=	# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing
LIBC=		# nothing

MAN=		# no manual page
NOMAN=		# defined
STRIPFLAG=
BINMODE=	444

RELOC=		0x00000000
ENTRY=		_start

### find out what to use for libkern
KERN_AS=	library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

### find out what to use for libsa
SA_AS=		library
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	${MKTARGET_LINK}
	${LD} -N -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${.TARGET} \
	    ${OBJS} ${LIBSA} ${LIBKERN}


cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.klinks.mk>
.include <bsd.prog.mk>

