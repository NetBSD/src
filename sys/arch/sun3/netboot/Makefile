.include <../../common/netboot/Makefile>

CFLAGS += -I../../common -I. -DDEBUG -Dsun3
CC = gcc
AS = gas
NOMAN =
OBJS+= start.o
SRCS+= le_poll.c machdep.c sunos_exec.c
CLEANFILES += genassym netboot.nm assym.s a.out netboot.o
LINKADDR= 240000

genassym: genassym.c
	${CC} ${CFLAGS} -Dmc68020 -Dsun3 -o genassym genassym.c

netboot: $(START_OBJ) $(OBJS)
	$(LD) -n -o netboot.o -e start -N -T $(LINKADDR) $(START_OBJ) $(OBJS) $(LIBC) 
	cp netboot.o a.out
	strip a.out
	dd if=a.out of=$@ ibs=32 skip=1
	rm -f a.out

netboot.nm:
	nm -n netboot > netboot.nm

assym.s: genassym
	./genassym >assym.s

start.o: start.s assym.s
	${CPP} -E ${COPTS} $< | ${AS} -o $@

install: netboot
	cp netboot /tftpboot/boot.sun3	

.include <bsd.prog.mk>
