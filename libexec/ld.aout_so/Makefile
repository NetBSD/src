#	$NetBSD: Makefile,v 1.18 1997/05/30 14:06:34 cjs Exp $

.include <bsd.own.mk>	# for BUILDDIR

PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c etc.c md.c vfprintf.c
MAN= rtld.1
PICFLAG=-fpic -fno-function-cse
CFLAGS+=$(PICFLAG) -DRTLD -DLIBC_SCCS
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
MLINKS=	rtld.1 ld.so.1
INCS=${HDRS}
INCSDIR=/usr/include

.PATH: ${.CURDIR}/../../../../lib/libc/stdio

$(PROG):
	$(LD) -o $(PROG) $(LDFLAGS) $(OBJS) $(LDADD)

.S.o:
	${CPP} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET} -

.include <bsd.prog.mk>
