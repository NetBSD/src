#	$NetBSD: Makefile.inc,v 1.9.2.1 2000/12/13 22:51:51 he Exp $

BINDIR=			/usr/mdec

# RELOC=FFF00000 allows for boot prog up to FF000 (1044480) bytes long
RELOC=	FFF00000

CPPFLAGS+=		-DDCACONSOLE -DAPCICONSOLE -DITECONSOLE -DDCMCONSOLE
CPPFLAGS+=		-DHIL_KEYBOARD -DUK_KEYBOARD
CPPFLAGS+=		-DDOMAIN_KEYBOARD
CPPFLAGS+=		-D_STANDALONE -DCOMPAT_NOLABEL -Dhp300
CPPFLAGS+=		-DCOMPAT_UFS -DNO_LSEEK

# Limit the alloc() heap to below the msgbuf and ROM scratch pages.
CPPFLAGS+=		-DHEAP_LIMIT="0xffffdfff"

COMMONSOURCE=		srt0.S autoconf.c clock.c conf.c cons.c devopen.c \
			machdep.c prf.c rawfs.c netio.c
DRIVERSOURCE=		apci.c ct.c dca.c dcm.c dnkbd.c fhpib.c hil.c \
			hpib.c if_le.c ite.c ite_dv.c ite_gb.c ite_rb.c \
			ite_subr.c ite_tc.c ite_hy.c kbd.c kbdconf.c \
			nhpib.c rd.c scsi.c sd.c

.ifnmake(print-objdir)
MKBOOTOBJDIR!=		cd ${.CURDIR}/../mkboot ; ${MAKE} print-objdir
MKBOOT_PROG=		${MKBOOTOBJDIR}/mkboot
.endif

# Make sure ${MKBOOT_PROG} is always available
${MKBOOT_PROG}:
	@cd ${MKBOOTDIR} && make depend && make

release:
