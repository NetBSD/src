# $Id: Makefile.am,v 1.1.1.1 2000/06/16 18:46:24 thorpej Exp $

include $(top_srcdir)/Makefile.am.common

bin_PROGRAMS = otp otpprint
bin_SUIDS = otp
otp_SOURCES = otp.c otp_locl.h
otpprint_SOURCES = otpprint.c otp_locl.h

LDADD = \
	$(top_builddir)/lib/otp/libotp.la \
	$(top_builddir)/lib/des/libdes.la \
	$(LIB_roken) \
	$(DBLIB)
