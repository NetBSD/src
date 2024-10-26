BUILDDIR = build
OBJDIR = objects
GENKEY=\"/usr/local/bin/mbedtls_gen_key\"
CERTWRITE=\"/usr/local/bin/mbedtls_cert_write\"

ifndef os
  SYSTEM := $(shell uname -s)
  ifeq ($(SYSTEM), Darwin)
    os=x
  else ifeq ($(SYSTEM), Linux)
    os_id := $(shell sed -n -e 's/^ID=//p' < /etc/os-release)
    ifeq ($(os_id), raspbian)
      os=raspbian
    else
      os=linux
    endif
  endif
endif

ifdef INSTBASE
  INSTALL_PREFIX=$(INSTBASE)
else
  INSTALL_PREFIX=$(DESTDIR)/usr
endif

INSTALL?=install

INCLUDEDIRS=-I../mDNSShared -I../mDNSMacOSX/xpc_services -I../DSO -I../mDNSCore -I../mDNSShared/utilities
ifeq ($(os),x)
SRPCFLAGS = -O0 -g -Wall -Werror -I../mDNSCore -I/usr/local/include -I. -I../mDNSMacOSX/Private $(INCLUDEDIRS) -I../DSO -MMD -MF .depfile-${notdir $@} -DUSE_KQUEUE -DHAVE_SOCKADDR_DL -DGENKEY_PROGRAM=$(GENKEY) -DCERTWRITE_PROGRAM=$(CERTWRITE) -DEXCLUDE_TLS -DPOSIX_BUILD -D__APPLE_USE_RFC_3542 -DSTANDALONE
SRPLDOPTS = -framework CoreServices -framework Security -framework CoreFoundation -framework Network
HMACOBJS     = $(OBJDIR)/hmac-macos.o
SIGNOBJS     = $(OBJDIR)/sign-macos.o $(OBJDIR)/srp-filedata.o
VERIFYOBJS   = $(OBJDIR)/verify-macos.o
TLSOBJS      = $(OBJDIR)/tls-macos.o
IOOBJS       = $(OBJDIR)/ioloop.o $(OBJDIR)/posix.o $(OBJDIR)/ioloop-common.o
IOWOTLSOBJS  = $(OBJDIR)/ioloop.o $(OBJDIR)/posix.o
else ifeq ($(os), linux)
SRPCFLAGS = -DMDNS_UDS_SERVERPATH=\"/var/run/mdnsd\" -O0 -g -Wall -Werror -DSTANDALONE -I../mDNSCore -I/usr/local/include -I. -I../mDNSMacOSX/Private $(INCLUDEDIRS) -I../DSO -MMD -MF .depfile-${notdir $@} -DNOT_HAVE_SA_LEN -DUSE_SELECT -DUSE_INOTIFY -DGENKEY_PROGRAM=$(GENKEY) -DCERTWRITE_PROGRAM=$(CERTWRITE) -DLINUX -DSRP_CRYPTO_MBEDTLS -DPOSIX_BUILD -DMDNS_NO_STRICT
#SRPLDOPTS = /usr/local/lib/libmbedtls.a /usr/local/lib/libmbedx509.a /usr/local/lib/libmbedcrypto.a -lbsd
SRPLDOPTS = -lmbedcrypto -lmbedtls -lmbedx509
HMACOBJS     = $(OBJDIR)/hmac-mbedtls.o
SIGNOBJS     = $(OBJDIR)/sign-mbedtls.o $(OBJDIR)/srp-filedata.o
VERIFYOBJS   = $(OBJDIR)/verify-mbedtls.o
TLSOBJS      = $(OBJDIR)/tls-mbedtls.o
IOOBJS       = $(OBJDIR)/ioloop.o $(OBJDIR)/posix.o $(TLSOBJS) $(OBJDIR)/ioloop-common.o
IOWOTLSOBJS  = $(OBJDIR)/ioloop-notls.o $(OBJDIR)/posix.o
else ifeq ($(os), linux-uclibc)
SRPCFLAGS = -DMDNS_UDS_SERVERPATH=\"/var/run/mdnsd\" -O0 -g -Wall -Werror -DSTANDALONE -I../mDNSCore -I/usr/local/include -I. -I../mDNSMacOSX/Private $(INCLUDEDIRS) -I../DSO -MMD -MF .depfile-${notdir $@} -DNOT_HAVE_SA_LEN -DUSE_SELECT -DLINUX_GETENTROPY -DGENKEY_PROGRAM=$(GENKEY) -DCERTWRITE_PROGRAM=$(CERTWRITE) -DLINUX -DSRP_CRYPTO_MBEDTLS -DPOSIX_BUILD -DMDNS_NO_STRICT
SRPLDOPTS = -lmbedcrypto -lmbedtls -lmbedx509 -lbsd
HMACOBJS     = $(OBJDIR)/hmac-mbedtls.o
SIGNOBJS     = $(OBJDIR)/sign-mbedtls.o $(OBJDIR)/srp-filedata.o
VERIFYOBJS   = $(OBJDIR)/verify-mbedtls.o
TLSOBJS      = $(OBJDIR)/tls-mbedtls.o
IOOBJS       = $(OBJDIR)/ioloop.o $(OBJDIR)/posix.o $(TLSOBJS) $(OBJDIR)/ioloop-common.o
IOWOTLSOBJS  = $(OBJDIR)/ioloop-notls.o $(OBJDIR)/posix.o
else ifeq ($(os), raspbian)
ifdef ASAN
SRPCFLAGS    = -DMDNS_UDS_SERVERPATH=\"/var/run/mdnsd\" -O0 -g -Wall -Werror -DSTANDALONE -I../mDNSCore -I/usr/local/include -I. -I../mDNSMacOSX/Private $(INCLUDEDIRS) -I../DSO -MMD -MF .depfile-${notdir $@} -DNOT_HAVE_SA_LEN -DUSE_SELECT -DGENKEY_PROGRAM=$(GENKEY) -DCERTWRITE_PROGRAM=$(CERTWRITE) -DLINUX -DRPI -DSRP_CRYPTO_MBEDTLS -DPOSIX_BUILD -fsanitize=address -DMDNS_NO_STRICT
SRPLDOPTS    = -lasan -lmbedtls -lmbedx509 -lmbedcrypto -lbsd
else
SRPCFLAGS    = -DMDNS_UDS_SERVERPATH=\"/var/run/mdnsd\" -O0 -g -Wall -Werror -DSTANDALONE -I../mDNSCore -I/usr/local/include -I. -I../mDNSMacOSX/Private $(INCLUDEDIRS) -I../DSO -MMD -MF .depfile-${notdir $@} -DNOT_HAVE_SA_LEN -DUSE_SELECT -DGENKEY_PROGRAM=$(GENKEY) -DCERTWRITE_PROGRAM=$(CERTWRITE) -DLINUX -DRPI -DSRP_CRYPTO_MBEDTLS -DPOSIX_BUILD -DMDNS_NO_STRICT
SRPLDOPTS    = -lmbedtls -lmbedx509 -lmbedcrypto -lbsd
endif
HMACOBJS     = $(OBJDIR)/hmac-mbedtls.o
SIGNOBJS     = $(OBJDIR)/sign-mbedtls.o $(OBJDIR)/srp-filedata.o
VERIFYOBJS   = $(OBJDIR)/verify-mbedtls.o
TLSOBJS      = $(OBJDIR)/tls-mbedtls.o
IOOBJS       = $(OBJDIR)/ioloop.o $(OBJDIR)/posix.o $(TLSOBJS) $(OBJDIR)/ioloop-common.o
IOWOTLSOBJS  = $(OBJDIR)/ioloop-notls.o $(OBJDIR)/posix.o
else
SRPCFLAGS=$(os) $(os_id)
endif

