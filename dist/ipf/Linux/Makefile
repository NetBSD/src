#
# Copyright (C) 1993-1998 by Darren Reed.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and due credit is given
# to the original author and the contributors.
#
#gcc -D__KERNEL__ -I/usr/src/linux/include -Wall -Wstrict-prototypes -O2
#-fomit-frame-pointer -fno-strength-reduce -pipe -m486 -malign-loops=2
#-malign-jumps=2 -malign-functions=2 -DCPU=586  -c -o ip_input.o ip_input.c

BINDEST=/usr/sbin
SBINDEST=/sbin
MANDIR=/usr/share/man
#CC=cc -Wall -Wuninitialized -Wstrict-prototypes -Werror -O
CFLAGS=-g -I$(TOP)
#
# For Linux
#
MODDIR=/lib/modules/`uname -r`
CPU=`uname -m`
INC=-I$(TOP) -I/usr/include -I/usr/src/linux -I/usr/src/linux/include
DEF=-D$(CPU) -D__$(CPU)__ -DINET -DKERNEL -D_KERNEL -D__KERNEL__ $(INC)
IPDEF=$(DEF) -DGATEWAY -DDIRECTED_BROADCAST
IPFILC=ip_lfil.c
MFLAGS="BINDEST=$(BINDEST)" "SBINDEST=$(SBINDEST)" "MANDIR=$(MANDIR)" \
	'CFLAGS=$(CFLAGS)' "IPFLKM=$(IPFLKM)" \
	"IPFLOG=$(IPFLOG)" "LOGFAC=$(LOGFAC)" "POLICY=$(POLICY)" \
	"DEBUG=$(DEBUG)" "DCPU=$(CPU)" "CPUDIR=$(CPUDIR)"
#
########## ########## ########## ########## ########## ########## ##########
#
CP=/bin/cp
RM=/bin/rm
CHMOD=/bin/chmod
INSTALL=install
#
MODOBJS=fil_k.o ip_fil.o ip_nat.o ip_frag.o ip_state.o ip_proxy.o \
	ip_auth.o ip_log.o
DFLAGS=-O2 $(IPFLKM) $(IPFLOG) $(DEF) $(DLKM)
IPF=ipf.o parse.o opt.o facpri.o
IPT=ipt.o parse.o fil.o ipft_sn.o ipft_ef.o ipft_td.o ipft_pc.o opt.o \
    ipft_tx.o misc.o ip_frag_u.o ip_state_u.o ip_nat_u.o ip_proxy_u.o \
    ip_auth_u.o ipft_hx.o ip_fil_u.o facpri.o
FILS=fils.o parse.o kmem.o opt.o facpri.o

build all: ipf ipfstat ipftest ipmon ipnat if_ipl.o

ipfstat: $(FILS)
	$(CC) $(DEBUG) $(CFLAGS) $(FILS) -o $@ $(LIBS)

ipf: $(IPF)
	$(CC) $(DEBUG) $(CFLAGS) $(IPF) -o $@ $(LIBS)
	/bin/rm -f $(TOP)/ipf
	ln -s `pwd`/ipf $(TOP)

ipftest: $(IPT)
	$(CC) $(DEBUG) $(CFLAGS) $(IPT) -o $@ $(LIBS)
	/bin/rm -f $(TOP)/ipftest
	ln -s `pwd`/ipftest $(TOP)

ipnat: ipnat.o kmem.o
	$(CC) $(DEBUG) $(CFLAGS) ipnat.o kmem.o -o $@ $(LIBS)

tests:
	(cd test; make )

fils.o: $(TOP)/fils.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_frag.h \
	$(TOP)/ip_compat.h $(TOP)/ip_state.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/fils.c -o $@

fil.o: $(TOP)/fil.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/fil.c -o $@

fil_k.o: $(TOP)/fil.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) $(POLICY) $(DFLAGS) -c $(TOP)/fil.c -o $@

ipf.o: $(TOP)/ipf.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipf.c -o $@

ipt.o: $(TOP)/ipt.c $(TOP)/ip_fil.h $(TOP)/ipt.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipt.c -o $@

misc.o: $(TOP)/misc.c $(TOP)/ip_fil.h $(TOP)/ipt.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/misc.c -o $@

opt.o: $(TOP)/opt.c $(TOP)/ip_fil.h $(TOP)/ipf.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/opt.c -o $@

ipnat.o: $(TOP)/ipnat.c $(TOP)/ip_fil.h $(TOP)/ipf.h $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ipnat.c -o $@

