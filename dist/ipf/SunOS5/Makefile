#
# Copyright (C) 1993-1998 by Darren Reed.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and due credit is given
# to the original author and the contributors.
#
BINDEST=/usr/local/bin
SBINDEST=/sbin/$(OBJ)
MANDIR=/usr/share/man
CC=cc
CFLAGS=-I$(TOP)
#
# For SunOS 5.x
#
PKGDIR=$(ROOT)/opt/ipf
PKGMAN=$(PKGDIR)/man
PKGBIN=$(PKGDIR)/bin/$(OBJ)
#
TOP=..
#CPU:sh=uname -p
#REV:sh=uname -r
CPUDIR=`uname -p`-`uname -r`
BITS=`optisa sparcv9 >/dev/null 2>&1; if [ $$? -eq 0 ] ; then echo "64"; else echo "32"; fi`
CC=gcc -Wall
DEBUG=-g
LIBS=-lsocket -lnsl -lelf
DEF=-D_KERNEL -DSUNDDI
OBJ=.
PKG=ipf
PROTO=prototype
BITTYPE=`if [ "$(BITS)" = 64 ];then echo ' (64-bit)'; fi`
OBJDIR=$(CPUDIR)/$(OBJ)
TMP=/tmp/ipfpkgs
ATON=-DNEED_INET_ATON
ROOT=$(CPUDIR)/root
ROOTINC=$(ROOT)/usr/include
#
MFLAGS="BINDEST=$(BINDEST)" "SBINDEST=$(SBINDEST)" "MANDIR=$(MANDIR)" \
        'CFLAGS=$(CFLAGS) $(ARCHINC) $(SOLARIS2)' "IPFLKM=$(IPFLKM)" \
        "IPFLOG=$(IPFLOG)" "LOGFAC=$(LOGFAC)" "POLICY=$(POLICY)" \
        "SOLARIS2=$(SOLARIS2)" "DEBUG=$(DEBUG)" "CPUDIR=$(CPUDIR)" \
	"BITS=$(BITS)" "OBJ=$(OBJ)"
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
MODOBJS=$(OBJ)/ip_sfil.o $(OBJ)/fil_k.o $(OBJ)/solaris.o $(OBJ)/ip_state.o \
    $(OBJ)/ip_frag.o $(OBJ)/ip_nat.o $(OBJ)/ip_proxy.o $(OBJ)/ip_auth.o \
    $(OBJ)/ip_log.o
IPF=$(OBJ)/ipf.o $(OBJ)/parse.o $(OBJ)/common.o $(OBJ)/opt.o \
    $(OBJ)/inet_addr.o $(OBJ)/facpri.o
IPT=$(OBJ)/ipt.o $(OBJ)/parse.o $(OBJ)/common.o $(OBJ)/fil.o $(OBJ)/ipft_sn.o \
    $(OBJ)/ipft_ef.o $(OBJ)/ipft_td.o $(OBJ)/ipft_pc.o  $(OBJ)/opt.o \
    $(OBJ)/ipft_tx.o $(OBJ)/misc.o $(OBJ)/ip_state_u.o $(OBJ)/ip_frag_u.o \
    $(OBJ)/ip_nat_u.o $(OBJ)/inet_addr.o $(OBJ)/ip_fil_u.o \
    $(OBJ)/ipft_hx.o $(OBJ)/ip_proxy_u.o $(OBJ)/ip_auth_u.o \
    $(OBJ)/natparse.o $(OBJ)/facpri.o
IPNAT=$(OBJ)/ipnat.o $(OBJ)/kmem.o $(OBJ)/natparse.o $(OBJ)/inet_addr.o \
    $(OBJ)/common.o
FILS=$(OBJ)/fils.o $(OBJ)/parse.o $(OBJ)/kmem.o $(OBJ)/opt.o \
    $(OBJ)/inet_addr.o $(OBJ)/facpri.o $(OBJ)/common.o

$(OBJ):
	@if [ ! -d $(OBJ) ]; then mkdir $(OBJ); fi


sunos5 solaris2 build: $(OBJ) $(OBJ)/ipf.exe $(OBJ)/ipfstat $(OBJ)/ipftest \
    $(OBJ)/ipmon $(OBJ)/ipnat $(OBJ)/ipf $(OBJ)/ipfs

