KERN_IDENT=GODZILLA
IDENT=-DGODZILLA -DA3000 -DGENERIC -DNKMEMCLUSTERS=256 -DKTRACE -DFPCOPROC -DDIAGNOSTIC -DDEBUG -DHAVE_USL_UFS -DCOMPAT_NOMID -DTCP_COMPAT_42 -DCOMPAT_43 -DTPIP -DISO -DFDESC -DKERNFS -DPCFS -DFIFO -DISOFS -DMFS -DNFSCLIENT -DNFSSERVER -DNFS -DINET
PARAM=-DTIMEZONE=-60 -DDST=1 -DMAXUSERS=16 -DMAXFDESCS=2048
#	@(#)Makefile.hp300	7.10 (Berkeley) 6/27/91
#
# Makefile for 4.4 BSD
#
# This makefile is constructed from a machine description:
#	config machineid
# Most changes should be made in the machine description
#	/sys/conf/``machineid''
# after which you should do
#	 config machineid
# Machine generic makefile changes should be made in
#	/sys/conf/Makefile.``machinetype''
# after which config should be rerun for all machines of that type.
#
# N.B.: NO DEPENDENCIES ON FOLLOWING FLAGS ARE VISIBLE TO MAKEFILE
#	IF YOU CHANGE THE DEFINITION OF ANY OF THESE RECOMPILE EVERYTHING
#
# -DTRACE	compile in kernel tracing hooks
# -DQUOTA	compile in file system quotas


# DEBUG is set to -g by config if debugging is requested (config -g).
# PROF is set to -pg by config if profiling is requested (config -p).
AS=	gcc:compilers/amiga/2.3.3/as ${DEBUG}
CC=	gcc ${DEBUG}
CPP=	gcc:compilers/amiga/2.3.3/cpp -traditional
LD=	ld.bsd
TOUCH=	touch -f -c
GPROF.EX=/usr/src/usr.lib/libc/csu/amiga/gmon.ex

# source tree is located via $S relative to the compilation directory
S=	../../../..
AMIGA=	../..

INCLUDES= -I. -I$S/arch -I$S -I$S/sys
COPTS=	${INCLUDES} ${IDENT} -DKERNEL -Dmc68020 -Damiga -DREFBIT
CFLAGS=	-O -mc68020 -m68881 ${COPTS}

# compile rules: rules are named ${TYPE}_${SUFFIX}${CONFIG_DEP}
# where TYPE is NORMAL, DRIVER, or PROFILE}; SUFFIX is the file suffix,
# capitalized (e.g. C for a .c file), and CONFIG_DEP is _C if the file
# is marked as config-dependent.

NORMAL_C=	${CC} -c ${CFLAGS} ${PROF} $<
NORMAL_C_C=	${CC} -c ${CFLAGS} ${PROF} ${PARAM} $<

DRIVER_C=	${CC} -c ${CFLAGS} ${PROF} $<
DRIVER_C_C=	${CC} -c ${CFLAGS} ${PROF} ${PARAM} $<

PROFILE_C=	${CC} -S -c ${COPTS} $<; \
		ex - $*.s < ${GPROF.EX}; \
		${AS} -o $@ $*.s; \
		rm -f $*.s

NORMAL_S=	${CPP} ${COPTS} $< | ${AS} -o $@
NORMAL_S_C=	${CPP} ${COPTS} ${PARAM} $< | ${AS} -o $@

