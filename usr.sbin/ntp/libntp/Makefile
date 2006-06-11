#	$NetBSD: Makefile,v 1.8 2006/06/11 19:34:22 kardel Exp $

MKPRIVATELIB=	yes

LIB=	ntp
SRCS=	a_md5encrypt.c adjtime.c atoint.c atolfp.c atouint.c \
	audio.c authkeys.c authreadkeys.c authusekey.c \
	buftvtots.c caljulian.c caltontp.c calyearstart.c \
	clocktime.c clocktypes.c decodenetnum.c dofptoa.c dolfptoa.c \
	emalloc.c findconfig.c fptoa.c fptoms.c getopt.c hextoint.c \
	hextolfp.c humandate.c icom.c inttoa.c iosignal.c lib_strbuf.c \
	machines.c md5c.c \
	memmove.c mfptoa.c mfptoms.c mktime.c modetoa.c mstolfp.c msutotsf.c \
	msyslog.c netof.c ntp_random.c ntp_rfc2553.c numtoa.c numtohost.c \
	octtoint.c prettydate.c \
	refnumtoa.c socktoa.c socktohost.c statestr.c syssignal.c \
	systime.c tsftomsu.c \
	tstotv.c tvtoa.c tvtots.c uglydate.c uinttoa.c utvtoa.c ymd2yd.c \
	recvbuff.c
NTPSRCDIR=lib${LIB}

.include <bsd.lib.mk>
