PREFIX?=	/usr/local
SYS_INCLUDES?=	-I/usr/src/sys-altq

LIB=	libaltq.a

OBJS= 	qop.o parser.o quip_server.o qop_cdnr.o qop_errlist.o \
	qop_conf.o qop_dummy.o qop_cbq.o qop_hfsc.o qop_priq.o \
	qop_red.o qop_rio.o qop_blue.o qop_wfq.o qop_fifoq.o \
	qop_priq.o

CFLAGS=		-g -Wall
CFLAGS+=	-Wpointer-arith -Wcast-qual -Wmissing-prototypes -ansi
DEFINES=	-DALTQ $(SYS_DEFINES)
INCLUDES=	-I. $(SYS_INCLUDES)

all: $(LIB)

install: $(LIB)
	$(INSTALL) $(COPY) -m 0444 $(LIB) $(PREFIX)/lib

libaltq.a: libaltq.a($(OBJS))
	rm -f $@
	ar cq $@ `lorder $(OBJS) | tsort -q`
	ranlib $@

.c.o: 
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $*.c

clean:;		-rm -f $(LIB) *.a *.o core *.core *.bak ,* *~ "#"*









