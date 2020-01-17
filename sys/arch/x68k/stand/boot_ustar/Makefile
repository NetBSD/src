#	$NetBSD: Makefile,v 1.24.20.1 2020/01/17 21:47:28 ad Exp $

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

BOOT_MAXSIZE=	65536		# size enough to read /boot whole

PROG=		fd$(BOOT)
BINDIR=		/usr/mdec
STRIPFLAG=
BINMODE=	444

S=		${.CURDIR}/../../../..
M=		$S/arch/x68k
COMMONDIR=	$M/stand/common
LIBIOCS=	$M/stand/libiocs
SRCS=	boot_ustar.S


.include "${S}/conf/newvers_stand.mk"

CPPFLAGS+= -DTEXTADDR="0x$(TEXT)" -DBOOT_TEXTADDR="0x$(BOOT_TEXT)"
CPPFLAGS+= -DBOOT_MAXSIZE="$(BOOT_MAXSIZE)"
CPPFLAGS+= -DBOOT=\"$(BOOT)\" -DBOOT_VERS=\"$(VERSION)\"
CPPFLAGS+= -I${COMMONDIR} -I${LIBIOCS} -I${S} -I. -D_STANDALONE
CFLAGS+=   -m68000
AFLAGS=	   ${CFLAGS:M-[ID]*}
AFLAGS+=   -Wa,-march=m68000 -Wa,-mcpu=m68000
LINKFLAGS=   -n -Bstatic -T ${.CURDIR}/${BOOT}.ldscript -M
LINKFLAGS+=  -noinhibit-exec	# XXX

.include "${.CURDIR}/../Makefile.booters"
.include "${S}/../common/lib/libc/Makefile.inc"

CLEANFILES+=	${PROG}.x $(PROG).map


${PROG}: $(OBJS)
	${_MKTARGET_LINK}
	$(LD) $(LINKFLAGS) -o ${PROG} $(OBJS) $(LDADD) > $(PROG).map
	@grep first_kbyte $(PROG).map
	@if [ `${TOOL_AWK}						   \
	    '/first_kbyte/ {print "eval(eval("$$1")-eval(0x'$(TEXT)'))"}'  \
	    $(PROG).map | ${TOOL_M4} -` -gt 1024 ];			   \
	then echo '$(BOOT): first_kbyte exceeds the first killobyte';	   \
	    rm $(PROG) ; exit 1; 					   \
	fi
	@if [ `${TOOL_AWK}						   \
	    '/_edata/ {print "eval(eval("$$1")-eval(0x'$(TEXT)'))"}'	   \
	    $(PROG).map | ${TOOL_M4} -` -gt 8192 ];			   \
	then echo '$(BOOT): text+data is too large';			   \
	    rm $(PROG) ; exit 1; 					   \
	fi

.include <bsd.prog.mk>
