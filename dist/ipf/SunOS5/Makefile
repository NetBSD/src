#
# Copyright (C) 1993-1998 by Darren Reed.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and due credit is given
# to the original author and the contributors.
#
BINDEST=/usr/local/bin
SBINDEST=/sbin
MANDIR=/usr/share/man
CC=cc
CFLAGS=-I$(TOP)
#
# For SunOS 5.x
#
PKGDIR=$(ROOT)/opt/ipf
PKGMAN=$(PKGDIR)/man
PKGBIN=$(PKGDIR)/bin
#
TOP=..
TMP=/tmp
CPUDIR=`uname -p`-`uname -r`
CC=gcc -Wall
DEBUG=-g
LIBS=-lsocket -lnsl -lelf
DEF=-D_KERNEL -DSUNDDI
ATON=-DNEED_INET_ATON
ROOT=$(CPUDIR)/root
ROOTINC=$(ROOT)/usr/include
#
MFLAGS="BINDEST=$(BINDEST)" "SBINDEST=$(SBINDEST)" "MANDIR=$(MANDIR)" \
        'CFLAGS=$(CFLAGS) $(ARCHINC) $(SOLARIS2)' "IPFLKM=$(IPFLKM)" \
        "IPFLOG=$(IPFLOG)" "LOGFAC=$(LOGFAC)" "POLICY=$(POLICY)" \
        "SOLARIS2=$(SOLARIS2)" "DEBUG=$(DEBUG)" "CPUDIR=$(CPUDIR)"
#
########## ########## ########## ########## ########## ########## ##########
#
CP=/bin/cp
RM=/bin/rm
CHMOD=/bin/chmod
INSTALL=$(TOP)/bsdinstall
#
DFLAGS=$(IPFLKM) $(IPFLOG) $(DEF) -DIPFILTER_LOG $(SOLARIS2)
#-DIPFDEBUG
MODOBJS=ip_sfil.o fil_k.o solaris.o ip_state.o ip_frag.o ip_nat.o ip_proxy.o \
	ip_auth.o ip_log.o
IPF=ipf.o parse.o opt.o inet_addr.o facpri.o
IPT=ipt.o parse.o fil.o ipft_sn.o ipft_ef.o ipft_td.o ipft_pc.o opt.o \
    ipft_tx.o misc.o ip_state_u.o ip_frag_u.o ip_nat_u.o inet_addr.o \
    ip_fil_u.o ipft_hx.o ip_proxy_u.o ip_auth_u.o natparse.o facpri.o
FILS=fils.o parse.o kmem.o opt.o inet_addr.o facpri.o

sunos5 solaris2 build: ipf.exe ipfstat ipftest ipmon ipnat ipf

pkg: ipf.pkg

ipfstat: $(FILS)
	$(CC) $(DEBUG) $(CFLAGS) $(FILS) -o $@ $(LIBS)

ipf.exe: $(IPF)
	$(CC) $(DEBUG) $(CFLAGS) $(IPF) -o $@ $(LIBS)
	/bin/rm -f $(TOP)/ipf
	ln -s `pwd`/ipf.exe $(TOP)/ipf

ipftest: $(IPT)
	$(CC) $(DEBUG) $(CFLAGS) $(IPT) -o $@ $(LIBS)
	/bin/rm -f $(TOP)/ipftest
	ln -s `pwd`/ipftest $(TOP)/ipftest

ipnat: ipnat.o kmem.o natparse.o
	$(CC) $(CFLAGS) ipnat.o kmem.o natparse.o -o $@ $(LIBS)

tests:
	(cd test; make )

fils.o: $(TOP)/fils.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/fils.c -o $@

fil.o: $(TOP)/fil.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/fil.c -o $@

fil_k.o: $(TOP)/fil.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) -I$(TOP) $(POLICY) $(DFLAGS) -c $(TOP)/fil.c -o $@

ipf.o: $(TOP)/ipf.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipf.c -o $@

ipt.o: $(TOP)/ipt.c $(TOP)/ip_fil.h $(TOP)/ipt.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipt.c -o $@

misc.o: $(TOP)/misc.c $(TOP)/ip_fil.h $(TOP)/ipt.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/misc.c -o $@

inet_addr.o: $(TOP)/inet_addr.c
	$(CC) $(ATON) $(DEBUG) $(CFLAGS) -c $(TOP)/inet_addr.c -o $@

