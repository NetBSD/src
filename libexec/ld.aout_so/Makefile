#	$NetBSD: Makefile,v 1.13 1996/06/08 22:22:26 pk Exp $

PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c etc.c md.c vfprintf.c
MAN= rtld.1
LDDIR?= $(.CURDIR)/..
PICFLAG=-fpic -fno-function-cse
CFLAGS+=-I$(LDDIR) -I$(.CURDIR) -I$(LDDIR)/$(MACHINE_ARCH) $(PICFLAG) -DRTLD -DLIBC_SCCS
ASFLAGS+=-k
LDFLAGS+=-Bshareable -Bsymbolic -assert nosymbolic
.if defined(DESTDIR)
LDFLAGS+= -nostdlib -L${DESTDIR}/usr/lib
.endif
LDADD+=	-lc_pic
BINDIR= /usr/libexec
MLINKS=	rtld.1 ld.so.1

.PATH: $(LDDIR) $(LDDIR)/$(MACHINE_ARCH) ${.CURDIR}/../../../../lib/libc/stdio

$(PROG):
	$(LD) -o $(PROG) $(LDFLAGS) $(OBJS) $(LDADD)

.S.o:
	${CPP} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET} -

.include <bsd.prog.mk>
