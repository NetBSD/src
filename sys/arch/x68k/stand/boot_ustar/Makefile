#	$NetBSD: Makefile,v 1.1 2001/09/28 15:22:21 minoura Exp $

.include <bsd.own.mk>

BOOT=	boot_ustar
VERSION=1.0

# text and bss addresses in hex
TEXT=		002000		# Primary (me)
BOOT_TEXT=	006000		# Secondary (/boot)

PROG=		$(BOOT)
BINDIR=		/usr/mdec
STRIPFLAG=
BINMODE=	444
MKMAN=		no

OBJCOPY?=	objcopy

COMMONDIR=	${.CURDIR}/../common
LIBIOCS=	${.CURDIR}/../libiocs
#.PATH:		${COMMONDIR}
#SRCS=	boot_ustar.S exec_image.S
SRCS=	boot_ustar.S

CPPFLAGS+= -DTEXTADDR="0x$(TEXT)" -DBOOT_TEXTADDR="0x$(BOOT_TEXT)"
CPPFLAGS+= -DBOOT=\"$(BOOT)\" -DBOOT_VERS=\"$(VERSION)\"
CPPFLAGS+= -I${COMMONDIR} -I${LIBIOCS}
AFLAGS=	   ${CFLAGS:M-[ID]*}
.if ${OBJECT_FMT} == "ELF"
LDFLAGS=   -n -Bstatic -T ${.CURDIR}/${BOOT}.ldscript
LDFLAGS+=  -noinhibit-exec	# XXX
.else
LDFLAGS=   -n -Bstatic -Ttext ${TEXT}
.endif

CLEANFILES=	$(BOOT).x s.x x.s x.o


$(BOOT): $(OBJS)
	$(LD) $(LDFLAGS) -o $(BOOT).x $(OBJS) $(LDADD)
	@$(NM) --target=a.out-m68k-netbsd $(BOOT).x | sed -n '/T first_kbyte/p'
	@if [ `(echo ibase=16; \
		$(NM) --target=a.out-m68k-netbsd $(BOOT).x | sed -n 's/T first_kbyte/-$(TEXT)-400/p' | \
				tr a-f A-F) | bc` -gt 0 ];\
	then echo '$(BOOT): first_kbyte exceeds the first killobyte'; exit 1; fi
	@$(SIZE) --target=a.out-m68k-netbsd $(BOOT).x
	@if [ `(echo ibase=16; \
		$(NM) --target=a.out-m68k-netbsd $(BOOT).x | sed -n 's/D _edata/-$(TEXT)-2000/p' | tr a-f A-F) |\
			bc` -gt 0 ];\
	then	echo '$(BOOT): text+data is too large'; exit 1; fi
	@$(OBJCOPY) -I a.out-m68k-netbsd -O binary $(BOOT).x $(BOOT)

.include <bsd.prog.mk>
