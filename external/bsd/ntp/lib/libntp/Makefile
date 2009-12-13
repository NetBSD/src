#	$NetBSD: Makefile,v 1.1 2009/12/13 17:13:47 kardel Exp $

LIBISPRIVATE=yes

LIB=ntp

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/libntp

.PATH.c:	${DIST}

SRCS=systime.c a_md5encrypt.c adjtime.c atoint.c atolfp.c atouint.c audio.c \
     authkeys.c authreadkeys.c authusekey.c bsd_strerror.c buftvtots.c \
     caljulian.c caltontp.c calyearstart.c clocktime.c clocktypes.c \
     decodenetnum.c dofptoa.c dolfptoa.c emalloc.c findconfig.c fptoa.c \
     fptoms.c getopt.c hextoint.c hextolfp.c humandate.c icom.c inttoa.c \
     iosignal.c lib_strbuf.c machines.c memmove.c mfptoa.c ntp_lineedit.c \
     mfptoms.c mktime.c modetoa.c mstolfp.c msutotsf.c msyslog.c netof.c \
     ntp_rfc2553.c numtoa.c numtohost.c octtoint.c prettydate.c ntp_random.c \
     recvbuff.c refnumtoa.c snprintf.c socktoa.c socktohost.c ssl_init.c \
     statestr.c strdup.c strstr.c syssignal.c tsftomsu.c tstotv.c tvtots.c \
     uglydate.c uinttoa.c ymd2yd.c 
#     net.c netaddr.c netscope.c \
#     ondestroy.c random.c result.c stdio.c stdtime.c strerror.c task.c \
#     thread.c time.c sockaddr.c
# not found in libparse
# assertions.c
# buffer.c
# conditions.c
# dir.c
# error.c 
# errno2result.c
# event.c
# file.c
# inet_ntop.c
# inet_pton.c
# interfaceiter.c
# lib.c
# log.c
# md5.c   
# msgcat.c 
.include <bsd.lib.mk>
