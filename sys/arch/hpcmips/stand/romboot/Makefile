#	$NetBSD: Makefile,v 1.1 2001/12/23 13:27:56 shin Exp $

CC?=		${CROSSDIR}cc
AS?=		${CROSSDIR}as
LD?=		${CROSSDIR}ld
OBJCOPY?=	${CROSSDIR}objcopy

AFLAGS+=	-x assembler-with-cpp -traditional-cpp

all: romboot.bin

clean:
	rm -f romboot.bin romboot romboot.o

romboot: romboot.o
	$(LD) -o $@ -N -T romboot.ldscript -Ttext 0 -e _start $?

romboot.bin: romboot
	$(OBJCOPY) --remove-section=.reginfo -O binary $? $@