opt.o: $(TOP)/opt.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/opt.c -o $@

ipnat.o: $(TOP)/ipnat.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipnat.c -o $@

natparse.o: $(TOP)/natparse.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/natparse.c -o $@

ipft_sn.o: $(TOP)/ipft_sn.c $(TOP)/ipt.h $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/snoop.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_sn.c -o $@

ipft_ef.o: $(TOP)/ipft_ef.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_ef.c -o $@

ipft_td.o: $(TOP)/ipft_td.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_td.c -o $@

ipft_pc.o: $(TOP)/ipft_pc.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_pc.c -o $@

ipft_tx.o: $(TOP)/ipft_tx.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_tx.c -o $@

ipft_hx.o: $(TOP)/ipft_hx.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_hx.c -o $@

ipf:  $(MODOBJS)
	ld -r $(MODOBJS) -o $@

ip_nat.o: $(TOP)/ip_nat.c $(TOP)/ip_nat.h $(TOP)/ip_compat.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_nat.c -o $@

ip_state.o: $(TOP)/ip_state.c $(TOP)/ip_state.h $(TOP)/ip_compat.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_state.c -o $@

ip_proxy.o: $(TOP)/ip_proxy.c $(TOP)/ip_proxy.h $(TOP)/ip_compat.h \
	    $(TOP)/ip_ftp_pxy.c $(TOP)/ip_raudio_pxy.c $(TOP)/ip_nat.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_proxy.c -o $@

ip_frag.o: $(TOP)/ip_frag.c $(TOP)/ip_frag.h $(TOP)/ip_compat.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_frag.c -o $@

ip_auth.o: $(TOP)/ip_auth.c $(TOP)/ip_auth.h $(TOP)/ip_compat.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_auth.c -o $@

ip_nat_u.o: $(TOP)/ip_nat.c $(TOP)/ip_nat.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_nat.c -o $@

ip_frag_u.o: $(TOP)/ip_frag.c $(TOP)/ip_frag.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_frag.c -o $@

ip_state_u.o: $(TOP)/ip_state.c $(TOP)/ip_state.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_state.c -o $@

ip_auth_u.o: $(TOP)/ip_auth.c $(TOP)/ip_auth.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_auth.c -o $@

ip_proxy_u.o: $(TOP)/ip_proxy.c $(TOP)/ip_proxy.h $(TOP)/ip_compat.h \
	      $(TOP)/ip_ftp_pxy.c $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_proxy.c -o $@

ip_fil_u.o: $(TOP)/ip_fil.c $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_fil.c -o $@

ip_sfil.o: $(TOP)/ip_sfil.c $(TOP)/ip_fil.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_sfil.c -o $@

ip_log.o: $(TOP)/ip_log.c $(TOP)/ip_fil.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_log.c -o $@

solaris.o: $(TOP)/solaris.c $(TOP)/ipl.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/solaris.c -o $@

kmem.o: $(TOP)/kmem.c
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/kmem.c -o $@

parse.o: $(TOP)/parse.c $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/parse.c -o $@

facpri.o: $(TOP)/facpri.c $(TOP)/facpri.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/facpri.c -o $@

ipmon: $(TOP)/ipmon.c
	$(CC) $(DEBUG) $(CFLAGS) $(LOGFAC) $(TOP)/ipmon.c -o $@ $(LIBS)

clean:
	if [ `basename \`pwd\`` != SunOS5 ] ; then \
		${RM} -f core *.o ipt fils ipf ipfstat ipftest ipmon ipnat ipf.exe; \
		${RM} -f ipnat.5 ipnat.4 ipnat.1 ipfilter.5; \
		${RM} -f prototype pkginfo postinstall copyright; \
	fi
	/bin/rm -rf */root ipf.pkg
	make -f Makefile.ipsend clean
	-(for i in *; do \
		if [ -d $${i} -a -f $${i}/Makefile ] ; then \
			cd $${i}; (make clean); cd ..; \
			rm $${i}/Makefile $${i}/Makefile.ipsend; \
			rmdir $${i}; \
		fi \
	 done)

