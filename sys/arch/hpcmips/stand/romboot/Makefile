#	$NetBSD: Makefile,v 1.2 2002/01/03 11:28:12 shin Exp $

CC?=		${CROSSDIR}cc
AS?=		${CROSSDIR}as
LD?=		${CROSSDIR}ld
OBJCOPY?=	${CROSSDIR}objcopy

AFLAGS+=	-x assembler-with-cpp

all: romboot.bin

clean:
	rm -f romboot.bin romboot romboot.o

romboot: romboot.o
	$(LD) -o $@ -N -T romboot.ldscript -Ttext 0 -e _start $?

romboot.bin: romboot
	$(OBJCOPY) --remove-section=.reginfo -O binary $? $@
