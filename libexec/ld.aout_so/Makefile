#	$NetBSD: Makefile,v 1.20 1997/06/23 13:25:58 mrg Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32")
PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c etc.c md.c vfprintf.c
PICFLAG=-fpic -fno-function-cse
CFLAGS+=$(PICFLAG) -DRTLD -DLIBC_SCCS
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

.PATH: ${.CURDIR}/../../../../lib/libc/stdio

$(PROG):
	$(LD) -o $(PROG) $(LDFLAGS) $(OBJS) $(LDADD)

.S.o:
	${CPP} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET} -
.endif

MAN= rtld.1
MLINKS=	rtld.1 ld.so.1

.include <bsd.prog.mk>
