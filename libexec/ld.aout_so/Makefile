#	$NetBSD: Makefile,v 1.37 2002/03/29 09:48:49 matt Exp $

.include <bsd.own.mk>
.if (${OBJECT_FMT} == "a.out")
PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c md.c vfprintf.c
CLIB=	${.CURDIR}/../../lib/libc
CLIBOBJ!=cd ${CLIB} && ${PRINTOBJDIR}
PICFLAG=-fpic -fno-function-cse
CPPFLAGS+=$(PICFLAG) -DRTLD -DLIBC_SCCS \
	  -I${CLIB}/include -I$(.CURDIR)/arch/$(MACHINE_ARCH) -I$(.CURDIR)
.if defined(DEBUG)
CPPFLAGS+=-DDEBUG
.endif
ASFLAGS+=-k
LDFLAGS+=-Bshareable -Bsymbolic -assert nosymbolic -L${CLIBOBJ}
.if defined(DESTDIR)
LDFLAGS+= -nostdlib -L${DESTDIR}/usr/lib
.endif
LDADD+=	-lc_pic
DPADD+=	${LIBC_PIC}

.PATH: ${CLIB}/stdio $(.CURDIR)/arch/$(MACHINE_ARCH)

$(PROG):
	$(LD) -o $(PROG) $(LDFLAGS) $(OBJS) $(LDADD)

.S.o:
	${CPP} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET} -
.elif (${OBJECT_FMT} == "ELF" && \
       (${MACHINE_ARCH} == "arm" || \
        ${MACHINE_ARCH} == "i386" || \
        ${MACHINE_ARCH} == "m68k" || \
        ${MACHINE_ARCH} == "sparc" || \
        ${MACHINE_ARCH} == "vax"))

FILES=		ld.so
FILESDIR=	/usr/libexec
FILESMODE=	${BINMODE}
CLEANFILES+=	ld.so

all dependall: ${FILES}

UUDECODE?=	uudecode

ld.so:	${.CURDIR}/ld.so.${MACHINE_ARCH}.uue
	${UUDECODE} ${.ALLSRC}

.include <bsd.files.mk>

.endif

MAN= ld.aout_so.1

.include <bsd.prog.mk>
