PREFIX?=	/usr/local
SYS_INCLUDES?=	-I/usr/src/sys-altq

PROG=		altqstat
OBJS= 		altqstat.o quip_client.o qdisc_conf.o \
		qdisc_cbq.o qdisc_hfsc.o qdisc_cdnr.o \
		qdisc_wfq.o qdisc_fifoq.o \
		qdisc_red.o qdisc_rio.o qdisc_blue.o qdisc_priq.o
MAN1=		altqstat.1

CFLAGS=		-g -Wall
CFLAGS+=	-W -Wpointer-arith -Wcast-qual -Wmissing-prototypes -ansi
DEFINES=	-DALTQ $(SYS_DEFINES)
INCLUDES=	-I. -I../libaltq $(SYS_INCLUDES)

all: $(PROG)

install: $(PROG)
	$(INSTALL) $(COPY) -m 0755 $(PROG) $(PREFIX)/bin
	$(INSTALL) $(COPY) -m 0644 $(MAN1) $(PREFIX)/man/man1

altqstat: $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -o $@ $(OBJS) $(SYS_LIBS) -lcurses

.c.o: 
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $*.c

clean:;		-rm -f $(PROG) *.a *.o core *.core *.bak ,* *~ "#"*
