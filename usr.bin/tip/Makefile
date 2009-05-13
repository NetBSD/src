#	$NetBSD: Makefile,v 1.27.28.1 2009/05/13 19:20:08 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#
# Files are:
#	/etc/remote		remote host description file
#	/etc/phones		phone number file, owned by ${OWNER} and
#				mode 6??
# Presently supports:
#	BIZCOMP
#	DEC DF02-AC, DF03-AC
#	DEC DN-11/Able Quadracall
#	HAYES and Hayes emulators
#	USR COURIER (2400 baud)
#	VENTEL 212+
#	VADIC 831 RS232 adaptor
#	VADIC 3451
#	TELEBIT T3000
#
# Configuration defines:
#	DF02, DF03, DN11	ACU's supported
#	  BIZ1031, BIZ1022, VENTEL, V831, V3451, HAYES, COURIER, T3000
#	DEFBR			default baud rate to make connection at
#	DEFFS			default frame size for FTP buffering of
#				writes on local side
#	BUFSIZ			buffer sizing from stdio, must be fed
#				explicitly to remcap.c if not 1024

.include <bsd.own.mk>

PROG=	tip
LINKS=	${BINDIR}/tip ${BINDIR}/cu
MLINKS=	tip.1 cu.1
CPPFLAGS+=-I${.CURDIR} \
	-DDEFBR=9600 -DDEFFS=BUFSIZ -DHAYES \
	#-DV831 -DVENTEL -DHAYES -DCOURIER -DT3000
.PATH:	${.CURDIR}/aculib

# Dialers we can choose from:
# biz22.c courier.c df.c dn11.c hayes.c t3000.c v3451.c v831.c ventel.c
DIALSRCS = hayes.c

SRCS=	acu.c acutab.c cmds.c cmdtab.c cu.c hunt.c partab.c \
	remote.c tip.c tipout.c value.c vars.c ${DIALSRCS}

# -- acutab is configuration dependent, and so depends on the Makefile
# -- remote.o depends on the Makefile because of DEFBR and DEFFS
acutab.o remote.o: Makefile

.include <bsd.prog.mk>
