include $(top_srcdir)/Makefile.top

lib_LTLIBRARIES = libdns.la

nodist_libdns_ladir = $(includedir)/dns
nodist_libdns_la_HEADERS =		\
	include/dns/enumclass.h		\
	include/dns/enumtype.h		\
	include/dns/rdatastruct.h

nodist_libdns_la_SOURCES =		\
	$(nodist_libdns_la_HEADERS)	\
	code.h

BUILT_SOURCES =				\
	$(nodist_libdns_la_SOURCES)

CLEANFILES =				\
	$(nodist_libdns_la_SOURCES)	\
	gen$(BUILD_EXEEXT)

gen$(BUILD_EXEEXT): gen.c
	$(CC_FOR_BUILD) -g -I. $(srcdir)/gen.c -o $@

EXTRA_DIST =		\
	dnstap.proto	\
	gen.c		\
	rdata/*

include/dns/enumtype.h: gen Makefile
	mkdir -p include/dns
	$(builddir)/gen -s $(srcdir) -t > $@

include/dns/enumclass.h: gen Makefile
	mkdir -p include/dns
	$(builddir)/gen -s $(srcdir) -c > $@

include/dns/rdatastruct.h: gen rdata/rdatastructpre.h rdata/rdatastructsuf.h Makefile
	mkdir -p include/dns
	$(builddir)/gen -s $(srcdir) -i \
		-P $(srcdir)/rdata/rdatastructpre.h \
		-S $(srcdir)/rdata/rdatastructsuf.h > $@

code.h: gen Makefile
	$(builddir)/gen -s $(srcdir) > $@

libdns_ladir = $(includedir)/dns
libdns_la_HEADERS =			\
	include/dns/acl.h		\
	include/dns/adb.h		\
	include/dns/badcache.h		\
	include/dns/bit.h		\
	include/dns/byaddr.h		\
	include/dns/cache.h		\
	include/dns/callbacks.h		\
	include/dns/catz.h		\
	include/dns/cert.h		\
	include/dns/client.h		\
	include/dns/clientinfo.h	\
	include/dns/compress.h		\
	include/dns/db.h		\
	include/dns/dbiterator.h	\
	include/dns/diff.h		\
	include/dns/dispatch.h		\
	include/dns/dlz.h		\
	include/dns/dlz_dlopen.h	\
	include/dns/dns64.h		\
	include/dns/dnsrps.h		\
	include/dns/dnssec.h		\
	include/dns/ds.h		\
	include/dns/dsdigest.h		\
	include/dns/dnstap.h		\
	include/dns/dyndb.h		\
	include/dns/ecs.h		\
	include/dns/edns.h		\
	include/dns/events.h		\
	include/dns/fixedname.h		\
	include/dns/forward.h		\
	include/dns/geoip.h		\
	include/dns/ipkeylist.h		\
	include/dns/iptable.h		\
	include/dns/journal.h		\
	include/dns/kasp.h		\
	include/dns/keydata.h		\
	include/dns/keyflags.h		\
	include/dns/keymgr.h		\
	include/dns/keytable.h		\
	include/dns/keyvalues.h		\
	include/dns/librpz.h		\
	include/dns/lookup.h		\
	include/dns/log.h		\
	include/dns/master.h		\
	include/dns/masterdump.h	\
	include/dns/message.h		\
	include/dns/name.h		\
	include/dns/ncache.h		\
	include/dns/nsec.h		\
	include/dns/nsec3.h		\
	include/dns/nta.h		\
	include/dns/opcode.h		\
	include/dns/order.h		\
	include/dns/peer.h		\
	include/dns/private.h		\
	include/dns/rbt.h		\
	include/dns/rcode.h		\
	include/dns/rdata.h		\
	include/dns/rdataclass.h	\
	include/dns/rdatalist.h		\
	include/dns/rdataset.h		\
	include/dns/rdatasetiter.h	\
	include/dns/rdataslab.h		\
	include/dns/rdatatype.h		\
	include/dns/request.h		\
	include/dns/resolver.h		\
	include/dns/result.h		\
	include/dns/rootns.h		\
	include/dns/rpz.h		\
	include/dns/rriterator.h	\
	include/dns/rrl.h		\
	include/dns/sdb.h		\
	include/dns/sdlz.h		\
	include/dns/secalg.h		\
	include/dns/secproto.h		\
	include/dns/soa.h		\
	include/dns/ssu.h		\
	include/dns/stats.h		\
	include/dns/time.h		\
	include/dns/transport.h		\
	include/dns/tkey.h		\
	include/dns/tsec.h		\
	include/dns/tsig.h		\
	include/dns/ttl.h		\
	include/dns/types.h		\
	include/dns/update.h		\
	include/dns/validator.h		\
	include/dns/view.h		\
	include/dns/xfrin.h		\
	include/dns/zone.h		\
	include/dns/zonekey.h		\
	include/dns/zoneverify.h	\
	include/dns/zt.h

dstdir = $(includedir)/dst
dst_HEADERS =				\
	include/dst/dst.h		\
	include/dst/gssapi.h

libdns_la_SOURCES =			\
	$(libdns_la_HEADERS)		\
	$(dst_HEADERS)			\
	acl.c				\
	adb.c				\
	badcache.c			\
	byaddr.c			\
	cache.c				\
	callbacks.c			\
	catz.c				\
	clientinfo.c			\
	compress.c			\
	db.c				\
	dbiterator.c			\
	diff.c				\
	dispatch.c			\
	dlz.c				\
	dns64.c				\
	dnsrps.c			\
	dnssec.c			\
	ds.c				\
	dst_api.c			\
	dst_internal.h			\
	dst_openssl.h			\
	dst_parse.c			\
	dst_parse.h			\
	dyndb.c				\
	ecs.c				\
	fixedname.c			\
	forward.c			\
	gssapictx.c			\
	hmac_link.c			\
	ipkeylist.c			\
	iptable.c			\
	journal.c			\
	kasp.c				\
	key.c				\
	keydata.c			\
	keymgr.c			\
	keytable.c			\
	log.c				\
	lookup.c			\
	master.c			\
	masterdump.c			\
	message.c			\
	name.c				\
	ncache.c			\
	nsec.c				\
	nsec3.c				\
	nta.c				\
	openssl_link.c			\
	openssl_shim.c			\
	openssl_shim.h			\
	openssldh_link.c		\
	opensslecdsa_link.c		\
	openssleddsa_link.c		\
	opensslrsa_link.c		\
	order.c				\
	peer.c				\
	private.c			\
	rbt.c				\
	rbtdb.h				\
	rbtdb.c				\
	rcode.c				\
	rdata.c				\
	rdatalist.c			\
	rdataset.c			\
	rdatasetiter.c			\
	rdataslab.c			\
	request.c			\
	resolver.c			\
	result.c			\
	rootns.c			\
	rpz.c				\
	rrl.c				\
	rriterator.c			\
	sdb.c				\
	sdlz.c				\
	soa.c				\
	ssu.c				\
	ssu_external.c			\
	stats.c				\
	time.c				\
	transport.c			\
	tkey.c				\
	tsec.c				\
	tsig.c				\
	ttl.c				\
	update.c			\
	validator.c			\
	view.c				\
	xfrin.c				\
	zone.c				\
	zoneverify.c			\
	zonekey.c			\
	zt.c				\
	client.c			\
	rdatalist_p.h			\
	tsig_p.h			\
	zone_p.h

if HAVE_GSSAPI
libdns_la_SOURCES +=			\
	gssapi_link.c
endif

if HAVE_GEOIP2
libdns_la_SOURCES += \
	geoip2.c
endif

libdns_la_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(LIBDNS_CFLAGS)	\
	$(LIBISC_CFLAGS)	\
	$(LIBUV_CFLAGS)		\
	$(OPENSSL_CFLAGS)

libdns_la_LDFLAGS =		\
	$(AM_LDFLAGS)		\
	-release "$(PACKAGE_VERSION)"

libdns_la_LIBADD =		\
	$(LIBISC_LIBS)		\
	$(LIBUV_LIBS)		\
	$(OPENSSL_LIBS)

if HAVE_JSON_C
libdns_la_CPPFLAGS +=		\
	$(JSON_C_CFLAGS)

libdns_la_LIBADD +=		\
	$(JSON_C_LIBS)
endif HAVE_JSON_C

if HAVE_LIBXML2
libdns_la_CPPFLAGS +=		\
	$(LIBXML2_CFLAGS)

libdns_la_LIBADD +=		\
	$(LIBXML2_LIBS)
endif HAVE_LIBXML2

if HAVE_GSSAPI
libdns_la_CPPFLAGS +=		\
	$(GSSAPI_CFLAGS)	\
	$(KRB5_CFLAGS)
libdns_la_LIBADD +=		\
	$(GSSAPI_LIBS)		\
	$(KRB5_LIBS)
endif

if HAVE_GEOIP2
libdns_la_CPPFLAGS +=		\
	$(MAXMINDDB_CFLAGS)
libdns_la_LIBADD +=		\
	$(MAXMINDDB_LIBS)
endif

if HAVE_DNSTAP
nodist_libdns_la_SOURCES +=	\
	dnstap.pb-c.h		\
	dnstap.pb-c.c

libdns_la_SOURCES +=		\
	dnstap.c

dnstap.pb-c.h dnstap.pb-c.c: dnstap.proto
	$(PROTOC_C) --proto_path=$(srcdir) --c_out=. dnstap.proto

libdns_la_CPPFLAGS += $(DNSTAP_CFLAGS)
libdns_la_LIBADD += $(DNSTAP_LIBS)
endif

if HAVE_LMDB
libdns_la_CPPFLAGS += $(LMDB_CFLAGS)
libdns_la_LIBADD += $(LMDB_LIBS)
endif