OBJS=isofs_bmap.o isofs_lookup.o isofs_node.o isofs_util.o \
	isofs_vfsops.o isofs_vnops.o dead_vnops.o fifo_vnops.o \
	init_main.o init_sysent.o kern_acct.o kern_clock.o \
	kern_descrip.o kern_exec.o kern_exit.o kern_fork.o kern_kinfo.o \
	kern_ktrace.o kern_malloc.o kern_physio.o kern_proc.o \
	kern_prot.o kern_resource.o kern_sig.o kern_subr.o kern_synch.o \
	kern_time.o kern_xxx.o spec_vnops.o subr_log.o subr_mcount.o \
	subr_prf.o subr_rlist.o subr_xxx.o sys_generic.o sys_process.o \
	sys_socket.o tty.o tty_compat.o tty_conf.o tty_pty.o tty_ring.o \
	tty_tty.o uipc_domain.o uipc_mbuf.o uipc_proto.o uipc_socket.o \
	uipc_socket2.o uipc_syscalls.o uipc_usrreq.o vfs_bio.o \
	vfs_cache.o vfs_conf.o vfs_lookup.o vfs_subr.o vfs_syscalls.o \
	vfs_vnops.o fdesc_vfsops.o fdesc_vnops.o kernfs_vfsops.o \
	kernfs_vnops.o af.o bpf.o bpf_filter.o if.o if_ethersubr.o \
	if_loop.o if_sl.o radix.o raw_cb.o raw_usrreq.o route.o rtsock.o \
	slcompress.o if_ether.o in.o in_pcb.o in_proto.o ip_icmp.o \
	ip_input.o ip_output.o raw_ip.o tcp_debug.o tcp_input.o \
	tcp_output.o tcp_subr.o tcp_timer.o tcp_usrreq.o udp_usrreq.o \
	clnp_debug.o clnp_er.o clnp_frag.o clnp_input.o clnp_options.o \
	clnp_output.o clnp_raw.o clnp_subr.o clnp_timer.o cltp_usrreq.o \
	esis.o iso.o iso_chksum.o iso_pcb.o iso_proto.o iso_snpac.o \
	tp_astring.o tp_cons.o tp_driver.o tp_emit.o tp_inet.o \
	tp_input.o tp_iso.o tp_meas.o tp_output.o tp_pcb.o tp_subr.o \
	tp_subr2.o tp_timer.o tp_trace.o tp_usrreq.o nfs_bio.o \
	nfs_node.o nfs_serv.o nfs_socket.o nfs_srvcache.o nfs_subs.o \
	nfs_syscalls.o nfs_vfsops.o nfs_vnops.o pcfs_conv.o \
	pcfs_denode.o pcfs_fat.o pcfs_lookup.o pcfs_vfsops.o \
	pcfs_vnops.o mfs_vfsops.o mfs_vnops.o ufs_alloc.o ufs_bmap.o \
	ufs_disksubr.o ufs_inode.o ufs_lockf.o ufs_lookup.o ufs_subr.o \
	ufs_tables.o ufs_vfsops.o ufs_vnops.o device_pager.o kern_lock.o \
	swap_pager.o vm_fault.o vm_glue.o vm_init.o vm_kern.o vm_map.o \
	vm_meter.o vm_mmap.o vm_object.o vm_page.o vm_pageout.o \
	vm_pager.o vm_swap.o vm_unix.o vm_user.o vnode_pager.o \
	amiga_init.o autoconf.o clock.o conf.o cons.o disksubr.o dkbad.o \
	in_cksum.o machdep.o mem.o oc_cksum.o pmap.o sys_machdep.o \
	trap.o vm_machdep.o dma.o vn.o scsi.o sd.o st.o ser.o kbd.o \
	ite.o ite_cc.o ite_tg.o kernel_font.o kbdmap.o grf.o grf_cc.o \
	grf_tg.o 

