PREFIX?=	/usr/local
SYS_INCLUDES?=	-I/usr/src/sys-altq

PROGS=		altqd
ALTQD_OBJS= 	altqd.o if_nameindex.o libaltq2.o ../libaltq/libaltq.a
MAN5=		altq.conf.5
MAN8=		altqd.8

CFLAGS=		-g -Wall
CFLAGS+=	-W -Wpointer-arith -Wcast-qual -Wmissing-prototypes -ansi
DEFINES=	-DALTQ $(SYS_DEFINES)
INCLUDES=	-I. -I../libaltq $(SYS_INCLUDES)

all: $(PROGS)

install: $(PROGS)
	$(INSTALL) $(COPY) -m 0755 $(PROGS) $(PREFIX)/bin
	$(INSTALL) $(COPY) -m 0644 $(MAN5) $(PREFIX)/man/man5
	$(INSTALL) $(COPY) -m 0644 $(MAN8) $(PREFIX)/man/man8

altqd: $(ALTQD_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -o $@ $(ALTQD_OBJS) $(SYS_LIBS) -lm

.c.o: 
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $*.c

clean:;		-rm -f $(PROGS) *.a *.o core *.core *.bak ,* *~ "#"*