all:	setup $(BUILDDIR)/srp-client $(BUILDDIR)/srp-mdns-proxy $(BUILDDIR)/keydump $(BUILDDIR)/dnssd-proxy $(BUILDDIR)/cti-server $(BUILDDIR)/srputil # $(BUILDDIR)/srp-dns-proxy $(BUILDDIR)/dnssd-relay

install: all
	$(INSTALL) -D $(BUILDDIR)/srp-client $(INSTALL_PREFIX)/sbin/srp-client
	$(INSTALL) -D $(BUILDDIR)/srp-mdns-proxy $(INSTALL_PREFIX)/sbin/srp-mdns-proxy
	$(INSTALL) -D $(BUILDDIR)/keydump $(INSTALL_PREFIX)/bin/keydump
	$(INSTALL) -D $(BUILDDIR)/dnssd-proxy $(INSTALL_PREFIX)/sbin/dnssd-proxy
	$(INSTALL) -D $(BUILDDIR)/srp-dns-proxy $(INSTALL_PREFIX)/sbin/srp-dns-proxy
#	$(INSTALL) -D $(BUILDDIR)/dnssd-relay $(INSTALL_PREFIX)/sbin/dnssd-relay

# 'setup' sets up the build directory structure the way we want
setup:
	@if test ! -d $(OBJDIR)   ; then mkdir -p $(OBJDIR)   ; fi
	@if test ! -d $(BUILDDIR) ; then mkdir -p $(BUILDDIR) ; fi