CFILES=$S/isofs/isofs_bmap.c $S/isofs/isofs_lookup.c \
	$S/isofs/isofs_node.c $S/isofs/isofs_util.c \
	$S/isofs/isofs_vfsops.c $S/isofs/isofs_vnops.c \
	$S/kern/dead_vnops.c $S/kern/fifo_vnops.c $S/kern/init_main.c \
	$S/kern/init_sysent.c $S/kern/kern_acct.c $S/kern/kern_clock.c \
	$S/kern/kern_descrip.c $S/kern/kern_exec.c $S/kern/kern_exit.c \
	$S/kern/kern_fork.c $S/kern/kern_kinfo.c $S/kern/kern_ktrace.c \
	$S/kern/kern_malloc.c $S/kern/kern_physio.c $S/kern/kern_proc.c \
	$S/kern/kern_prot.c $S/kern/kern_resource.c $S/kern/kern_sig.c \
	$S/kern/kern_subr.c $S/kern/kern_synch.c $S/kern/kern_time.c \
	$S/kern/kern_xxx.c $S/kern/spec_vnops.c $S/kern/subr_log.c \
	$S/kern/subr_mcount.c $S/kern/subr_prf.c $S/kern/subr_rlist.c \
	$S/kern/subr_xxx.c $S/kern/sys_generic.c $S/kern/sys_process.c \
	$S/kern/sys_socket.c $S/kern/tty.c $S/kern/tty_compat.c \
	$S/kern/tty_conf.c $S/kern/tty_pty.c $S/kern/tty_ring.c \
	$S/kern/tty_tty.c $S/kern/uipc_domain.c $S/kern/uipc_mbuf.c \
	$S/kern/uipc_proto.c $S/kern/uipc_socket.c \
	$S/kern/uipc_socket2.c $S/kern/uipc_syscalls.c \
	$S/kern/uipc_usrreq.c $S/kern/vfs_bio.c $S/kern/vfs_cache.c \
	$S/kern/vfs_conf.c $S/kern/vfs_lookup.c $S/kern/vfs_subr.c \
	$S/kern/vfs_syscalls.c $S/kern/vfs_vnops.c \
	$S/miscfs/fdesc/fdesc_vfsops.c $S/miscfs/fdesc/fdesc_vnops.c \
	$S/miscfs/kernfs/kernfs_vfsops.c $S/miscfs/kernfs/kernfs_vnops.c \
	$S/net/af.c $S/net/bpf.c $S/net/bpf_filter.c $S/net/if.c \
	$S/net/if_ethersubr.c $S/net/if_loop.c $S/net/if_sl.c \
	$S/net/radix.c $S/net/raw_cb.c $S/net/raw_usrreq.c \
	$S/net/route.c $S/net/rtsock.c $S/net/slcompress.c \
	$S/netinet/if_ether.c $S/netinet/in.c $S/netinet/in_pcb.c \
	$S/netinet/in_proto.c $S/netinet/ip_icmp.c $S/netinet/ip_input.c \
	$S/netinet/ip_output.c $S/netinet/raw_ip.c \
	$S/netinet/tcp_debug.c $S/netinet/tcp_input.c \
	$S/netinet/tcp_output.c $S/netinet/tcp_subr.c \
	$S/netinet/tcp_timer.c $S/netinet/tcp_usrreq.c \
	$S/netinet/udp_usrreq.c $S/netiso/clnp_debug.c \
	$S/netiso/clnp_er.c $S/netiso/clnp_frag.c $S/netiso/clnp_input.c \
	$S/netiso/clnp_options.c $S/netiso/clnp_output.c \
	$S/netiso/clnp_raw.c $S/netiso/clnp_subr.c \
	$S/netiso/clnp_timer.c $S/netiso/cltp_usrreq.c $S/netiso/esis.c \
	$S/netiso/iso.c $S/netiso/iso_chksum.c $S/netiso/iso_pcb.c \
	$S/netiso/iso_proto.c $S/netiso/iso_snpac.c \
	$S/netiso/tp_astring.c $S/netiso/tp_cons.c $S/netiso/tp_driver.c \
	$S/netiso/tp_emit.c $S/netiso/tp_inet.c $S/netiso/tp_input.c \
	$S/netiso/tp_iso.c $S/netiso/tp_meas.c $S/netiso/tp_output.c \
	$S/netiso/tp_pcb.c $S/netiso/tp_subr.c $S/netiso/tp_subr2.c \
	$S/netiso/tp_timer.c $S/netiso/tp_trace.c $S/netiso/tp_usrreq.c \
	$S/nfs/nfs_bio.c $S/nfs/nfs_node.c $S/nfs/nfs_serv.c \
	$S/nfs/nfs_socket.c $S/nfs/nfs_srvcache.c $S/nfs/nfs_subs.c \
	$S/nfs/nfs_syscalls.c $S/nfs/nfs_vfsops.c $S/nfs/nfs_vnops.c \
	$S/pcfs/pcfs_conv.c $S/pcfs/pcfs_denode.c $S/pcfs/pcfs_fat.c \
	$S/pcfs/pcfs_lookup.c $S/pcfs/pcfs_vfsops.c $S/pcfs/pcfs_vnops.c \
	$S/ufs/mfs_vfsops.c $S/ufs/mfs_vnops.c $S/ufs/ufs_alloc.c \
	$S/ufs/ufs_bmap.c $S/ufs/ufs_disksubr.c $S/ufs/ufs_inode.c \
	$S/ufs/ufs_lockf.c $S/ufs/ufs_lookup.c $S/ufs/ufs_subr.c \
	$S/ufs/ufs_tables.c $S/ufs/ufs_vfsops.c $S/ufs/ufs_vnops.c \
	$S/vm/device_pager.c $S/vm/kern_lock.c $S/vm/swap_pager.c \
	$S/vm/vm_fault.c $S/vm/vm_glue.c $S/vm/vm_init.c $S/vm/vm_kern.c \
	$S/vm/vm_map.c $S/vm/vm_meter.c $S/vm/vm_mmap.c \
	$S/vm/vm_object.c $S/vm/vm_page.c $S/vm/vm_pageout.c \
	$S/vm/vm_pager.c $S/vm/vm_swap.c $S/vm/vm_unix.c $S/vm/vm_user.c \
	$S/vm/vnode_pager.c $S/arch/amiga/amiga/amiga_init.c \
	$S/arch/amiga/amiga/autoconf.c $S/arch/amiga/amiga/clock.c \
	$S/arch/amiga/amiga/conf.c $S/arch/amiga/amiga/cons.c \
	$S/arch/amiga/amiga/disksubr.c $S/arch/amiga/amiga/dkbad.c \
	$S/arch/amiga/amiga/in_cksum.c $S/arch/amiga/amiga/machdep.c \
	$S/arch/amiga/amiga/mem.c $S/arch/amiga/amiga/pmap.c \
	$S/arch/amiga/amiga/sys_machdep.c $S/arch/amiga/amiga/trap.c \
	$S/arch/amiga/amiga/vm_machdep.c $S/arch/amiga/dev/dma.c \
	$S/arch/amiga/dev/vn.c $S/arch/amiga/dev/scsi.c \
	$S/arch/amiga/dev/sd.c $S/arch/amiga/dev/st.c \
	$S/arch/amiga/dev/ser.c $S/arch/amiga/dev/kbd.c \
	$S/arch/amiga/dev/ite.c $S/arch/amiga/dev/ite_cc.c \
	$S/arch/amiga/dev/ite_tg.c $S/arch/amiga/dev/kernel_font.c \
	$S/arch/amiga/dev/kbdmap.c $S/arch/amiga/dev/grf.c \
	$S/arch/amiga/dev/grf_cc.c $S/arch/amiga/dev/grf_tg.c \
	$S/arch/amiga/amiga/swapgeneric.c 

