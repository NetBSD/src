#	$NetBSD: Makefile.inc,v 1.6 1997/04/27 20:29:43 thorpej Exp $

BINDIR=			/usr/mdec

# RELOC=FFF00000 allows for boot prog up to FF000 (1044480) bytes long
RELOC=	FFF00000

CPPFLAGS+=		-DDCACONSOLE -DITECONSOLE -DDCMCONSOLE
CPPFLAGS+=		-DHIL_KEYBOARD -DUK_KEYBOARD
CPPFLAGS+=		-DDOMAIN_KEYBOARD
CPPFLAGS+=		-DSTANDALONE -DCOMPAT_NOLABEL -Dhp300
CPPFLAGS+=		-DCOMPAT_UFS -DNO_LSEEK

# Limit the alloc() heap to below the msgbuf and ROM scratch pages.
CPPFLAGS+=		-DHEAP_LIMIT="0xffffdfff"

COMMONSOURCE=		srt0.S autoconf.c clock.c conf.c cons.c devopen.c \
			machdep.c prf.c rawfs.c netio.c
DRIVERSOURCE=		ct.c dca.c dcm.c dnkbd.c fhpib.c hil.c hpib.c if_le.c \
			ite.c ite_dv.c ite_gb.c ite_rb.c ite_subr.c \
			ite_tc.c ite_hy.c kbd.c kbdconf.c nhpib.c rd.c \
			scsi.c sd.c

### Figure out what to use for mkboot
MKBOOTDIR=		${.CURDIR}/../mkboot

.if exists(${MKBOOTDIR}/obj.${MACHINE})
MKBOOTOBJDIR=		${MKBOOTDIR}/obj.${MACHINE}
.elif exists (${MKBOOTDIR}/obj)
MKBOOTOBJDIR=		${MKBOOTDIR}/obj
.else
MKBOOTOBJDIR=		${MKBOOTDIR}
.endif

MKBOOT_PROG=		${MKBOOTOBJDIR}/mkboot

# Make sure ${MKBOOT_PROG} is always available
${MKBOOT_PROG}:
	@cd ${MKBOOTDIR} && make depend && make
