#	$NetBSD: Makefile,v 1.1 2003/08/24 17:33:30 marcus Exp $

S	= ${.CURDIR}/../../../..

CC	= ${TOOLDIR}/bin/arm--netbsdelf-gcc
OBJCOPY	= ${TOOLDIR}/bin/arm--netbsdelf-objcopy

CFLAGS	= -W -Wall -mlittle-endian
CFLAGS	+= -O3 -mcpu=arm7tdmi
CFLAGS	+= -fomit-frame-pointer -funroll-loops -finline-functions
CFLAGS	+= -I${S}
#AFLAGS	= -mcpu=arm7tdmi -mthumb -mthumb-interwork -mapcs-32

all: aica_armcode.h

aica_armcode.h: aica_armcode.elf
	${OBJCOPY} -O binary aica_armcode.elf aica_armcode.bin
	echo '/* $$NetBSD: Makefile,v 1.1 2003/08/24 17:33:30 marcus Exp $$ */' > ${.TARGET}.tmp
	echo 'static u_int32_t aica_armcode[] = {' >> ${.TARGET}.tmp
	hexdump -v -e '" /* %04.4_ax */\t" 1/4 "0x%08x, " "\n"' \
		aica_armcode.bin >> ${.TARGET}.tmp
	echo '		0 };' >> ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}

aica_armcode.elf: aica_arm_locore.o aica_arm.o
	${CC} ${CFLAGS} -Wl,-Ttext,0 -Wl,-T ldscript -nostdlib -e 0 \
		-o aica_armcode.elf aica_arm_locore.o aica_arm.o

clean: clean-tmp
#	rm -f aica_armcode.h

clean-tmp:
	rm -f *.o aica_armcode.elf aica_armcode.bin aica_armcode.h.tmp

