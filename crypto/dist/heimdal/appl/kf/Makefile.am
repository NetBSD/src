# $Id: Makefile.am,v 1.1.1.1.4.2 2000/06/16 18:31:52 thorpej Exp $

include $(top_srcdir)/Makefile.am.common

noinst_PROGRAMS = kf kfd

kf_SOURCES = kf.c kf_locl.h

kfd_SOURCES = kfd.c kf_locl.h

LDADD = $(top_builddir)/lib/krb5/libkrb5.la \
	$(top_builddir)/lib/des/libdes.la \
	$(top_builddir)/lib/asn1/libasn1.la \
	$(LIB_roken)