# load lines for config "xxx" will be emitted as:
# xxx: ${SYSTEM_DEP} swapxxx.o
#	${SYSTEM_LD_HEAD}
#	${SYSTEM_LD} swapxxx.o
#	${SYSTEM_LD_TAIL}
SYSTEM_OBJ=	locore.o ${OBJS} param.o ioconf.o
#SYSTEM_DEP=	Makefile symbols.sort ${SYSTEM_OBJ}
SYSTEM_DEP=	Makefile ${SYSTEM_OBJ}
SYSTEM_LD_HEAD=	@echo loading $@; rm -f $@
SYSTEM_LD=	@strip=-x; [ X${DEBUG} = X-g ] && strip=-X; \
		${LD} $$strip -n -o $@ -e start ${SYSTEM_OBJ} vers.o
#SYSTEM_LD_TAIL= @echo rearranging symbols;\
#		symorder symbols.sort $@;\
SYSTEM_LD_TAIL=	@size $@; chmod 755 $@; \
		[ X${DEBUG} = X-g ] && { \
		echo cp $@ $@.gdb; rm -f $@.gdb; cp $@ $@.gdb; \
		echo strip -d $@; strip -d $@; } || true

vmunix: ${SYSTEM_DEP} swapgeneric.o newvers
	${SYSTEM_LD_HEAD}
	${SYSTEM_LD} swapgeneric.o
	${SYSTEM_LD_TAIL}

swapgeneric.o: ../../amiga/swapgeneric.c
	${NORMAL_C}

all: vmunix

newvers:
	sh $S/conf/newvers.sh
	${CC} $(CFLAGS) -c vers.c

clean:
	rm -f eddep *vmunix tags *.o locore.i [a-z]*.s \
		Errs errs linterrs makelinks

lint: /tmp param.c
	@lint -hbxn -DGENERIC -Dvolatile= ${COPTS} ${PARAM} -UKGDB \
	    ${AMIGA}/amiga/Locore.c ${CFILES} ${AMIGA}/amiga/swapgeneric.c \
	    ioconf.c param.c| \
	    grep -v 'struct/union .* never defined' | \
	    grep -v 'possible pointer alignment problem'

symbols.sort: ${AMIGA}/amiga/symbols.raw
	grep -v '^#' ${AMIGA}/amiga/symbols.raw \
	    | sed 's/^	//' | sort -u > symbols.sort

locore.o: assym.s ${AMIGA}/amiga/vectors.s ${AMIGA}/amiga/locore.s 
locore.o: ${AMIGA}/include/trap.h ${AMIGA}/include/psl.h ${AMIGA}/amiga/pte.h
locore.o: ${AMIGA}/include/cpu.h
	${CPP} -DLOCORE ${COPTS} ${AMIGA}/amiga/locore.s | ${AS} -o locore.o

# the following is necessary because autoconf.o depends on #if GENERIC
autoconf.o: Makefile

# the following are necessary because the files depend on the types of
# hp cpu's included in the system configuration
clock.o machdep.o autoconf.o conf.o: Makefile

# depend on network configuration
af.o uipc_proto.o locore.o: Makefile

# depend on maxusers
assym.s: Makefile

assym.s: genassym
	./genassym >assym.s

