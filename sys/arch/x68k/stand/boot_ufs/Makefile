#	$NetBSD: Makefile,v 1.26.12.1 2014/08/20 00:03:28 tls Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BOOT=	boot_ufs
VERSIONFILE=	${.CURDIR}/version
VERSION!=	${TOOL_AWK} -F: '$$1 ~ /^[0-9.]*$$/ { it = $$1; } \
			END { print it }' ${VERSIONFILE}
NEWVERSWHAT=	"${BOOT}"

# text and bss addresses in hex
TEXT=		0f0000		# Primary (me)
BOOT_TEXT=	006000		# Secondary (/boot)

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

SRCS+=		vers.c
CLEANFILES+=	vers.c
vers.c:	${VERSIONFILE}
	${_MKTARGET_CREATE}
	${HOST_SH} ${S}/conf/newvers_stand.sh \
	    ${.ALLSRC} ${MACHINE} ${NEWVERSWHAT}

CFLAGS=	-Os -fomit-frame-pointer -fno-unwind-tables
CFLAGS+= -m68020-60
CFLAGS+= -W -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -DTEXTADDR="0x$(TEXT)" -DBOOT_TEXTADDR="0x$(BOOT_TEXT)"
CPPFLAGS+= -DBOOT=\"$(BOOT)\" -DBOOT_VERS=\"$(VERSION)\"
CPPFLAGS+= -DSCSI_ADHOC_BOOTPART
#CPPFLAGS+= -DBOOT_DEBUG
CPPFLAGS+= -DUSE_FFS -DUSE_LFS -DUSE_UFS1 -DUSE_UFS2
CPPFLAGS+= -I${COMMONDIR} -I${LIBIOCS} -I${S} -I. -D_STANDALONE
AFLAGS=	   ${CFLAGS:M-[ID]*}
LINKFLAGS=   -n -Bstatic -T ${.CURDIR}/${BOOT}.ldscript -M
LINKFLAGS+=  -noinhibit-exec	# XXX

.include "${.CURDIR}/../Makefile.booters"
.include "${S}/../common/lib/libc/Makefile.inc"

CLEANFILES+=	$(PROG).x $(PROG).map


$(PROG): $(OBJS)
	${_MKTARGET_LINK}
	:
	: Note:	"relocation truncated to fit: R_68K_16" messages are expected.
	:	Other errors are fatal.
	:
	$(LD) $(LINKFLAGS) -o $(PROG).x $(OBJS) $(LDADD) >$(PROG).map
	@grep first_kbyte $(PROG).map
	@if [ `(echo ibase=16;						\
		${TOOL_SED} -n	-e '/[ 	]first_kbyte/{'			\
			-e 's/.*0x\([0-9a-fA-F]*\).*/\1-$(TEXT)-400/p'	\
			-e '}' $(PROG).map |				\
		tr a-f A-F) | bc` -gt 0 ];				\
	then echo '$(BOOT): first_kbyte exceeds the first killobyte';	\
		exit 1;							\
	fi
	mv -f $(PROG).x $(PROG)

.include <bsd.prog.mk>
