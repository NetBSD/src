#
# Simple makefile for the F77 I/O library.
#

CFLAGS=-Wall -Werror -Wstrict-prototypes -Wmissing-prototypes

.c.o :
	cc -O $(CFLAGS) -c $<
	-ld -r -x $*.o
	mv a.out $*.o

OBJS= backspace.o dfe.o due.o iio.o inquire.o lib.o rewind.o rsfe.o rdfmt.o \
	sue.o uio.o wsfe.o sfe.o fmt.o lio.o lread.o open.o close.o \
	util.o endfile.o wrtfmt.o err.o fmtlib.o

libI77.a: $(OBJS)
	ar r libI77.a $?

clean:
	/bin/rm -f $(OBJS) libI77.a
