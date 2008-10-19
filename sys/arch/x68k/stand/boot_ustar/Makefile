#	$NetBSD: Makefile,v 1.14 2008/10/19 22:05:22 apb Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BOOT=	boot_ustar
VERSIONFILE=	${.CURDIR}/version
VERSION!=	${TOOL_AWK} -F: '$$1 ~ /^[0-9.]*$$/ { it = $$1; } \
			END { print it }' ${VERSIONFILE}
NEWVERSWHAT=	"${BOOT}"

# text and bss addresses in hex
TEXT=		002000		# Primary (me)
BOOT_TEXT=	006000		# Secondary (/boot)

PROG=		fd$(BOOT)
BINDIR=		/usr/mdec
STRIPFLAG=
BINMODE=	444

S=		${.CURDIR}/../../../..
M=		$S/arch/x68k
COMMONDIR=	$M/stand/common
LIBIOCS=	$M/stand/libiocs
SRCS=	boot_ustar.S

SRCS+=		vers.c
CLEANFILES+=	vers.c
vers.c:	${VERSIONFILE}
	${_MKTARGET_CREATE}
	${HOST_SH} ${S}/conf/newvers_stand.sh -DM ${.ALLSRC} ${MACHINE} ${NEWVERSWHAT}

CPPFLAGS+= -DTEXTADDR="0x$(TEXT)" -DBOOT_TEXTADDR="0x$(BOOT_TEXT)"
CPPFLAGS+= -DBOOT=\"$(BOOT)\" -DBOOT_VERS=\"$(VERSION)\"
CPPFLAGS+= -I${COMMONDIR} -I${LIBIOCS}
AFLAGS=	   ${CFLAGS:M-[ID]*}
AFLAGS+=   -Wa,-mc68000
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=   -n -Bstatic -T ${.CURDIR}/${BOOT}.ldscript -M
LINKFLAGS+=  -noinhibit-exec	# XXX
.else
LINKFLAGS=   -n -Bstatic -Ttext ${TEXT} -M
OBJCOPY?=  objcopy
.endif

CLEANFILES+=	${PROG}.x $(PROG).map


${PROG}: $(OBJS)
	${_MKTARGET_LINK}
	$(LD) $(LINKFLAGS) -o ${PROG} $(OBJS) $(LDADD) > $(PROG).map
	@grep first_kbyte $(PROG).map
.if ${OBJECT_FMT} == "ELF"
	@if [ `(echo ibase=16; 						   \
	    sed -n 's/^.*0x\([0-9a-f]*\).* first_kbyte$$/\1-$(TEXT)-400/p' \
		$(PROG).map | 						   \
	    tr a-f A-F) | bc` -gt 0 ]; 					   \
	then echo '$(BOOT): first_kbyte exceeds the first killobyte';	   \
	    rm $(PROG) ; exit 1; 					   \
	fi
	@if [ `(echo ibase=16; 						   \
	    sed -n 's/^.*0x\([0-9a-f]*\).* _edata *= *\.$$/\1-$(TEXT)-2000/p' \
		$(PROG).map | 						   \
	    tr a-f A-F) | bc` -gt 0 ]; 					   \
	then echo '$(BOOT): text+data is too large';			   \
	    rm $(PROG) ; exit 1; 					   \
	fi
.else
	mv $(PROG) $(PROG).x
	$(OBJCOPY) -I a.out-m68k-netbsd -O binary $(PROG).x $(PROG)
	@rm -f $(PROG).x
	@if [ `(echo ibase=16; 						   \
	    sed -n 's/  first_kbyte:.*0x\(.*\),.*$$/\1-$(TEXT)-400/p'	   \
		$(PROG).map | 						   \
	    tr a-f A-F) | bc` -gt 0 ]; 					   \
	then echo '$(BOOT): first_kbyte exceeds the first killobyte';	   \
	    rm $(PROG) ; exit 1; 					   \
	fi
	@if [ `(echo ibase=16; 						   \
	    sed -n 's/  _edata:.*0x\(.*\),.*$$/\1-$(TEXT)-2000/p'	   \
		$(PROG).map | 						   \
	    tr a-f A-F) | bc` -gt 0 ]; 					   \
	then echo '$(BOOT): text+data is too large';			   \
	    rm $(PROG) ; exit 1; 					   \
	fi
.endif

.include <bsd.prog.mk>
