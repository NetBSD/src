#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	     $Id: Makefile,v 1.2 1994/07/01 10:46:55 pk Exp $

DESTDIR=

RELOC_SUN4=	240000
RELOC_SUN4C=	340000
RELOC_SUN4M=	440000

RELOC=		${RELOC_SUN4C}

DEFS= -DSTANDALONE -DCOMPAT_NOLABEL # -DROMPRF
CFLAGS=	-O2 ${INCPATH} ${DEFS}

SRCS=   boot.c filesystem.c promdev.c version.c
#SRCS+=	io.c strerror.c

S=	${.CURDIR}/../../..

.PATH: ${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH: ${S}/stand ${S}/lib/libsa

INCPATH=-I${.CURDIR} -I${.CURDIR}/../.. -I${S} -I${S}/lib/libsa

### find out what to use for libkern
.include "$S/lib/libkern/Makefile.inc"
LIBKERN= ${KERNLIB}
#KERNLIB= ${.CURDIR}/../compile/libkern.a

.include "$S/lib/libsa/Makefile.inc"
LIBSA=	${SA_LIB}

LIBS=	${LIBSA} ${LIBKERN}

BOOTS=	boot
ALL=	${BOOTS}

all: ${ALL}

${BOOTS}: ${LIBS}

OBJS=	${SRCS:N*.h:R:S/$/.o/g}
${.OBJDIR}/libdrive.a: ${OBJS}
	ar crv $@ $?
	ranlib $@

# depend on DEFS

devopen.o machdep.o srt0.o: Makefile

# startups

srt0.o: ${.CURDIR}/srt0.S
	${CC} ${INCPATH} ${DEFS} -c ${.CURDIR}/srt0.S

# new boot
boot:	srt0.o ${OBJS} ${LIBS} fixhdr
	${LD} -N -T ${RELOC} -e start srt0.o ${OBJS} ${LIBS} -o $@
	${.OBJDIR}/fixhdr boot
	@size boot

bootconf.o: ${.CURDIR}/conf.c
	rm -f bootconf.c
	ln -s ${.CURDIR}/conf.c bootconf.c
	${CC} -c ${CFLAGS} -DBOOT bootconf.c
	rm -f bootconf.c

mkboot: ${.CURDIR}/mkboot.c
	${CC} ${CFLAGS} -o mkboot ${.CURDIR}/mkboot.c

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

