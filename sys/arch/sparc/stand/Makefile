#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	     $Id: Makefile,v 1.3 1994/07/20 20:47:02 pk Exp $

DESTDIR=

RELOC_SUN4=	240000
RELOC_SUN4C=	340000
RELOC_SUN4M=	440000

RELOC?=		${RELOC_SUN4C}

DEFS= -DSTANDALONE
CFLAGS=	-O2 ${INCPATH} ${DEFS}

SRCS=   boot.c filesystem.c promdev.c version.c
XXSRCS=	bootxx.c promdev.c

S=	${.CURDIR}/../../..

.PATH: ${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH: ${S}/stand ${S}/lib/libsa

INCPATH=-I${.CURDIR} -I${S}/arch -I${S} -I${S}/lib/libsa

### find out what to use for libkern and libsa
.include "$S/lib/libkern/Makefile.inc"
LIBKERN= ${KERNLIB}
.include "$S/lib/libsa/Makefile.inc"
LIBSA=	${SA_LIB}

LIBS=	${LIBSA} ${LIBKERN}

BOOTS=	boot bootxx
ALL=	${BOOTS} installboot

all: ${ALL}

${BOOTS}: ${LIBS} .NOTMAIN

OBJS=	${SRCS:N*.h:R:S/$/.o/g}
XXOBJS=	${XXSRCS:N*.h:R:S/$/.o/g}

# depend on DEFS

srt0.o: Makefile

# startups

srt0.o: ${.CURDIR}/srt0.S
	${CC} ${INCPATH} ${DEFS} -c ${.CURDIR}/srt0.S

# new boot
boot:	srt0.o ${OBJS} ${LIBS} fixhdr
	${LD} -N -T ${RELOC} -e start srt0.o ${OBJS} ${LIBS} -o $@
	${.OBJDIR}/fixhdr $@
	@size $@

bootxx:	srt0.o ${XXOBJS} ${LIBS} fixhdr
	${LD} -N -T ${RELOC} -e start srt0.o ${XXOBJS} ${LIBS} -o $@
	${.OBJDIR}/fixhdr $@
	@size $@

installboot: ${.CURDIR}/installboot.c
	${CC} ${CFLAGS} -o installboot ${.CURDIR}/installboot.c

# utilities

fixhdr: fixhdr.c
	${CC} -o fixhdr ${.CURDIR}/fixhdr.c

clean cleandir:
	rm -f *.o errs make.out
	rm -f a.out boot cat ls fixhdr

.include <bsd.dep.mk>
.include <bsd.obj.mk>

