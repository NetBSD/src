# $Id: Makefile.am,v 1.1.1.1.4.2 2000/06/16 18:46:33 thorpej Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += -I$(srcdir)/.. $(INCLUDE_krb4)

libexec_PROGRAMS = telnetd

CHECK_LOCAL = 

telnetd_SOURCES  = telnetd.c state.c termstat.c slc.c sys_term.c \
		   utility.c global.c authenc.c defs.h ext.h telnetd.h

LDADD = \
	../libtelnet/libtelnet.a \
	$(LIB_krb5) \
	$(LIB_krb4) \
	$(top_builddir)/lib/des/libdes.la \
	$(LIB_tgetent) \
	$(LIB_logwtmp) \
	$(LIB_roken)
