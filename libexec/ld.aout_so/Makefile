#	$NetBSD: Makefile,v 1.17 1997/05/30 11:39:01 veego Exp $

.include <bsd.own.mk>	# for OBJDIR and BUILDDIR

PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c etc.c md.c vfprintf.c
MAN= rtld.1
PICFLAG=-fpic -fno-function-cse
CFLAGS+=$(PICFLAG) -DRTLD -DLIBC_SCCS
ASFLAGS+=-k
LDFLAGS+=-Bshareable -Bsymbolic -assert nosymbolic
.if defined(OBJDIR)
LDFLAGS+= -nostdlib -L${BUILDDIR}/usr/lib
.else
.if defined(DESTDIR)
LDFLAGS+= -nostdlib -L${DESTDIR}/usr/lib
.endif
.endif	# defined(OBJDIR)
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
