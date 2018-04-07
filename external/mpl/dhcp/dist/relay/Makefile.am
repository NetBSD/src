AM_CPPFLAGS = -DLOCALSTATEDIR='"@localstatedir@"'

sbin_PROGRAMS = dhcrelay
dhcrelay_SOURCES = dhcrelay.c
dhcrelay_LDADD = ../common/libdhcp.@A@ ../omapip/libomapi.@A@ \
		 @BINDLIBIRSDIR@/libirs.@A@ \
		 @BINDLIBDNSDIR@/libdns.@A@ \
		 @BINDLIBISCCFGDIR@/libisccfg.@A@ \
		 @BINDLIBISCDIR@/libisc.@A@
man_MANS = dhcrelay.8
EXTRA_DIST = $(man_MANS)

