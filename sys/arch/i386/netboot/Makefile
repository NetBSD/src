# netboot
#
# $Log: Makefile,v $
# Revision 1.1  1993/07/08 16:03:44  brezak
# Diskless boot prom code from Jim McKim (mckim@lerc.nasa.gov)
#
# Revision 1.4  1993/06/30  20:14:08  mckim
# Added BOOTP support.
#
# Revision 1.3  1993/06/08  14:37:18  mckim
# TAG rule missed proto.h.
#
# Revision 1.2  1993/05/28  20:01:24  mckim
# Fixed various StartProg() problems.
#
# Revision 1.1.1.1  1993/05/28  11:41:08  mckim
# Initial version.
#

# uncomment one of these as appropriate for the board being used
#ETH_OBJ = ne2100.o
ETH_OBJ = wd80x3.o

# set this to the size of the eprom (decimal)
ROM_SIZE = 16384

.SUFFIXES: .s .c .o .list
.c.list:
	$(CC) $(CFLAGS) $(INC) -S $<
	$(AS) $*.s -a -o /dev/null > $@
	rm $*.s

.s.o:
	@echo $(AS) -o $*.o $< [$(DEFINES)]
	-@trap "/bin/rm -f $*.i X$*.c; exit 0" 0 1 2 3 10 15; \
	/bin/rm -f X$*.c; \
	ln $*.s X$*.c; \
	$(CC) -E $(CFLAGS) X$*.c > $*.i; \
	if [ $$? != 0 ]; then :; \
	else \
	 $(AS) $*.i -o $@; \
	fi
.s.list:
	@echo $(AS) -o $*.o $< [$(DEFINES)]
	-@trap "/bin/rm -f $*.i X$*.c; exit 0" 0 1 2 3 10 15; \
	/bin/rm -f X$*.c;	\
	ln $*.s X$*.c;	\
	$(CC) -E $(CFLAGS) X$*.c > $*.i;	\
	if [ $$? != 0 ]; then :;	\
	else \
		$(AS) $*.i -a -o /dev/null > $@;	\
	fi

#AS = as
#CC = cc
#LD = ld


# the relocation address (hex)
RELOC = 98000
CFLAGS = -O $(DEFINES) -nostdinc
DEFINES = -DRELOC=0x$(RELOC) -DROM_SIZE=$(ROM_SIZE) -DTRACE=0 -Dprinte=printf \
 -DDEBUG -DUSE_BOOTP -DUSE_RARP #-DPhysBcopy=bcopy -DPhysBzero=bzero

OBJS = start.o main.o cga.o kbd.o packet.o tftp.o arp.o \
 $(ETH_OBJ) misc.o asm.o

all: boot.bin

boot.bin: $(OBJS) genprom
	${LD} -N -T ${RELOC} $(OBJS)
	cp a.out boot.sym
	rm -f $@
	strip a.out
	dd if=a.out of=boot.v ibs=32 skip=1 obs=1024b
	genprom <boot.v >$@
	rm -f a.out boot.v

genprom: genprom.c
	$(CC) -o $@ -DROM_SIZE=$(ROM_SIZE) genprom.c

# copy to dos floppy for testing (faster than burning an eprom)
dostest: boot.bin
	mcopy -n boot.bin a:

TAGS:	main.c cga.c kbd.c packet.c tftp.c arp.c wd80x3.c misc.c proto.h
	etags main.c cga.c kbd.c packet.c tftp.c arp.c wd80x3.c misc.c proto.h

clean:
	rm -f *.o *.list *~ genprom

cleandir: clean
	rm -f boot.bin boot.sym

.include <bsd.prog.mk>
