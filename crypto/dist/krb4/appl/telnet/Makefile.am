# $Id: Makefile.am,v 1.1.1.1.4.2 2000/06/16 18:46:29 thorpej Exp $

include $(top_srcdir)/Makefile.am.common

SUBDIRS = libtelnet telnet telnetd

dist-hook:
	$(mkinstalldirs) $(distdir)/arpa
	$(INSTALL_DATA) $(srcdir)/arpa/telnet.h $(distdir)/arpa

EXTRA_DIST = README.ORIG telnet.state