genassym:
	${CC} ${INCLUDES} ${IDENT} ${PARAM} -Dmc68020 -Damiga -o genassym \
	    ${AMIGA}/amiga/genassym.c

depend: assym.s param.c
	mkdep ${COPTS} ${CFILES} ioconf.c param.c
	mkdep -a -p ${INCLUDES} ${IDENT} ${PARAM} ${AMIGA}/amiga/genassym.c

links:
	egrep '#if' ${CFILES} | sed -f $S/conf/defines | \
	  sed -e 's/:.*//' -e 's/\.c/.o/' | sort -u > dontlink
	echo ${CFILES} | tr -s ' ' '\12' | sed 's/\.c/.o/' | \
	  sort -u | comm -23 - dontlink | \
	  sed 's,../.*/\(.*.o\),rm -f \1;ln -s ../GENERIC/\1 \1,' > makelinks
	sh makelinks && rm -f dontlink

tags:
	@echo "see $S/kern/Makefile for tags"

ioconf.o: ioconf.c
	${CC} -c ${CFLAGS} ioconf.c

param.c: $S/conf/param.c
	rm -f param.c
	cp $S/conf/param.c .

param.o: param.c Makefile
	${CC} -c ${CFLAGS} ${PARAM} param.c

isofs_bmap.o: $S/isofs/isofs_bmap.c
	${NORMAL_C}

isofs_lookup.o: $S/isofs/isofs_lookup.c
	${NORMAL_C}

isofs_node.o: $S/isofs/isofs_node.c
	${NORMAL_C}

isofs_util.o: $S/isofs/isofs_util.c
	${NORMAL_C}

isofs_vfsops.o: $S/isofs/isofs_vfsops.c
	${NORMAL_C}

isofs_vnops.o: $S/isofs/isofs_vnops.c
	${NORMAL_C}

dead_vnops.o: $S/kern/dead_vnops.c
	${NORMAL_C}

fifo_vnops.o: $S/kern/fifo_vnops.c
	${NORMAL_C}

init_main.o: $S/kern/init_main.c
	${NORMAL_C}

init_sysent.o: $S/kern/init_sysent.c
	${NORMAL_C}

kern_acct.o: $S/kern/kern_acct.c
	${NORMAL_C}

kern_clock.o: $S/kern/kern_clock.c
	${NORMAL_C}

kern_descrip.o: $S/kern/kern_descrip.c
	${NORMAL_C}

kern_exec.o: $S/kern/kern_exec.c
	${NORMAL_C}

kern_exit.o: $S/kern/kern_exit.c
	${NORMAL_C}

kern_fork.o: $S/kern/kern_fork.c
	${NORMAL_C}

kern_kinfo.o: $S/kern/kern_kinfo.c
	${NORMAL_C}

kern_ktrace.o: $S/kern/kern_ktrace.c
	${NORMAL_C}

kern_malloc.o: $S/kern/kern_malloc.c
	${NORMAL_C}

kern_physio.o: $S/kern/kern_physio.c
	${NORMAL_C}

kern_proc.o: $S/kern/kern_proc.c
	${NORMAL_C}

kern_prot.o: $S/kern/kern_prot.c
	${NORMAL_C}

kern_resource.o: $S/kern/kern_resource.c
	${NORMAL_C}

kern_sig.o: $S/kern/kern_sig.c
	${NORMAL_C}

kern_subr.o: $S/kern/kern_subr.c
	${NORMAL_C}

kern_synch.o: $S/kern/kern_synch.c
	${NORMAL_C}

kern_time.o: $S/kern/kern_time.c
	${NORMAL_C}

kern_xxx.o: $S/kern/kern_xxx.c
	${NORMAL_C}

spec_vnops.o: $S/kern/spec_vnops.c
	${NORMAL_C}

subr_log.o: $S/kern/subr_log.c
	${NORMAL_C}

subr_mcount.o: $S/kern/subr_mcount.c
	${NORMAL_C}

subr_prf.o: $S/kern/subr_prf.c
	${NORMAL_C}

subr_rlist.o: $S/kern/subr_rlist.c
	${NORMAL_C}

subr_xxx.o: $S/kern/subr_xxx.c
	${NORMAL_C}

sys_generic.o: $S/kern/sys_generic.c
	${NORMAL_C}

sys_process.o: $S/kern/sys_process.c
	${NORMAL_C}

sys_socket.o: $S/kern/sys_socket.c
	${NORMAL_C}

tty.o: $S/kern/tty.c
	${NORMAL_C}

tty_compat.o: $S/kern/tty_compat.c
	${NORMAL_C}

tty_conf.o: $S/kern/tty_conf.c
	${NORMAL_C}