ipft_sn.o: $(TOP)/ipft_sn.c $(TOP)/ipt.h $(TOP)/ipf.h $(TOP)/ip_fil.h \
	$(TOP)/snoop.h
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

ip_nat_u.o: $(TOP)/ip_nat.c $(TOP)/ip_nat.h $(TOP)/ip_compat.h $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_nat.c -o $@

ip_proxy_u.o: $(TOP)/ip_proxy.c $(TOP)/ip_proxy.h $(TOP)/ip_compat.h \
	$(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_proxy.c -o $@

ip_frag_u.o: $(TOP)/ip_frag.c $(TOP)/ip_frag.h $(TOP)/ip_compat.h \
	$(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_frag.c -o $@

ip_state_u.o: $(TOP)/ip_state.c $(TOP)/ip_state.h $(TOP)/ip_compat.h \
	$(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_state.c -o $@

ip_auth_u.o: $(TOP)/ip_auth.c $(TOP)/ip_auth.h $(TOP)/ip_compat.h \
	$(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/ip_auth.c -o $@

ip_fil_u.o: $(TOP)/$(IPFILC) $(TOP)/ip_fil.h $(TOP)/ip_compat.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/$(IPFILC) -o $@

if_ipl.o: $(MODOBJS) 
	ld -r $(MODOBJS) -o if_ipl.o
	${RM} -f if_ipl

ip_nat.o: $(TOP)/ip_nat.c $(TOP)/ip_nat.h $(TOP)/ip_compat.h $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_nat.c -o $@

ip_frag.o: $(TOP)/ip_frag.c $(TOP)/ip_frag.h $(TOP)/ip_compat.h $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_frag.c -o $@

ip_state.o: $(TOP)/ip_state.c $(TOP)/ip_state.h $(TOP)/ip_compat.h \
	$(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_state.c -o $@

ip_proxy.o: $(TOP)/ip_proxy.c $(TOP)/ip_proxy.h $(TOP)/ip_compat.h \
	$(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_proxy.c -o $@

ip_auth.o: $(TOP)/ip_auth.c $(TOP)/ip_auth.h $(TOP)/ip_compat.h \
	$(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_auth.c -o $@

ip_fil.o: $(TOP)/$(IPFILC) $(TOP)/ip_fil.h $(TOP)/ip_compat.h $(TOP)/ip_nat.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -DMODVERSIONS -D__GENKSYMS__ -DMODULE -DEXPORT_SYMTAB -c $(TOP)/$(IPFILC) -o $@

ip_log.o: $(TOP)/ip_log.c $(TOP)/ip_compat.h $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) $(DFLAGS) -c $(TOP)/ip_log.c -o $@

kmem.o: $(TOP)/kmem.c
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/kmem.c -o $@

parse.o: $(TOP)/parse.c $(TOP)/ip_fil.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/parse.c -o $@

facpri.o: $(TOP)/facpri.c $(TOP)/facpri.h
	$(CC) $(DEBUG) $(CFLAGS) -c $(TOP)/facpri.c -o $@

ipmon: $(TOP)/ipmon.c
	$(CC) $(DEBUG) $(CFLAGS) $(LOGFAC) $(TOP)/ipmon.c -o $@ $(LIBS)

clean:
	${RM} -f *.core *.o ipt fils ipf ipfstat ipftest ipmon if_ipl ipnat \
	if_ipl.o
	${MAKE} -f Makefile.ipsend ${MFLAGS} clean
	-(for i in *; do \
		if [ -d $${i} -a -f $${i}/Makefile ] ; then \
			cd $${i}; (make clean); cd ..; \
			rm $${i}/Makefile $${i}/Makefile.ipsend; \
			rmdir $${i}; \
		fi \
	done)

install:
	-$(CP) $(TOP)/ip_fil.h /usr/include/netinet/ip_fil.h
	-$(CHMOD) 444 /usr/include/netinet/ip_fil.h
	-if [ -d $(MODDIR) ] ; then \
		cp if_ipl.o $(MODDIR); \
	fi
	-$(INSTALL) -cs -g wheel -m 755 -o root ipfstat ipf ipnat $(SBINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipmon ipftest $(BINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipftest ipftest $(BINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipf ipftest $(SBINDEST)
	-$(INSTALL) -cs -g wheel -m 755 -o root ipnat ipftest $(SBINDEST)
	(cd $(TOP)/man; make INSTALL=$(INSTALL) MANDIR=$(MANDIR) install; cd $(TOP))
