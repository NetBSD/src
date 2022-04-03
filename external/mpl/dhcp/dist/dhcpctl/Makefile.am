BINDLIBIRSDIR=@BINDLIBIRSDIR@
BINDLIBDNSDIR=@BINDLIBDNSDIR@
BINDLIBISCCFGDIR=@BINDLIBISCCFGDIR@
BINDLIBISCDIR=@BINDLIBISCDIR@

bin_PROGRAMS = omshell
lib_LIBRARIES = libdhcpctl.a
noinst_PROGRAMS = cltest cltest2
man_MANS = omshell.1 dhcpctl.3
EXTRA_DIST = $(man_MANS)

omshell_SOURCES = omshell.c
omshell_LDADD = libdhcpctl.a ../common/libdhcp.a ../omapip/libomapi.a \
		$(BINDLIBIRSDIR)/libirs.a \
		$(BINDLIBDNSDIR)/libdns.a \
	        $(BINDLIBISCCFGDIR)/libisccfg.a \
		$(BINDLIBISCDIR)/libisc.a

libdhcpctl_a_SOURCES = dhcpctl.c callback.c remote.c

cltest_SOURCES = cltest.c
cltest_LDADD = libdhcpctl.a ../common/libdhcp.a ../omapip/libomapi.a \
	       $(BINDLIBIRSDIR)/libirs.a \
	       $(BINDLIBDNSDIR)/libdns.a \
	       $(BINDLIBISCCFGDIR)/libisccfg.a \
	       $(BINDLIBISCDIR)/libisc.a

cltest2_SOURCES = cltest2.c
cltest2_LDADD = libdhcpctl.a ../common/libdhcp.a ../omapip/libomapi.a \
	       $(BINDLIBIRSDIR)/libirs.a \
	       $(BINDLIBDNSDIR)/libdns.a \
	       $(BINDLIBISCCFGDIR)/libisccfg.a \
	       $(BINDLIBISCDIR)/libisc.a
