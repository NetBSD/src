# $Id: Makefile.am,v 1.1.1.1.4.2 2000/06/16 18:31:56 thorpej Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += -I$(srcdir)/.. $(INCLUDE_krb4)

bin_PROGRAMS = telnet

CHECK_LOCAL = 

telnet_SOURCES  = authenc.c commands.c main.c network.c ring.c \
		  sys_bsd.c telnet.c terminal.c \
		  utilities.c defines.h externs.h ring.h telnet_locl.h types.h

man_MANS = telnet.1

LDADD = ../libtelnet/libtelnet.a \
	$(LIB_krb5) \
	$(LIB_krb4) \
	$(top_builddir)/lib/des/libdes.la \
	$(LIB_tgetent) \
	$(LIB_roken)
