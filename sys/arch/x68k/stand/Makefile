#	$NetBSD: Makefile,v 1.1.2.1 1998/10/13 21:25:40 cgd Exp $

# Define all target-dependent utilities as macros in case of cross compilation.
# These definitions can be overridden by  <sys.mk>
LD?=	ld
AS?=	as
NM?=	nm
XSTRIP?=strip
SIZE?=	size

# text and bss addresses in hex
TEXT=	3f0000
BSS=	3f2000

BOOT=	xxboot
VERSION=0.2

SRCS=	xxboot.S bootufs.c unzip.c
OBJS=	xxboot.o bootufs.o unzip.o
.PATH:  ${.CURDIR}/gunzip

#G=-g
#M=-m68020 -fall-bsr
CFLAGS=	-O -fomit-frame-pointer -Wall $M $G -DGZIP -DBOOT=\"$(BOOT)\"
#LDFLAGS=-N -Bstatic -Ttext 0x$(TEXT) -Tbss 0x$(BSS)
LDFLAGS=-n -Bstatic -T $(TEXT)
#LDADD=	-lc

CLEANFILES=	$(BOOT) $(BOOT).x s.x x.s x.o $(BOOT)-$(VERSION){,.tar.gz}

.SUFFIXES: .S

.if 0
# saves about 60bytes...
.c.o:
	@echo ${CC} -fall-bsr ${CFLAGS} ${CPPFLAGS} -c ${.IMPSRC}
	@${CC} ${CFLAGS} ${CPPFLAGS} -S ${.IMPSRC} -o x.s
	@${CC} -c -o x.o x.s
	@${NM} x.o | ${.CURDIR}/all_bsr.sh x.s | ${AS} -o ${.TARGET}
	@rm -f x.s x.o
.endif

.S.o .s.o:
	${CPP} ${CPPFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
		${AS} -o ${.TARGET}

all: $(BOOT)

$(BOOT): $(OBJS)
	@echo loading $(BOOT)
	@$(LD) $(LDFLAGS) -o $(BOOT).x $(OBJS) $(LDADD)
	@if [ `(echo ibase=16; \
		$(NM) $(BOOT).x | sed -n 's/D _edata/-$(BSS)/p' | tr a-f A-F) |\
			bc` -gt 0 ];\
	then	echo $(BOOT): text+data is too large; exit 1; fi
	@cp $(BOOT).x s.x
	@$(XSTRIP) s.x
	@dd bs=32 skip=1 count=256 if=s.x of=$(BOOT) 2> /dev/null
	@rm s.x
	@$(SIZE) $(BOOT).x

#
# installation
#
${DESTDIR}/usr/mdec/sdboot!
	install -c -o bin -g bin -m 444 $(BOOT) ${DESTDIR}/usr/mdec/sdboot

${DESTDIR}/usr/mdec/fdboot:	${DESTDIR}/usr/mdec/sdboot
	rm -f ${DESTDIR}/usr/mdec/fdboot
	ln ${DESTDIR}/usr/mdec/sdboot ${DESTDIR}/usr/mdec/fdboot

install: ${DESTDIR}/usr/mdec/sdboot ${DESTDIR}/usr/mdec/fdboot
	install -c -o bin -g bin -m 755 ${.CURDIR}/writefdboot ${DESTDIR}/usr/mdec

# make package
dist: ${.CURDIR}/$(BOOT)
	rm -f $(BOOT)-$(VERSION); ln -s ${.CURDIR} $(BOOT)-$(VERSION); \
	tar --exclude \*/$(BOOT)-$(VERSION)\* --exclude RCS --exclude tmp \
		--exclude obj --exclude obj.${MACHINE} \
		-czhf $(BOOT)-$(VERSION).tar.gz $(BOOT)-$(VERSION); \
	rm -f $(BOOT)-$(VERSION) ${.CURDIR}/$(BOOT)

${.CURDIR}/$(BOOT): $(BOOT)
	-ln -f $(BOOT) ${.CURDIR}/$(BOOT)	2>/dev/null

.include <bsd.prog.mk>