pkg: ipf.pkg

$(OBJ)/ipfstat: $(FILS)
	$(CC) $(DEBUG) $(CFLAGS) $(FILS) -o $@ $(LIBS) $(STATETOP_LIB)

$(OBJ)/ipf.exe: $(IPF)
	$(CC) $(DEBUG) $(CFLAGS) $(IPF) -o $@ $(LIBS)
	/bin/rm -f $(TOP)/ipf $(TOP)/$(OBJ)/ipf
	if [ -f /usr/lib/isaexec -a "$(OBJ)" != . ] ; then \
		mkdir -p $(TOP)/$(OBJ); \
		cp /usr/lib/isaexec $(TOP)/ipf; \
	fi
	ln -s `pwd`/$(OBJ)/ipf.exe $(TOP)/$(OBJ)/ipf

# Magic dependency on ipf.exe creates the $(TOP)/sparcv{7,9} directories.
$(OBJ)/ipftest: $(IPT) $(OBJ)/ipf.exe
	$(CC) $(DEBUG) $(CFLAGS) $(IPT) -o $@ $(LIBS)
	/bin/rm -f $(TOP)/ipftest $(TOP)/$(OBJ)/ipftest
	if [ -f /usr/lib/isaexec -a "$(OBJ)" != . ] ; then \
		ln $(TOP)/ipf $(TOP)/ipftest; \
	fi
	ln -s `pwd`/$(OBJ)/ipftest $(TOP)/$(OBJ)/ipftest

$(OBJ)/ipnat: $(IPNAT)
	$(CC) $(CFLAGS) $(IPNAT) -o $@ $(LIBS)

$(OBJ)/ipfs: $(OBJ)/ipfs.o
	$(CC) $(DEBUG) $(CFLAGS) $(OBJ)/ipfs.o -o $@ $(LIBS)

tests:
	(cd test; make )

$(OBJ)/fils.o: $(TOP)/fils.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) $(STATETOP_CFLAGS) $(STATETOP_INC) \
		-c $(TOP)/fils.c -o $@

$(OBJ)/ipfs.o: $(TOP)/ipfs.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_state.h \
		$(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipfs.c -o $@

$(OBJ)/fil.o: $(TOP)/fil.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/fil.c -o $@

$(OBJ)/fil_k.o: $(TOP)/fil.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) -I$(TOP) $(POLICY) $(DFLAGS) -c $(TOP)/fil.c -o $@

$(OBJ)/ipf.o: $(TOP)/ipf.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipf.c -o $@

$(OBJ)/ipt.o: $(TOP)/ipt.c $(TOP)/ip_fil.h $(TOP)/ipt.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipt.c -o $@

$(OBJ)/misc.o: $(TOP)/misc.c $(TOP)/ip_fil.h $(TOP)/ipt.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/misc.c -o $@

$(OBJ)/inet_addr.o: $(TOP)/inet_addr.c
	$(CC) $(ATON) $(DEBUG) $(CFLAGS) -c $(TOP)/inet_addr.c -o $@

$(OBJ)/opt.o: $(TOP)/opt.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/opt.c -o $@

$(OBJ)/ipnat.o: $(TOP)/ipnat.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipnat.c -o $@

