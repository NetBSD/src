#	$NetBSD: Makefile,v 1.22 1997/10/25 10:12:46 lukem Exp $

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
CPPFLAGS+=$(PICFLAG) -DRTLD -DLIBC_SCCS -I${CLIB}/include
ASFLAGS+=-k
LDFLAGS+=-Bshareable -Bsymbolic -assert nosymbolic
.if defined(DESTDIR)
LDFLAGS+= -nostdlib -L${DESTDIR}/usr/lib
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