tty_pty.o: $S/kern/tty_pty.c
	${NORMAL_C}

tty_ring.o: $S/kern/tty_ring.c
	${NORMAL_C}

tty_tty.o: $S/kern/tty_tty.c
	${NORMAL_C}

uipc_domain.o: $S/kern/uipc_domain.c
	${NORMAL_C}

uipc_mbuf.o: $S/kern/uipc_mbuf.c
	${NORMAL_C}

uipc_proto.o: $S/kern/uipc_proto.c
	${NORMAL_C}

uipc_socket.o: $S/kern/uipc_socket.c
	${NORMAL_C}

uipc_socket2.o: $S/kern/uipc_socket2.c
	${NORMAL_C}

uipc_syscalls.o: $S/kern/uipc_syscalls.c
	${NORMAL_C}

uipc_usrreq.o: $S/kern/uipc_usrreq.c
	${NORMAL_C}

vfs_bio.o: $S/kern/vfs_bio.c
	${NORMAL_C}

vfs_cache.o: $S/kern/vfs_cache.c
	${NORMAL_C}

vfs_conf.o: $S/kern/vfs_conf.c
	${NORMAL_C}

vfs_lookup.o: $S/kern/vfs_lookup.c
	${NORMAL_C}

vfs_subr.o: $S/kern/vfs_subr.c
	${NORMAL_C}

vfs_syscalls.o: $S/kern/vfs_syscalls.c
	${NORMAL_C}

vfs_vnops.o: $S/kern/vfs_vnops.c
	${NORMAL_C}

fdesc_vfsops.o: $S/miscfs/fdesc/fdesc_vfsops.c
	${NORMAL_C}

fdesc_vnops.o: $S/miscfs/fdesc/fdesc_vnops.c
	${NORMAL_C}

kernfs_vfsops.o: $S/miscfs/kernfs/kernfs_vfsops.c
	${NORMAL_C}

kernfs_vnops.o: $S/miscfs/kernfs/kernfs_vnops.c
	${NORMAL_C}

af.o: $S/net/af.c
	${NORMAL_C}

bpf.o: $S/net/bpf.c
	${NORMAL_C}

bpf_filter.o: $S/net/bpf_filter.c
	${NORMAL_C}

if.o: $S/net/if.c
	${NORMAL_C}

if_ethersubr.o: $S/net/if_ethersubr.c
	${NORMAL_C}

if_loop.o: $S/net/if_loop.c
	${NORMAL_C}

if_sl.o: $S/net/if_sl.c
	${NORMAL_C}

radix.o: $S/net/radix.c
	${NORMAL_C}

raw_cb.o: $S/net/raw_cb.c
	${NORMAL_C}

raw_usrreq.o: $S/net/raw_usrreq.c
	${NORMAL_C}

route.o: $S/net/route.c
	${NORMAL_C}

rtsock.o: $S/net/rtsock.c
	${NORMAL_C}

slcompress.o: $S/net/slcompress.c
	${NORMAL_C}

if_ether.o: $S/netinet/if_ether.c
	${NORMAL_C}

in.o: $S/netinet/in.c
	${NORMAL_C}

in_pcb.o: $S/netinet/in_pcb.c
	${NORMAL_C}

in_proto.o: $S/netinet/in_proto.c
	${NORMAL_C}

ip_icmp.o: $S/netinet/ip_icmp.c
	${NORMAL_C}

ip_input.o: $S/netinet/ip_input.c
	${NORMAL_C}

ip_output.o: $S/netinet/ip_output.c
	${NORMAL_C}

raw_ip.o: $S/netinet/raw_ip.c
	${NORMAL_C}

tcp_debug.o: $S/netinet/tcp_debug.c
	${NORMAL_C}

tcp_input.o: $S/netinet/tcp_input.c
	${NORMAL_C}

tcp_output.o: $S/netinet/tcp_output.c
	${NORMAL_C}

tcp_subr.o: $S/netinet/tcp_subr.c
	${NORMAL_C}

tcp_timer.o: $S/netinet/tcp_timer.c
	${NORMAL_C}

tcp_usrreq.o: $S/netinet/tcp_usrreq.c
	${NORMAL_C}

udp_usrreq.o: $S/netinet/udp_usrreq.c
	${NORMAL_C}

clnp_debug.o: $S/netiso/clnp_debug.c
	${NORMAL_C}

clnp_er.o: $S/netiso/clnp_er.c
	${NORMAL_C}

clnp_frag.o: $S/netiso/clnp_frag.c
	${NORMAL_C}

clnp_input.o: $S/netiso/clnp_input.c
	${NORMAL_C}

