# $Id: Makefile,v 1.1.2.2 2013/01/16 05:32:56 yamt Exp $

S=		${.CURDIR}/../../../../
PROG=		bootimx23
SRCS=		boot_prep.c power_prep.c clock_prep.c emi_prep.c \
		pinctrl_prep.c common.c

.include <bsd.own.mk>

CLEANFILES+=	${PROG}
CFLAGS+=	-Wall -Wno-main -ffreestanding -march=armv5te -mtune=arm926ej-s
CPPFLAGS+=	-D_STANDALONE
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -I${.OBJDIR} -I${S} -I${S}/arch
#CPPFLAGS+=	-DDEBUG
#DBG=		-g

LIBCRT0=	# nothing
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
	${LD} -N -Ttext ${RELOC} -Bstatic -e ${ENTRY} -o ${.TARGET} \
	    ${OBJS} ${LIBSA} ${LIBKERN}


cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.klinks.mk>
.include <bsd.prog.mk>