# clean removes targets and objects
clean:
	@if test -d $(OBJDIR)   ; then rm -r $(OBJDIR)   ; fi
	@if test -d $(BUILDDIR) ; then rm -r $(BUILDDIR) ; fi

SIMPLEOBJS   = $(OBJDIR)/towire.o $(SIGNOBJS)
DSOOBJS      = $(OBJDIR)/dso.o $(OBJDIR)/dso-utils.o
MDNSOBJS     = $(OBJDIR)/dnssd_clientstub.o $(OBJDIR)/dnssd_ipc.o $(OBJDIR)/dnssd_clientlib.o
FROMWIREOBJS = $(OBJDIR)/fromwire.o $(VERIFYOBJS) $(OBJDIR)/wireutils.o
CFOBJS       = $(OBJDIR)/config-parse.o
CTIOBJS      = $(OBJDIR)/cti-services.o $(OBJDIR)/cti-proto.o
$(BUILDDIR)/dnssd-relay:  $(OBJDIR)/dnssd-relay.o $(DSOOBJS) $(IOOBJS) $(CFOBJS)
	$(CC) -o $@ $+ $(SRPLDOPTS)

$(BUILDDIR)/dnssd-proxy:  $(OBJDIR)/dnssd-proxy.o $(SIMPLEOBJS) $(DSOOBJS) $(MDNSOBJS) $(FROMWIREOBJS) $(IOOBJS) $(CFOBJS) $(OBJDIR)/srp-log.o
	$(CC) -o $@ $+ $(SRPLDOPTS)

$(BUILDDIR)/srp-client:	$(OBJDIR)/srp-ioloop.o $(OBJDIR)/srp-client.o $(OBJDIR)/dnssd_clientlib.o $(CTIOBJS) $(SIMPLEOBJS) $(IOWOTLSOBJS) $(CFOBJS) $(DSOOBJS) $(FROMWIREOBJS)
	$(CC) -o $@ $+ $(SRPLDOPTS)

$(BUILDDIR)/srp-dns-proxy:	$(OBJDIR)/srp-dns-proxy.o $(OBJDIR)/srp-parse.o $(SIMPLEOBJS) $(FROMWIREOBJS) $(IOOBJS) $(HMACOBJS) $(CFOBJS)
	$(CC) -o $@ $+ $(SRPLDOPTS)

$(BUILDDIR)/srp-mdns-proxy:	$(OBJDIR)/srp-mdns-proxy.o $(OBJDIR)/srp-parse.o $(OBJDIR)/route.o $(OBJDIR)/omr-watcher.o $(OBJDIR)/adv-ctl-server.o $(OBJDIR)/combined-dnssd-proxy.o $(OBJDIR)/srp-replication.o $(OBJDIR)/srp-log.o $(CTIOBJS) $(MDNSOBJS) $(SIMPLEOBJS) $(DSOOBJS) $(FROMWIREOBJS) $(IOOBJS) $(HMACOBJS) $(CFOBJS)
	$(CC) -o $@ $+ $(SRPLDOPTS)

