#	$Id: Makefile,v 1.9 1994/09/19 04:49:02 mycroft Exp $

PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c etc.c md.c vfprintf.c
NOMAN=	noman
LDDIR?= $(.CURDIR)/..
#PICFLAG=-pic
PICFLAG=-fpic
CFLAGS+=-I$(LDDIR) -I$(.CURDIR) -I$(LDDIR)/$(MACHINE_ARCH) $(PICFLAG) -DRTLD -DLIBC_SCCS
LDFLAGS+=-Bshareable -Bsymbolic -assert nosymbolic
ASFLAGS+=-k
LDADD+=	-lc_pic
BINDIR= /usr/libexec

.PATH: $(LDDIR) $(LDDIR)/$(MACHINE_ARCH) ${.CURDIR}/../../../../lib/libc/stdio

$(PROG):
	$(LD) -o $(PROG) $(LDFLAGS) $(OBJS) $(LDADD)

.S.o:
	${CPP} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET} -

.include <bsd.prog.mk>
