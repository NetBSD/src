#	$NetBSD: Makefile,v 1.1 2000/03/29 13:13:43 simonb Exp $

LIB=	ntp
SRCS=	a_md5encrypt.c adjtime.c atoint.c atolfp.c atouint.c \
	authencrypt.c authkeys.c authparity.c authreadkeys.c authusekey.c \
	buftvtots.c caljulian.c calleapwhen.c caltontp.c calyearstart.c \
	clocktime.c clocktypes.c decodenetnum.c dofptoa.c dolfptoa.c \
	emalloc.c findconfig.c fptoa.c fptoms.c getopt.c hextoint.c \
	hextolfp.c humandate.c inttoa.c lib_strbuf.c machines.c md5c.c \
	memmove.c mfptoa.c mfptoms.c modetoa.c mstolfp.c msutotsf.c \
	msyslog.c netof.c numtoa.c numtohost.c octtoint.c prettydate.c \
	ranny.c refnumtoa.c statestr.c syssignal.c systime.c tsftomsu.c \
	tstotv.c tvtoa.c tvtots.c uglydate.c uinttoa.c utvtoa.c ymd2yd.c \
	mfp_mul.c binio.c ieee754io.c gpstolfp.c recvbuff.c iosignal.c \
	icom.c audio.c
NTPSRCDIR=lib${LIB}

MKLINKLIB=no
MKMAN=	no
MKPIC=	no
MKLINT=	no	# XXX

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/lib/libntp/Makefile.frag"
.endif

.include <bsd.lib.mk>
