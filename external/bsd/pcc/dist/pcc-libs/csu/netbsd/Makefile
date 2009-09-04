MACH ?=	i386
TARGET ?= $(MACH)-netbsd
VERSION ?= 0.9.9
PCCLIBDIR ?= /usr/local/lib/pcc/$(TARGET)/$(VERSION)/lib
SRCDIR ?= .

CFLAGS = -I$SRCDIR/$(MACH) -I$(SRCDIR) -Wall -Wstrict-prototypes -Wmissing-prototypes

OBJS = crtbegin.o crtend.o

all:	$(OBJS)

crtbegin.o: crtbegin.c
	$(CC) -O $(CFLAGS) -c -o $@ $<

crtend.o: crtend.c
	$(CC) -O $(CFLAGS) -c -o $@ $<

install:
	mkdir -p ${DESTDIR}$(PCCLIBDIR)
	cp $(OBJS) ${DESTDIR}$(PCCLIBDIR)

clean:
	rm -f $(OBJS)
