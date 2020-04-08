#	$NetBSD: Makefile,v 1.24.14.1 2020/04/08 14:07:57 martin Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BOOT=	boot_ustar
VERSIONFILE=	${.CURDIR}/version
VERSION!=	${TOOL_AWK} -F: '$$1 ~ /^[0-9.]*$$/ { it = $$1; } \
			END { print it }' ${VERSIONFILE}
NEWVERSWHAT=	"${BOOT}"

# text and bss addresses
TEXT=		0x2000		# Primary (me)
BOOT_TEXT=	0x6000		# Secondary (/boot)

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

CPPFLAGS+= -DTEXTADDR="$(TEXT)" -DBOOT_TEXTADDR="$(BOOT_TEXT)"
CPPFLAGS+= -DBOOT_MAXSIZE="$(BOOT_MAXSIZE)"
CPPFLAGS+= -DBOOT=\"$(BOOT)\" -DBOOT_VERS=\"$(VERSION)\"
CPPFLAGS+= -I${COMMONDIR} -I${LIBIOCS} -I${S} -I. -D_STANDALONE
CFLAGS+=   -m68000
AFLAGS=	   ${CFLAGS:M-[ID]*}
AFLAGS+=   -Wa,-march=m68000 -Wa,-mcpu=m68000
LINKFLAGS=   -n -Bstatic -T ${.CURDIR}/${BOOT}.ldscript
LINKFLAGS+=  --defsym=TEXTADDR=$(TEXT)
LINKFLAGS+=  --defsym=BOOT_TEXTADDR=$(BOOT_TEXT)

.include "${.CURDIR}/../Makefile.booters"
.include "${S}/../common/lib/libc/Makefile.inc"


${PROG}: $(OBJS)
	${_MKTARGET_LINK}
	$(LD) $(LINKFLAGS) -o ${PROG} $(OBJS) $(LDADD)

.include <bsd.prog.mk>
