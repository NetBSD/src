#	$NetBSD: Makefile.inc,v 1.1.8.1 1997/11/18 02:50:15 mellon Exp $

BINDIR=		/usr/mdec

# Load at 0x260000 + 2800 * 512
# That way the boot code will be at the correct address if the floppy
# image is started from memory.
RELOC=		3BE000

CLEANFILES+=	${PROG}.sym ${PROG}

CPPFLAGS+=	-DSCNCONSOLE -DSTANDALONE

# Limit the alloc() heap to below 4mb
CPPFLAGS+=	-DHEAP_LIMIT=0x400000

COMMONSOURCE=		srt0.S conf.c cons.c devopen.c filesystem.c \
			machdep.c prf.c
DRIVERSOURCE=		md.c scn.c scsi_hi.c scsi_low.c sd.c