$(BUILDDIR)/keydump:	$(OBJDIR)/keydump.o $(MDNSOBJS) $(SIMPLEOBJS) $(FROMWIREOBJS) $(IOOBJS)
	$(CC) -o $@ $+ $(SRPLDOPTS)

$(BUILDDIR)/cti-server:	$(OBJDIR)/cti-server.o $(OBJDIR)/cti-proto-noioloop.o
	$(CC) -o $@ $+ $(SRPLDOPTS)

$(BUILDDIR)/srputil:	$(OBJDIR)/srputil.o $(OBJDIR)/advertising_proxy_services.o $(CTIOBJS) $(MDNSOBJS) $(SIMPLEOBJS) $(FROMWIREOBJS) $(IOOBJS)
	$(CC) -o $@ $+ $(SRPLDOPTS)

$(OBJDIR)/dso.o:	../DSO/dso.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -c -I. $(INCLUDEDIRS) $<

$(OBJDIR)/dnssd_clientstub.o:  ../mDNSShared/dnssd_clientstub.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -c -I. $(INCLUDEDIRS) $<

$(OBJDIR)/dnssd_clientlib.o:  ../mDNSShared/dnssd_clientlib.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -c -I. $(INCLUDEDIRS) $<

$(OBJDIR)/dnssd_ipc.o:  ../mDNSShared/dnssd_ipc.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -c -I. $(INCLUDEDIRS) $<

$(OBJDIR)/srputil.o: ../Clients/srputil/srputil.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -c -I. $(INCLUDEDIRS) $<

$(OBJDIR)/ioloop-notls.o: ioloop.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -DEXCLUDE_TLS -DEXCLUDE_DNSSD_TXN_SUPPORT -c  $<

$(OBJDIR)/cti-proto-noioloop.o: cti-proto.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -DNO_IOLOOP -c  $<

$(OBJDIR)/cti-server.o: cti-server.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -DNO_IOLOOP -c  $<

$(OBJDIR)/dnssd-proxy.o: dnssd-proxy.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -DSRP_FEATURE_DYNAMIC_CONFIGURATION=0 -DSRP_FEATURE_COMBINED_SRP_DNSSD_PROXY=0 -DTHREAD_BORDER_ROUTER=0 -DSRP_FEATURE_REPLICATION=0 -c  $<

$(OBJDIR)/combined-dnssd-proxy.o: dnssd-proxy.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -c  $<

$(OBJDIR)/%.o:	%.c
	$(CC) -o $@ $(SRPCFLAGS) $(CFLAGS) -c  $<

-include .depfile-adv-ctl-server.o
-include .depfile-advertising_proxy_services.o
-include .depfile-combined-dnssd-proxy.o
-include .depfile-config-parse.o
-include .depfile-cti-proto-noioloop.o
-include .depfile-cti-proto.o
-include .depfile-cti-server.o
-include .depfile-cti-services.o
-include .depfile-dnssd_clientlib.o
-include .depfile-dnssd_clientstub.o
-include .depfile-dnssd_ipc.o
-include .depfile-dnssd_clientlib.o
-include .depfile-dnssd-proxy.o
-include .depfile-dso.o
-include .depfile-fromwire.o
-include .depfile-hmac-mbedtls.o
-include .depfile-ioloop-common.o
-include .depfile-ioloop-notls.o
-include .depfile-ioloop.o
-include .depfile-keydump.o
-include .depfile-posix.o
-include .depfile-route.o
-include .depfile-sign-mbedtls.o
-include .depfile-srp-client.o
-include .depfile-srp-filedata.o
-include .depfile-srp-ioloop.o
-include .depfile-srp-mdns-proxy.o
-include .depfile-srp-parse.o
-include .depfile-srp-replication.o
-include .depfile-srputil.o
-include .depfile-tls-mbedtls.o
-include .depfile-towire.o
-include .depfile-verify-mbedtls.o
-include .depfile-wireutils.o
