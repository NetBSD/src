#	$Id: Makefile,v 1.6 1993/12/08 11:47:31 mycroft Exp $

PROG=	ld.so
SRCS=	mdprologue.S rtld.c sbrk.c shlib.c etc.c md.c
NOMAN=	noman
LDDIR?= $(.CURDIR)/..
#PICFLAG=-pic
PICFLAG=-fpic
CFLAGS += -I$(LDDIR) -I$(.CURDIR) -I$(LDDIR)/$(MACHINE_ARCH) -O $(PICFLAG) -DRTLD
LDFLAGS = -Bshareable -Bsymbolic -assert nosymbolic
LIBS =  -lc_pic
BINDIR= /usr/libexec

.PATH: $(LDDIR) $(LDDIR)/$(MACHINE_ARCH)

.SUFFIXES: .S

$(PROG):
	$(LD) -o $(PROG) $(LDFLAGS) $(OBJS) $(LIBS) $(LDADD)

.S.o:
	$(CPP) $(.IMPSRC) | $(AS) -k -o $(.TARGET) -

.include <bsd.prog.mk>