clnp_options.o: $S/netiso/clnp_options.c
	${NORMAL_C}

clnp_output.o: $S/netiso/clnp_output.c
	${NORMAL_C}

clnp_raw.o: $S/netiso/clnp_raw.c
	${NORMAL_C}

clnp_subr.o: $S/netiso/clnp_subr.c
	${NORMAL_C}

clnp_timer.o: $S/netiso/clnp_timer.c
	${NORMAL_C}

cltp_usrreq.o: $S/netiso/cltp_usrreq.c
	${NORMAL_C}

esis.o: $S/netiso/esis.c
	${NORMAL_C}

iso.o: $S/netiso/iso.c
	${NORMAL_C}

iso_chksum.o: $S/netiso/iso_chksum.c
	${NORMAL_C}

iso_pcb.o: $S/netiso/iso_pcb.c
	${NORMAL_C}

iso_proto.o: $S/netiso/iso_proto.c
	${NORMAL_C}

iso_snpac.o: $S/netiso/iso_snpac.c
	${NORMAL_C}

tp_astring.o: $S/netiso/tp_astring.c
	${NORMAL_C}

tp_cons.o: $S/netiso/tp_cons.c
	${NORMAL_C}

tp_driver.o: $S/netiso/tp_driver.c
	${NORMAL_C}

tp_emit.o: $S/netiso/tp_emit.c
	${NORMAL_C}

tp_inet.o: $S/netiso/tp_inet.c
	${NORMAL_C}

tp_input.o: $S/netiso/tp_input.c
	${NORMAL_C}

tp_iso.o: $S/netiso/tp_iso.c
	${NORMAL_C}

tp_meas.o: $S/netiso/tp_meas.c
	${NORMAL_C}

tp_output.o: $S/netiso/tp_output.c
	${NORMAL_C}

tp_pcb.o: $S/netiso/tp_pcb.c
	${NORMAL_C}

tp_subr.o: $S/netiso/tp_subr.c
	${NORMAL_C}

tp_subr2.o: $S/netiso/tp_subr2.c
	${NORMAL_C}

tp_timer.o: $S/netiso/tp_timer.c
	${NORMAL_C}

tp_trace.o: $S/netiso/tp_trace.c
	${NORMAL_C}

tp_usrreq.o: $S/netiso/tp_usrreq.c
	${NORMAL_C}

nfs_bio.o: $S/nfs/nfs_bio.c
	${NORMAL_C}

nfs_node.o: $S/nfs/nfs_node.c
	${NORMAL_C}

nfs_serv.o: $S/nfs/nfs_serv.c
	${NORMAL_C}

nfs_socket.o: $S/nfs/nfs_socket.c
	${NORMAL_C}

nfs_srvcache.o: $S/nfs/nfs_srvcache.c
	${NORMAL_C}

nfs_subs.o: $S/nfs/nfs_subs.c
	${NORMAL_C}

nfs_syscalls.o: $S/nfs/nfs_syscalls.c
	${NORMAL_C}

nfs_vfsops.o: $S/nfs/nfs_vfsops.c
	${NORMAL_C}

nfs_vnops.o: $S/nfs/nfs_vnops.c
	${NORMAL_C}

pcfs_conv.o: $S/pcfs/pcfs_conv.c
	${NORMAL_C}

pcfs_denode.o: $S/pcfs/pcfs_denode.c
	${NORMAL_C}

pcfs_fat.o: $S/pcfs/pcfs_fat.c
	${NORMAL_C}

pcfs_lookup.o: $S/pcfs/pcfs_lookup.c
	${NORMAL_C}

pcfs_vfsops.o: $S/pcfs/pcfs_vfsops.c
	${NORMAL_C}

pcfs_vnops.o: $S/pcfs/pcfs_vnops.c
	${NORMAL_C}

mfs_vfsops.o: $S/ufs/mfs_vfsops.c
	${NORMAL_C}

mfs_vnops.o: $S/ufs/mfs_vnops.c
	${NORMAL_C}

ufs_alloc.o: $S/ufs/ufs_alloc.c
	${NORMAL_C}

ufs_bmap.o: $S/ufs/ufs_bmap.c
	${NORMAL_C}

ufs_disksubr.o: $S/ufs/ufs_disksubr.c
	${NORMAL_C}

ufs_inode.o: $S/ufs/ufs_inode.c
	${NORMAL_C}

ufs_lockf.o: $S/ufs/ufs_lockf.c
	${NORMAL_C}

ufs_lookup.o: $S/ufs/ufs_lookup.c
	${NORMAL_C}

ufs_subr.o: $S/ufs/ufs_subr.c
	${NORMAL_C}

