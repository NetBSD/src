#	$NetBSD: Makefile,v 1.6 2000/01/19 16:30:02 tsutsui Exp $

BOOT=	xxboot
VERSION=0.3

# text and bss addresses in hex
TEXT=	3f0000
BSS=	3f2000

PROG=		$(BOOT)
BINDIR=		/usr/mdec
PROGNAME=	sdboot
LINKS=		${BINDIR}/sdboot ${BINDIR}/fdboot
STRIPFLAG=
BINMODE=	444
SCRIPTSMODE=	555
SCRIPTS=	installboot.sh
MKMAN=		no

STRIPPROG?=	strip

SRCS=	xxboot.S bootufs.c unzip.c
.PATH:	${.CURDIR}/gunzip

CFLAGS=	-O -fomit-frame-pointer
CFLAGS+= -W -Wall -Wconversion -Wstrict-prototypes -Wmissing-prototypes
CFLAGS+= -DTEXTADDR="0x$(TEXT)" -DBOOT=\"$(BOOT)\" -DBOOT_VERS=\"$(VERSION)\"
CFLAGS+= -DGZIP -DSCSI_ADHOC_BOOTPART
CFLAGS+= -DTITLE_IMAGE -DIMAGE_EXTRA=100
#CFLAGS+= -DBOOT_DEBUG
AFLAGS=	${CFLAGS:M-[ID]*}
LDFLAGS=-n -Bstatic -T $(TEXT)
#LDADD=	-lc

CLEANFILES=	$(BOOT).x s.x x.s x.o
CLEANFILES+=	ej2sjesc.lo ej2sjesc *.o.c *.o.s *.o.o

.c.o:
	./ej2sjesc ${.IMPSRC} ${.TARGET}.c
	@echo '${CC} -fall-bsr ${CFLAGS} ${CPPFLAGS} -o ${.TARGET} -c ${.TARGET}.c'
	@${CC} ${CFLAGS} ${CPPFLAGS} -S ${.TARGET}.c -o ${.TARGET}.s
	@${CC} -c -o ${.TARGET}.o ${.TARGET}.s
	@${NM} ${.TARGET}.o | ${.CURDIR}/all_bsr.sh ${.TARGET}.s | ${AS} -o ${.TARGET}
	@rm -f ${.TARGET}.c ${.TARGET}.s ${.TARGET}.o

$(BOOT): $(OBJS)
	$(LD) $(LDFLAGS) -o $(BOOT).x $(OBJS) $(LDADD)
	@$(NM) $(BOOT).x | sed -n '/T first_kbyte/p'
	@if [ `(echo ibase=16; \
		$(NM) $(BOOT).x | sed -n 's/T first_kbyte/-$(TEXT)-400/p' | \
				tr a-f A-F) | bc` -gt 0 ];\
	then echo '$(BOOT): first_kbyte exceeds the first killobyte'; exit 1; fi
	@$(SIZE) $(BOOT).x
	@if [ `(echo ibase=16; \
		$(NM) $(BOOT).x | sed -n 's/D _edata/-$(BSS)/p' | tr a-f A-F) |\
			bc` -gt 0 ];\
	then	echo '$(BOOT): text+data is too large'; exit 1; fi
	@cp $(BOOT).x s.x
	@$(STRIPPROG) s.x
	@dd bs=32 skip=1 count=256 if=s.x of=$(BOOT) 2>/dev/null
	@rm s.x

bootufs.o unzip.o: ej2sjesc

# helper program running on the host
ej2sjesc: ej2sjesc.lo
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC}

.include <bsd.prog.mk>
