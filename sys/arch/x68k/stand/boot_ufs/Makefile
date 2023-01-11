#	$NetBSD: Makefile,v 1.39 2023/01/11 09:35:06 isaki Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BOOT=	boot_ufs
VERSIONFILE=	${.CURDIR}/version
VERSION!=	${TOOL_AWK} -F: '$$1 ~ /^[0-9.]*$$/ { it = $$1; } \
			END { print it }' ${VERSIONFILE}
NEWVERSWHAT=	"${BOOT}"

# text and bss addresses
TEXT=		0x003000	# Primary (me)
BOOT_TEXT=	0x006000	# Secondary (/boot)

PROG=		xx$(BOOT)
LINKS=		${BINDIR}/xx$(BOOT) ${BINDIR}/sd$(BOOT)
LINKS+=		${BINDIR}/xx$(BOOT) ${BINDIR}/fd$(BOOT)
BINDIR=		/usr/mdec
STRIPFLAG=
BINMODE=	444

S=		${.CURDIR}/../../../..
M=		$S/arch/${MACHINE}
LIBKERN=	$S/lib/libkern/arch/${MACHINE_ARCH}
COMMONDIR=	$M/stand/common
LIBIOCS=	$M/stand/libiocs
.PATH:		${LIBKERN} ${COMMONDIR}
SRCS=	boot.S bootmain.c readufs.c readufs_ffs.c readufs_lfs.c
SRCS+=	exec_image.S memset.S strcmp.S

.PATH:	${S}/../common/lib/libc/quad
SRCS+=	udivdi3.c umoddi3.c qdivrem.c

.PATH:	${.CURDIR}/../xxboot
SRCS+=	ashldi3.S

.include "${S}/conf/newvers_stand.mk"

CFLAGS=	-Os -fomit-frame-pointer -fno-unwind-tables
CFLAGS+= -m68020-60
CFLAGS+= -Wextra -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -DTEXTADDR="$(TEXT)" -DBOOT_TEXTADDR="$(BOOT_TEXT)"
CPPFLAGS+= -DBOOT=\"$(BOOT)\" -DBOOT_VERS=\"$(VERSION)\"
CPPFLAGS+= -DSCSI_ADHOC_BOOTPART
#CPPFLAGS+= -DBOOT_DEBUG
CPPFLAGS+= -DUSE_FFS -DUSE_LFS -DUSE_UFS1 -DUSE_UFS2
CPPFLAGS+= -I${COMMONDIR} -I${LIBIOCS} -I${S} -I. -D_STANDALONE
AFLAGS=	   ${CFLAGS:M-[ID]*}
LINKFLAGS=   -n -Bstatic -T ${.CURDIR}/${BOOT}.ldscript
LINKFLAGS+=  --defsym=TEXTADDR=$(TEXT)
LINKFLAGS+=  --defsym=BOOT_TEXTADDR=$(BOOT_TEXT)

.include "${.CURDIR}/../Makefile.booters"
.include "${S}/../common/lib/libc/Makefile.inc"


$(PROG): $(OBJS)
	${_MKTARGET_LINK}
	$(LD) $(LINKFLAGS) -o $(PROG) $(OBJS) $(LDADD)

.include <bsd.prog.mk>