$(OBJ)/natparse.o: $(TOP)/natparse.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_nat.h \
		$(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/natparse.c -o $@

$(OBJ)/ipft_sn.o: $(TOP)/ipft_sn.c $(TOP)/ipt.h $(TOP)/ipf.h $(TOP)/ip_fil.h \
		$(TOP)/snoop.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_sn.c -o $@

$(OBJ)/ipft_ef.o: $(TOP)/ipft_ef.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_ef.c -o $@

$(OBJ)/ipft_td.o: $(TOP)/ipft_td.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_td.c -o $@

$(OBJ)/ipft_pc.o: $(TOP)/ipft_pc.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_pc.c -o $@

$(OBJ)/ipft_tx.o: $(TOP)/ipft_tx.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_tx.c -o $@

$(OBJ)/ipft_hx.o: $(TOP)/ipft_hx.c $(TOP)/ipf.h $(TOP)/ip_fil.h $(TOP)/ipt.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipft_hx.c -o $@

$(OBJ)/ipf:  $(MODOBJS)
	ld -r $(MODOBJS) -o $@

$(OBJ)/ip_nat.o: $(TOP)/ip_nat.c $(TOP)/ip_nat.h $(TOP)/ip_compat.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_nat.c -o $@

$(OBJ)/ip_state.o: $(TOP)/ip_state.c $(TOP)/ip_state.h $(TOP)/ip_compat.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_state.c -o $@

$(OBJ)/ip_proxy.o: $(TOP)/ip_proxy.c $(TOP)/ip_proxy.h $(TOP)/ip_compat.h \
	    $(TOP)/ip_ftp_pxy.c $(TOP)/ip_raudio_pxy.c $(TOP)/ip_nat.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_proxy.c -o $@

$(OBJ)/ip_frag.o: $(TOP)/ip_frag.c $(TOP)/ip_frag.h $(TOP)/ip_compat.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_frag.c -o $@

$(OBJ)/ip_auth.o: $(TOP)/ip_auth.c $(TOP)/ip_auth.h $(TOP)/ip_compat.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_auth.c -o $@

$(OBJ)/ip_nat_u.o: $(TOP)/ip_nat.c $(TOP)/ip_nat.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_nat.c -o $@

$(OBJ)/ip_frag_u.o: $(TOP)/ip_frag.c $(TOP)/ip_frag.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_frag.c -o $@

$(OBJ)/ip_state_u.o: $(TOP)/ip_state.c $(TOP)/ip_state.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_state.c -o $@

$(OBJ)/ip_auth_u.o: $(TOP)/ip_auth.c $(TOP)/ip_auth.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_auth.c -o $@

$(OBJ)/ip_proxy_u.o: $(TOP)/ip_proxy.c $(TOP)/ip_proxy.h $(TOP)/ip_compat.h \
	      $(TOP)/ip_ftp_pxy.c $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_proxy.c -o $@

$(OBJ)/ip_fil_u.o: $(TOP)/ip_fil.c $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_fil.c -o $@

$(OBJ)/ip_sfil.o: $(TOP)/ip_sfil.c $(TOP)/ip_fil.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_sfil.c -o $@

$(OBJ)/ip_log.o: $(TOP)/ip_log.c $(TOP)/ip_fil.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/ip_log.c -o $@

$(OBJ)/solaris.o: $(TOP)/solaris.c $(TOP)/ipl.h
	$(CC) -I$(TOP) $(DFLAGS) -c $(TOP)/solaris.c -o $@

$(OBJ)/kmem.o: $(TOP)/kmem.c
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/kmem.c -o $@

$(OBJ)/parse.o: $(TOP)/parse.c $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/parse.c -o $@

$(OBJ)/common.o: $(TOP)/common.c $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/common.c -o $@

$(OBJ)/facpri.o: $(TOP)/facpri.c $(TOP)/facpri.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/facpri.c -o $@

$(OBJ)/ipmon: $(TOP)/ipmon.c
	$(CC) $(DEBUG) $(CFLAGS) $(LOGFAC) $(TOP)/ipmon.c -o $@ $(LIBS)

clean:
	if [ `basename \`pwd\`` != SunOS5 ] ; then \
		${RM} -f core *.o ipt fils ipf ipfstat ipftest ipmon ipnat ipf.exe ipfs; \
		${RM} -f ipnat.5 ipnat.4 ipnat.1 ipfilter.5; \
		${RM} -f prototype pkginfo postinstall postremove copyright; \
		${RM} -f  y.tab.? lex.yy.c; \
	fi
	/bin/rm -rf */root ipf.pkg* sparcv7 sparcv9
	make -f Makefile.ipsend clean
	-(for i in *; do \
		if [ -d $${i} -a -h $${i}/Makefile ] ; then \
			cd $${i}; (make clean); cd ..; \
			rm $${i}/Makefile $${i}/Makefile.ipsend; \
			rmdir $${i}; \
		fi \
	 done)

ipf.pkg: ipf.pkg.common
	if [ -d $(CPUDIR)/sparcv7 ]; then \
	    $(MAKE) PROTO=prototype32 PKG=ipf OBJ=sparcv7 BITS=32 \
		ipf.pkg.bin; \
	    $(MAKE) PROTO=prototype64 PKG=ipfx OBJ=sparcv9 BITS=64 \
		ipf.pkg.bin; \
	else \
	    $(MAKE) OBJ=. BITS= ipf.pkg.bin;\
	fi
	touch $(CPUDIR)/ipf.pkg
	pkgtrans -s $(TMP) $(CPUDIR)/ipf.pkg `ls $(TMP)`
	/bin/rm -f ipf.pkg
	ln -s $(CPUDIR)/ipf.pkg ipf.pkg
	rm -rf $(TMP)/ipf*
	    

ipf.pkg.common:
	mkdir -p $(ROOTINC)/netinet $(PKGDIR)/examples $(ROOT)/usr/kernel/drv
	mkdir -p $(ROOT)/etc/init.d $(TMP)
	mkdir -p $(PKGMAN)/man1 $(PKGMAN)/man4 $(PKGMAN)/man5 $(PKGMAN)/man8
	-$(INSTALL) -c ipfboot $(ROOT)/etc/init.d
	-cp $(TOP)/man/*.[0-9] .
	-$(INSTALL) -c ipf.conf $(ROOT)/usr/kernel/drv
	-$(INSTALL) ipf.8 $(PKGMAN)/man8
	-$(INSTALL) ipfs.8 $(PKGMAN)/man8
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
	touch $@

#
# For 64 bit Solaris, we build 32 and 64 bit packages, but the 64 bit package
# is smaller and only contains the 64 bit specific and not the common stuff.
#
ipf.pkg.bin:
	mkdir -p $(PKGBIN) $(ROOT)/$(SBINDEST) $(ROOT)/usr/kernel/drv/$(OBJ)
	-$(INSTALL) -c -s $(OBJDIR)/ipftest $(PKGBIN)/ipftest
	-$(INSTALL) -c -s $(OBJDIR)/ipmon $(PKGBIN)/ipmon
	-$(INSTALL) -c -s $(OBJDIR)/ipsend $(PKGBIN)/ipsend
	-$(INSTALL) -c -s $(OBJDIR)/ipresend $(PKGBIN)/ipresend
	-$(INSTALL) -c $(TOP)/mkfilters $(PKGBIN)/mkfilters
	-$(INSTALL) -c $(OBJDIR)/ipf $(ROOT)/usr/kernel/drv/$(OBJ)/ipf
	-$(INSTALL) -c -s $(OBJDIR)/ipnat $(ROOT)/$(SBINDEST)/ipnat
	-$(INSTALL) -c -s $(OBJDIR)/ipf.exe $(ROOT)/$(SBINDEST)/ipf
	-$(INSTALL) -c -s $(OBJDIR)/ipfs $(ROOT)/$(SBINDEST)/ipfs
	-$(INSTALL) -c -s $(OBJDIR)/ipfstat $(ROOT)/$(SBINDEST)/ipfstat
	#-(cd $(CPUDIR); rm -f copyright post* prototype pkginfo)
	-(cd $(CPUDIR); rm -f prototype)
	#-(cd $(CPUDIR); ln -s ../copyright ../postinstall ../postremove .)
	-(cd $(CPUDIR); ln -s ../$(PROTO) prototype)
	-sed -e 's/ipf/$(PKG)/' -e "s/NAME=.*/&$(BITTYPE)/" \
		< pkginfo > $(CPUDIR)/pkginfo
	-if [ "$(BITS)" = 64 ]; then echo 'SUNW_ISA=sparcv9' >> $(CPUDIR)/pkginfo; fi
	(cd $(CPUDIR); pkgmk -a `uname -p` -o -d $(TMP))


package install: ipf.pkg
	@if [ `id|sed -e 's/^.[^(]*(\([^)]*\)).*/\1/'` != root ] ; then \
		echo "Can only install if root"; \
		exit 1; \
	fi
	# Must add ipfx before ipf or driver attach fails.
	-[ -d $(CPUDIR)/sparcv9 ] && pkgadd -d ipf.pkg ipfx
	-pkgadd -d ipf.pkg ipf