ipf.pkg:
	mkdir -p $(PKGBIN) $(ROOT)/$(SBINDEST) $(ROOT)/usr/kernel/drv $(ROOT)/etc/init.d
	mkdir -p $(ROOTINC)/netinet $(PKGDIR)/examples
	mkdir -p $(PKGMAN)/man1 $(PKGMAN)/man4 $(PKGMAN)/man5 $(PKGMAN)/man8
	-$(INSTALL) -c -s $(CPUDIR)/ipftest $(PKGBIN)/ipftest
	-$(INSTALL) -c -s $(CPUDIR)/ipmon $(PKGBIN)/ipmon
	-$(INSTALL) -c -s $(CPUDIR)/ipsend $(PKGBIN)/ipsend
	-$(INSTALL) -c -s $(CPUDIR)/ipresend $(PKGBIN)/ipresend
	-$(INSTALL) -c $(TOP)/mkfilters $(PKGBIN)/mkfilters
	-$(INSTALL) -c $(CPUDIR)/ipf $(ROOT)/usr/kernel/drv/ipf
	-$(INSTALL) -c ipf.conf $(ROOT)/usr/kernel/drv
	-$(INSTALL) -c -s $(CPUDIR)/ipnat $(ROOT)/$(SBINDEST)/ipnat
	-$(INSTALL) -c -s $(CPUDIR)/ipf.exe $(ROOT)/$(SBINDEST)/ipf
	-$(INSTALL) -c -s $(CPUDIR)/ipfstat $(ROOT)/$(SBINDEST)/ipfstat
	-$(INSTALL) -c ipfboot $(ROOT)/etc/init.d
	-cp $(TOP)/man/*.[0-9] .
	-$(INSTALL) ipf.8 $(PKGMAN)/man8
	-$(INSTALL) ipnat.1 $(PKGMAN)/man1
	-$(INSTALL) ipftest.1 $(PKGMAN)/man1
	-$(INSTALL) mkfilters.1 $(PKGMAN)/man1
	-$(INSTALL) ipf.4 $(PKGMAN)/man4
	-$(INSTALL) ipnat.4 $(PKGMAN)/man4
	-$(INSTALL) ipl.4 $(PKGMAN)/man4
	-$(INSTALL) ipf.5 $(PKGMAN)/man5
	-$(INSTALL) ipnat.5 $(PKGMAN)/man5
	-$(INSTALL) ipfilter.5 $(PKGMAN)/man5
	-$(INSTALL) ipfstat.8 $(PKGMAN)/man8
	-$(INSTALL) ipmon.8 $(PKGMAN)/man8
	-cp $(TOP)/rules/* $(PKGDIR)/examples
	-cp $(TOP)/ip_fil.h $(TOP)/ip_compat.h $(TOP)/ip_state.h .
	-cp $(TOP)/ip_nat.h $(TOP)/ip_frag.h $(TOP)/ip_proxy.h .
	-cp $(TOP)/ip_auth.h .
	-$(INSTALL) ip_fil.h $(ROOTINC)/netinet
	-$(INSTALL) ip_compat.h $(ROOTINC)/netinet
	-$(INSTALL) ip_state.h $(ROOTINC)/netinet
	-$(INSTALL) ip_nat.h $(ROOTINC)/netinet
	-$(INSTALL) ip_frag.h $(ROOTINC)/netinet
	-$(INSTALL) ip_proxy.h $(ROOTINC)/netinet
	-$(INSTALL) ip_auth.h $(ROOTINC)/netinet
	-(cd $(CPUDIR); optisa sparcv9 >/dev/null 2>&1; if [ $$? -eq 0 ] ; then ln -s ../prototype64 prototype; else ln -s ../prototype .; fi)
	-(cd $(CPUDIR); ln -s ../pkginfo ../copyright ../postinstall ../postremove .)
	(cd $(CPUDIR); pkgmk -o -d $(TMP))
	touch $(CPUDIR)/ipf.pkg
	pkgtrans -s ${TMP} $(CPUDIR)/ipf.pkg ipf
	/bin/rm -f ipf.pkg
	ln -s $(CPUDIR)/ipf.pkg ipf.pkg
	rm -rf $(TMP)/ipf

package install: ipf.pkg
	@if [ `id|sed -e 's/^.[^(]*(\([^)]*\)).*/\1/'` != root ] ; then \
		echo "Can only install if root"; \
		exit 1; \
	fi
	-pkgadd -d ipf.pkg
