#	$NetBSD: Makefile,v 1.18.2.1 1997/08/02 00:03:13 cjs Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32")
PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c etc.c md.c vfprintf.c
CLIB=	${.CURDIR}/../../../../lib/libc
PICFLAG=-fpic -fno-function-cse
CFLAGS+=$(PICFLAG) -DRTLD -DLIBC_SCCS -I${CLIB}/include
ASFLAGS+=-k
LDFLAGS+=-Bshareable -Bsymbolic -assert nosymbolic
# XXX The ELF ports do not currently use this ld.so, so we don't have to
# worry about their linker not accepting -nostdlib.
.if defined(BUILDDIR)
LDFLAGS+= -nostdlib -L${BUILDDIR}/usr/lib
.endif
LDADD+=	-lc_pic
DPADD+=	${LIBC_PIC}
BINDIR= /usr/libexec
HDRS=	link.h
INCS=${HDRS}
INCSDIR=/usr/include

.PATH: ${CLIB}/stdio

$(PROG):
	$(LD) -o $(PROG) $(LDFLAGS) $(OBJS) $(LDADD)

.S.o:
	${CPP} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET} -
.endif

MAN= rtld.1
MLINKS=	rtld.1 ld.so.1

.include <bsd.prog.mk>
