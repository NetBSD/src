#	$NetBSD: Makefile,v 1.16 1997/04/16 16:49:43 christos Exp $

PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c etc.c md.c vfprintf.c
MAN= rtld.1
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
MLINKS=	rtld.1 ld.so.1
INCS=${HDRS}
INCSDIR=/usr/include

.PATH: ${.CURDIR}/../../../../lib/libc/stdio

$(PROG):
	$(LD) -o $(PROG) $(LDFLAGS) $(OBJS) $(LDADD)

.S.o:
	${CPP} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET} -

.include <bsd.prog.mk>
