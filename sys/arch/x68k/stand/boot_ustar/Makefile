#	$NetBSD: Makefile,v 1.4 2001/10/15 16:23:01 minoura Exp $

.include <bsd.own.mk>

BOOT=	boot_ustar
VERSIONFILE=	${.CURDIR}/version
VERSION!=	awk -F: '$$1 ~ /^[0-9.]*$$/ { it = $$1; } END { print it }' ${VERSIONFILE}
NEWVERSWHAT=	"${BOOT}"

# text and bss addresses in hex
TEXT=		002000		# Primary (me)
BOOT_TEXT=	006000		# Secondary (/boot)

PROG=		fd$(BOOT)
BINDIR=		/usr/mdec
STRIPFLAG=
BINMODE=	444
MKMAN=		no

OBJCOPY?=	objcopy

S=		${.CURDIR}/../../../..
M=		$S/arch/x68k
COMMONDIR=	$M/stand/common
LIBIOCS=	$M/stand/libiocs
SRCS=	boot_ustar.S

SRCS+=		vers.c
CLEANFILES+=	vers.c
vers.c:	${VERSIONFILE}
	sh ${S}/conf/newvers_stand.sh -DM ${.ALLSRC} ${MACHINE} ${NEWVERSWHAT}

CPPFLAGS+= -DTEXTADDR="0x$(TEXT)" -DBOOT_TEXTADDR="0x$(BOOT_TEXT)"
CPPFLAGS+= -DBOOT=\"$(BOOT)\" -DBOOT_VERS=\"$(VERSION)\"
CPPFLAGS+= -I${COMMONDIR} -I${LIBIOCS}
AFLAGS=	   ${CFLAGS:M-[ID]*}
AFLAGS+=   -Wa,-mc68000
.if ${OBJECT_FMT} == "ELF"
LDFLAGS=   -n -Bstatic -T ${.CURDIR}/${BOOT}.ldscript
LDFLAGS+=  -noinhibit-exec	# XXX
.else
LDFLAGS=   -n -Bstatic -Ttext ${TEXT}
.endif

CLEANFILES=	${PROG}.x s.x x.s x.o


${PROG}: $(OBJS)
	$(LD) $(LDFLAGS) -o ${PROG}.x $(OBJS) $(LDADD)
	@$(NM) --target=a.out-m68k-netbsd ${PROG}.x | sed -n '/T first_kbyte/p'
	@if [ `(echo ibase=16; \
		$(NM) --target=a.out-m68k-netbsd ${PROG}.x | sed -n 's/T first_kbyte/-$(TEXT)-400/p' | \
				tr a-f A-F) | bc` -gt 0 ];\
	then echo '${PROG}: first_kbyte exceeds the first killobyte'; exit 1; fi
	@$(SIZE) --target=a.out-m68k-netbsd ${PROG}.x
	@if [ `(echo ibase=16; \
		$(NM) --target=a.out-m68k-netbsd ${PROG}.x | sed -n 's/D _edata/-$(TEXT)-2000/p' | tr a-f A-F) |\
			bc` -gt 0 ];\
	then	echo '${PROG}: text+data is too large'; exit 1; fi
	@$(OBJCOPY) -I a.out-m68k-netbsd -O binary ${PROG}.x ${PROG}

.include <bsd.prog.mk>
