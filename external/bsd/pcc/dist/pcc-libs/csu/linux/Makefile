MACH ?=	i386
TARGET ?= $(MACH)-linux
VERSION ?= 0.9.9
PCCLIBDIR ?= /usr/local/lib/pcc/$(TARGET)/$(VERSION)/lib
SRCDIR ?= .

CFLAGS = -I$(SRCDIR)/$(MACH) -I$(SRCDIR) -Wall -Wstrict-prototypes -Wmissing-prototypes

OBJS = crt0.o crt1.o gcrt1.o crti.o crtn.o crtbegin.o crtend.o

all:	$(OBJS)

crt0.o:	$(MACH)/crt0.c
	$(CC) -O $(CFLAGS) -c -o $@ $<

crt1.o:	$(MACH)/crt0.c
	$(CC) -O $(CFLAGS) -c -o $@ $<

gcrt1.o: $(MACH)/crt0.c
	$(CC) -O -DPROFILE $(CFLAGS) -c -o $@ $<

crti.o: $(MACH)/crti.c
	$(CC) -O $(CFLAGS) -c -o $@ $<

crtn.o: $(MACH)/crtn.c
	$(CC) -O $(CFLAGS) -c -o $@ $<

crtbegin.o: crtbegin.c
	$(CC) -O $(CFLAGS) -c -o $@ $<

crtend.o: crtend.c
	$(CC) -O $(CFLAGS) -c -o $@ $<

install:
	mkdir -p ${DESTDIR}$(PCCLIBDIR)
	cp $(OBJS) ${DESTDIR}$(PCCLIBDIR)

clean:
	rm -f $(OBJS)
