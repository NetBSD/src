#	$NetBSD: Makefile,v 1.9 1999/07/12 22:05:42 thorpej Exp $

SRCTOP=	../..
.include <bsd.crypto.mk>

LIB=	ntp
SRCS=	utvtoa.c uinttoa.c uglydate.c tvtots.c tvtoa.c tstotv.c \
tsftomsu.c systime.c syssignal.c statestr.c refnumtoa.c ranny.c \
prettydate.c octtoint.c numtohost.c numtoa.c netof.c msyslog.c \
msutotsf.c mstolfp.c modetoa.c mfptoms.c mfptoa.c mexit.c \
memmove.c machines.c lib_strbuf.h lib_strbuf.c inttoa.c humandate.c \
hextolfp.c hextoint.c getopt.c fptoms.c fptoa.c findconfig.c \
emalloc.c dolfptoa.c dofptoa.c decodenetnum.c clocktypes.c clocktime.c \
calyearstart.c caltontp.c calleapwhen.c caljulian.c buftvtots.c authusekey.c \
authreadkeys.c authparity.c authkeys.c authencrypt.c authdes.c authdecrypt.c \
auth12crypt.c atouint.c atolfp.c atoint.c adjtime.c a_md5encrypt.c \
a_md5decrypt.c a_md512crypt.c 
SRCS += version.c

CPPFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}/../../usr.sbin/xntp/include

.if defined(CRYPTOPATH)
.include "${CRYPTOPATH}/lib/libntp/Makefile.frag"
.endif

.include <bsd.lib.mk>