ufs_tables.o: $S/ufs/ufs_tables.c
	${NORMAL_C}

ufs_vfsops.o: $S/ufs/ufs_vfsops.c
	${NORMAL_C}

ufs_vnops.o: $S/ufs/ufs_vnops.c
	${NORMAL_C}

device_pager.o: $S/vm/device_pager.c
	${NORMAL_C}

kern_lock.o: $S/vm/kern_lock.c
	${NORMAL_C}

swap_pager.o: $S/vm/swap_pager.c
	${NORMAL_C}

vm_fault.o: $S/vm/vm_fault.c
	${NORMAL_C}

vm_glue.o: $S/vm/vm_glue.c
	${NORMAL_C}

vm_init.o: $S/vm/vm_init.c
	${NORMAL_C}

vm_kern.o: $S/vm/vm_kern.c
	${NORMAL_C}

vm_map.o: $S/vm/vm_map.c
	${NORMAL_C}

vm_meter.o: $S/vm/vm_meter.c
	${NORMAL_C}

vm_mmap.o: $S/vm/vm_mmap.c
	${NORMAL_C}

vm_object.o: $S/vm/vm_object.c
	${NORMAL_C}

vm_page.o: $S/vm/vm_page.c
	${NORMAL_C}

vm_pageout.o: $S/vm/vm_pageout.c
	${NORMAL_C}

vm_pager.o: $S/vm/vm_pager.c
	${NORMAL_C}

vm_swap.o: $S/vm/vm_swap.c
	${NORMAL_C}

vm_unix.o: $S/vm/vm_unix.c
	${NORMAL_C}

vm_user.o: $S/vm/vm_user.c
	${NORMAL_C}

vnode_pager.o: $S/vm/vnode_pager.c
	${NORMAL_C}

amiga_init.o: $S/arch/amiga/amiga/amiga_init.c
	${NORMAL_C}

autoconf.o: $S/arch/amiga/amiga/autoconf.c
	${NORMAL_C}

clock.o: $S/arch/amiga/amiga/clock.c
	${NORMAL_C}

conf.o: $S/arch/amiga/amiga/conf.c
	${NORMAL_C}

cons.o: $S/arch/amiga/amiga/cons.c
	${NORMAL_C}

disksubr.o: $S/arch/amiga/amiga/disksubr.c
	${NORMAL_C}

dkbad.o: $S/arch/amiga/amiga/dkbad.c
	${NORMAL_C}

in_cksum.o: $S/arch/amiga/amiga/in_cksum.c
	${NORMAL_C}

machdep.o: $S/arch/amiga/amiga/machdep.c
	${NORMAL_C_C}

mem.o: $S/arch/amiga/amiga/mem.c
	${NORMAL_C}

oc_cksum.o: $S/arch/amiga/amiga/oc_cksum.s
	${NORMAL_S}

pmap.o: $S/arch/amiga/amiga/pmap.c
	${NORMAL_C}

sys_machdep.o: $S/arch/amiga/amiga/sys_machdep.c
	${NORMAL_C}

trap.o: $S/arch/amiga/amiga/trap.c
	${NORMAL_C}

vm_machdep.o: $S/arch/amiga/amiga/vm_machdep.c
	${NORMAL_C}

dma.o: $S/arch/amiga/dev/dma.c
	${NORMAL_C}

vn.o: $S/arch/amiga/dev/vn.c
	${NORMAL_C}

scsi.o: $S/arch/amiga/dev/scsi.c
	${NORMAL_C}

sd.o: $S/arch/amiga/dev/sd.c
	${NORMAL_C}

st.o: $S/arch/amiga/dev/st.c
	${NORMAL_C}

ser.o: $S/arch/amiga/dev/ser.c
	${NORMAL_C}

kbd.o: $S/arch/amiga/dev/kbd.c
	${NORMAL_C}

ite.o: $S/arch/amiga/dev/ite.c
	${NORMAL_C}

ite_cc.o: $S/arch/amiga/dev/ite_cc.c
	${NORMAL_C}

ite_tg.o: $S/arch/amiga/dev/ite_tg.c
	${NORMAL_C}

kernel_font.o: $S/arch/amiga/dev/kernel_font.c
	${NORMAL_C}

kbdmap.o: $S/arch/amiga/dev/kbdmap.c
	${NORMAL_C}

grf.o: $S/arch/amiga/dev/grf.c
	${NORMAL_C}

grf_cc.o: $S/arch/amiga/dev/grf_cc.c
	${NORMAL_C}

grf_tg.o: $S/arch/amiga/dev/grf_tg.c
	${NORMAL_C}

