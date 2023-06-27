KERNEL_BUILD=/Users/sidqian/Downloads/summer/L2S/netbsd-src/sys/arch/luna68k/conf/TESTING
IDENT= \
	-DCOMPAT_UTILS \
	-DCOMPAT_44 \

MACHINE=luna68k
S=	/Users/sidqian/Downloads/summer/L2S/netbsd-src/sys
COPTS=-O2 -fno-reorder-blocks -fno-unwind-tables -fno-omit-frame-pointer
#	$NetBSD: Makefile.luna68k,v 1.30 2023/04/11 13:26:19 tsutsui Exp $

# Makefile for NetBSD
#
# This makefile is constructed from a machine description:
#	config machineid
# Most changes should be made in the machine description
#	/sys/arch/luna68k/conf/``machineid''
# after which you should do
#	config machineid
# Machine generic makefile changes should be made in
#	/sys/arch/luna68k/conf/Makefile.luna68k
# after which config should be rerun for all machines of that type.
# To specify debugging, add the config line: makeoptions DEBUG="-g"
# A better way is to specify -g only for a few files.
#
#	makeoptions DEBUGLIST="uvm* trap if_*"

MACHINE_ARCH=m68k
USETOOLS?=	no
NEED_OWN_INSTALL_TARGET?=no
.include <bsd.own.mk>

##
## (1) port identification
##
LUNA68K=	$S/arch/luna68k
GENASSYM_CONF=	${LUNA68K}/luna68k/genassym.cf

##
## (2) compile settings
##
CPPFLAGS+=	-Dluna68k
CFLAGS+=	-msoft-float
AFLAGS+=	-x assembler-with-cpp

##
## (3) libkern and compat
##
OPT_MODULAR=	1

##
## (4) local objects, compile rules, and dependencies
##
# for the Motorola 68040 Floating Point Software Product
.include "$S/arch/m68k/fpsp/Makefile.inc"

MD_OBJS=	locore.o
MD_LIBS=	${FPSP}
MD_CFILES=
MD_SFILES=	${LUNA68K}/luna68k/locore.s

locore.o: ${LUNA68K}/luna68k/locore.s assym.h
	${NORMAL_S}

.include "$S/arch/m68k/conf/Makefile.kern.m68k.inc"

##
## (5) link settings
##
LINKFORMAT=	-N
TEXTADDR?=	0xc000

##
## (6) port specific target dependencies
##

# depend on CPU configuration
machdep.c trap.o: Makefile

copy.o copypage.o lock_stubs.o: assym.h

##
## (7) misc settings
##

##
## (8) config(8) generated machinery
##
EXTRA_INCLUDES+=	-I$S/external/bsd/libnv/dist
EXTRA_INCLUDES+=	-I$S/external/bsd/acpica/dist
EXTRA_INCLUDES+=	-I$S/../common/lib/libx86emu
EXTRA_INCLUDES+=	-I$S/../common/lib/libc/misc

ALLFILES= \
	arch/luna68k/luna68k/autoconf.c \
	arch/luna68k/luna68k/clock.c \
	arch/luna68k/luna68k/disksubr.c \
	arch/luna68k/luna68k/isr.c \
	arch/luna68k/luna68k/machdep.c \
	arch/luna68k/luna68k/pmap_bootstrap.c \
	arch/luna68k/luna68k/trap.c \
	arch/m68k/m68k/cacheops.c \
	arch/m68k/m68k/db_memrw.c \
	arch/m68k/m68k/fpu.c \
	arch/m68k/m68k/m68k_trap.c \
	arch/m68k/m68k/pmap_motorola.c \
	arch/m68k/m68k/procfs_machdep.c \
	arch/m68k/m68k/sys_machdep.c \
	arch/m68k/m68k/vm_machdep.c \
	dev/cons.c \
	arch/luna68k/luna68k/mainbus.c \
	arch/luna68k/dev/timekeeper.c \
	arch/luna68k/dev/lcd.c \
	arch/luna68k/dev/if_le.c \
	arch/luna68k/dev/sio.c \
	arch/luna68k/dev/siotty.c \
	arch/luna68k/dev/xpbus.c \
	arch/luna68k/dev/xpcmd.c \
	arch/luna68k/dev/xplxfirm.c \
	arch/luna68k/dev/xp.c \
	arch/luna68k/dev/psgpam.c \
	arch/luna68k/dev/psgpam_enc.c \
	arch/luna68k/dev/psgpam_table.c \
	arch/luna68k/dev/lunaws.c \
	arch/luna68k/dev/omkbdmap.c \
	arch/luna68k/dev/lunafb.c \
	arch/luna68k/dev/omrasops.c \
	arch/luna68k/dev/spc.c \
	arch/m68k/m68k/copy.s \
	arch/m68k/m68k/copypage.s \
	arch/m68k/m68k/core_machdep.c \
	arch/m68k/m68k/cpu_in_cksum.c \
	arch/m68k/m68k/db_disasm.c \
	arch/m68k/m68k/db_interface.c \
	arch/m68k/m68k/db_trace.c \
	arch/m68k/m68k/kobj_machdep.c \
	arch/m68k/m68k/lock_stubs.s \
	arch/m68k/m68k/m68k_machdep.c \
	arch/m68k/m68k/oc_cksum.s \
	arch/m68k/m68k/process_machdep.c \
	arch/m68k/m68k/reenter_syscall.s \
	arch/m68k/m68k/regdump.c \
	arch/m68k/m68k/sig_machdep.c \
	arch/m68k/m68k/m68k_syscall.c \
	arch/m68k/m68k/compat_13_machdep.c \
	arch/m68k/m68k/compat_16_machdep.c \
	dev/scsipi/scsipiconf.c \
	dev/scsipi/scsipi_base.c \
	dev/scsipi/scsipi_ioctl.c \
	dev/scsipi/scsi_base.c \
	dev/scsipi/scsiconf.c \
	dev/scsipi/scsi_subr.c \
	dev/scsipi/cd.c \
	dev/scsipi/sd.c \
	dev/scsipi/st.c \
	dev/scsipi/st_scsi.c \
	compat/aoutm68k/aoutm68k_exec.c \
	compat/aoutm68k/aoutm68k_stat.c \
	compat/aoutm68k/aoutm68k_sysent.c \
	compat/aoutm68k/aoutm68k_mod.c \
	dev/dev_verbose.c \
	dev/ic/am7990.c \
	dev/ic/lance.c \
	dev/ic/mb89352.c \
	kern/kern_drvctl.c \
	compat/common/compat_exec.c \
	compat/common/compat_util.c \
	compat/common/compat_43_mod.c \
	compat/common/kern_exit_43.c \
	compat/common/kern_info_43.c \
	compat/common/kern_resource_43.c \
	compat/common/kern_sig_43.c \
	compat/common/tty_43.c \
	compat/common/uipc_syscalls_43.c \
	compat/common/vfs_syscalls_43.c \
	compat/common/vm_43.c \
	compat/common/if_43.c \
	compat/common/compat_sysctl_09_43.c \
	compat/common/compat_12_mod.c \
	compat/common/kern_xxx_12.c \
	compat/common/vfs_syscalls_12.c \
	compat/common/vm_12.c \
	compat/common/compat_13_mod.c \
	compat/common/kern_sig_13.c \
	compat/common/uvm_13.c \
	compat/common/compat_14_mod.c \
	compat/common/rtsock_14.c \
	compat/common/compat_16_mod.c \
	compat/common/kern_sig_16.c \
	compat/common/compat_20_mod.c \
	compat/common/vfs_syscalls_20.c \
	compat/common/if43_20.c \
	compat/common/ieee80211_20.c \
	compat/common/compat_30_mod.c \
	compat/common/kern_time_30.c \
	compat/common/vfs_syscalls_30.c \
	compat/common/uipc_syscalls_30.c \
	compat/common/bio_30.c \
	compat/common/vnd_30.c \
	compat/common/usb_subr_30.c \
	compat/common/compat_40_mod.c \
	compat/common/vfs_syscalls_40.c \
	compat/common/uipc_syscalls_40.c \
	compat/common/sysmon_power_40.c \
	compat/common/compat_50_mod.c \
	compat/common/kern_50.c \
	compat/common/kern_time_50.c \
	compat/common/kern_select_50.c \
	compat/common/rndpseudo_50.c \
	compat/common/rtsock_50.c \
	compat/common/vfs_syscalls_50.c \
	compat/common/uipc_syscalls_50.c \
	compat/common/vnd_50.c \
	compat/common/clockctl_50.c \
	compat/common/if_spppsubr50.c \
	compat/common/uvm_50.c \
	compat/common/kern_uipc_socket_50.c \
	compat/common/compat_60_mod.c \
	compat/common/kern_sa_60.c \
	compat/common/tty_60.c \
	compat/common/kern_time_60.c \
	compat/common/kern_cpu_60.c \
	compat/common/ccd_60.c \
	compat/common/compat_70_mod.c \
	compat/common/rtsock_70.c \
	compat/common/uipc_usrreq_70.c \
	compat/common/compat_80_mod.c \
	compat/common/kern_mod_80.c \
	compat/common/if_media_80.c \
	compat/common/compat_90_mod.c \
	compat/common/vfs_syscalls_90.c \
	compat/common/compat_100_mod.c \
	compat/common/compat_sysv_14_mod.c \
	compat/common/sysv_msg_14.c \
	compat/common/sysv_sem_14.c \
	compat/common/sysv_shm_14.c \
	compat/common/compat_sysv_50_mod.c \
	compat/common/sysv_ipc_50.c \
	compat/common/sysv_msg_50.c \
	compat/common/sysv_sem_50.c \
	compat/common/sysv_shm_50.c \
	crypto/aes/aes_bear.c \
	crypto/aes/aes_ccm.c \
	crypto/aes/aes_ccm_mbuf.c \
	crypto/aes/aes_ct.c \
	crypto/aes/aes_ct_dec.c \
	crypto/aes/aes_ct_enc.c \
	crypto/aes/aes_impl.c \
	crypto/aes/aes_selftest.c \
	crypto/chacha/chacha_impl.c \
	crypto/chacha/chacha_ref.c \
	crypto/chacha/chacha_selftest.c \
	crypto/nist_hash_drbg/nist_hash_drbg.c \
	crypto/cprng_fast/cprng_fast.c \
	crypto/blake2/blake2s.c \
	dev/sysmon/sysmon.c \
	dev/sysmon/sysmon_wdog.c \
	dev/sysmon/swwdog.c \
	netatalk/at_print.c \
	netinet/igmp.c \
	netinet/in.c \
	netinet/in_offload.c \
	netinet/in_pcb.c \
	netinet/in_print.c \
	netinet/in_proto.c \
	netinet/ip_icmp.c \
	netinet/ip_input.c \
	netinet/ip_output.c \
	netinet/ip_reass.c \
	netinet/raw_ip.c \
	netinet/tcp_input.c \
	netinet/tcp_output.c \
	netinet/tcp_sack.c \
	netinet/tcp_subr.c \
	netinet/tcp_syncache.c \
	netinet/tcp_timer.c \
	netinet/tcp_usrreq.c \
	netinet/tcp_congctl.c \
	netinet/tcp_vtw.c \
	netinet/udp_usrreq.c \
	netinet/portalgo.c \
	netinet6/dest6.c \
	netinet6/frag6.c \
	netinet6/icmp6.c \
	netinet6/in6.c \
	netinet6/in6_cksum.c \
	netinet6/in6_ifattach.c \
	netinet6/in6_offload.c \
	netinet6/in6_pcb.c \
	netinet6/in6_print.c \
	netinet6/in6_proto.c \
	netinet6/in6_src.c \
	netinet6/ip6_forward.c \
	netinet6/ip6_input.c \
	netinet6/ip6_mroute.c \
	netinet6/ip6_output.c \
	netinet6/mld6.c \
	netinet6/nd6.c \
	netinet6/nd6_nbr.c \
	netinet6/nd6_rtr.c \
	netinet6/raw_ip6.c \
	netinet6/route6.c \
	netinet6/scope6.c \
	netinet6/udp6_usrreq.c \
	ddb/db_access.c \
	ddb/db_autoconf.c \
	ddb/db_break.c \
	ddb/db_command.c \
	ddb/db_cpu.c \
	ddb/db_examine.c \
	ddb/db_expr.c \
	ddb/db_input.c \
	ddb/db_kernel.c \
	ddb/db_lex.c \
	ddb/db_lwp.c \
	ddb/db_output.c \
	ddb/db_panic.c \
	ddb/db_proc.c \
	ddb/db_print.c \
	ddb/db_run.c \
	ddb/db_sym.c \
	ddb/db_trap.c \
	ddb/db_variables.c \
	ddb/db_watch.c \
	ddb/db_write_cmd.c \
	ddb/db_xxx.c \
	dev/audio/alaw.c \
	dev/audio/linear.c \
	dev/audio/audio.c \
	dev/audio/mulaw.c \
	external/bsd/drm2/drm/drm_stub.c \
	dev/wscons/wsdisplay.c \
	dev/wscons/wsdisplay_compat_usl.c \
	dev/wscons/wsdisplay_util.c \
	dev/wscons/wsemulconf.c \
	dev/wscons/wsemul_dumb.c \
	dev/wscons/wsemul_vt100.c \
	dev/wscons/wsemul_vt100_subr.c \
	dev/wscons/wsemul_vt100_chars.c \
	dev/wscons/wsemul_vt100_keys.c \
	dev/wscons/wsevent.c \
	dev/wscons/wsevent_50.c \
	dev/wscons/wskbd.c \
	dev/wscons/wskbdutil.c \
	dev/wscons/wsmouse.c \
	dev/wscons/wsmux.c \
	dev/wscons/wsbellmux.c \
	dev/wsfont/wsfont.c \
	fs/cd9660/cd9660_bmap.c \
	fs/cd9660/cd9660_lookup.c \
	fs/cd9660/cd9660_node.c \
	fs/cd9660/cd9660_rrip.c \
	fs/cd9660/cd9660_util.c \
	fs/cd9660/cd9660_vfsops.c \
	fs/cd9660/cd9660_vnops.c \
	fs/ptyfs/ptyfs_subr.c \
	fs/ptyfs/ptyfs_vfsops.c \
	fs/ptyfs/ptyfs_vnops.c \
	fs/puffs/puffs_compat.c \
	fs/tmpfs/tmpfs_fifoops.c \
	fs/tmpfs/tmpfs_mem.c \
	fs/tmpfs/tmpfs_rename.c \
	fs/tmpfs/tmpfs_specops.c \
	fs/tmpfs/tmpfs_subr.c \
	fs/tmpfs/tmpfs_vfsops.c \
	fs/tmpfs/tmpfs_vnops.c \
	miscfs/kernfs/kernfs_vfsops.c \
	miscfs/kernfs/kernfs_vnops.c \
	miscfs/nullfs/null_vfsops.c \
	miscfs/nullfs/null_vnops.c \
	miscfs/procfs/procfs_auxv.c \
	miscfs/procfs/procfs_cmdline.c \
	miscfs/procfs/procfs_fd.c \
	miscfs/procfs/procfs_fpregs.c \
	miscfs/procfs/procfs_limit.c \
	miscfs/procfs/procfs_linux.c \
	miscfs/procfs/procfs_map.c \
	miscfs/procfs/procfs_mem.c \
	miscfs/procfs/procfs_note.c \
	miscfs/procfs/procfs_regs.c \
	miscfs/procfs/procfs_status.c \
	miscfs/procfs/procfs_subr.c \
	miscfs/procfs/procfs_vfsops.c \
	miscfs/procfs/procfs_vnops.c \
	nfs/krpc_subr.c \
	nfs/nfs_bio.c \
	nfs/nfs_boot.c \
	nfs/nfs_bootdhcp.c \
	nfs/nfs_clntsocket.c \
	nfs/nfs_clntsubs.c \
	nfs/nfs_iod.c \
	nfs/nfs_kq.c \
	nfs/nfs_node.c \
	nfs/nfs_socket.c \
	nfs/nfs_subs.c \
	nfs/nfs_vfsops.c \
	nfs/nfs_vnops.c \
	ufs/ffs/ffs_alloc.c \
	ufs/ffs/ffs_balloc.c \
	ufs/ffs/ffs_extattr.c \
	ufs/ffs/ffs_inode.c \
	ufs/ffs/ffs_snapshot.c \
	ufs/ffs/ffs_subr.c \
	ufs/ffs/ffs_tables.c \
	ufs/ffs/ffs_vfsops.c \
	ufs/ffs/ffs_vnops.c \
	ufs/ffs/ffs_wapbl.c \
	ufs/mfs/mfs_vfsops.c \
	ufs/mfs/mfs_vnops.c \
	ufs/mfs/mfs_miniroot.c \
	ufs/ufs/ufs_acl.c \
	ufs/ufs/ufs_bmap.c \
	ufs/ufs/ufs_inode.c \
	ufs/ufs/ufs_lookup.c \
	ufs/ufs/ufs_rename.c \
	ufs/ufs/ufs_vfsops.c \
	ufs/ufs/ufs_vnops.c \
	uvm/uvm_amap.c \
	uvm/uvm_anon.c \
	uvm/uvm_aobj.c \
	uvm/uvm_bio.c \
	uvm/uvm_coredump.c \
	uvm/uvm_device.c \
	uvm/uvm_fault.c \
	uvm/uvm_glue.c \
	uvm/uvm_init.c \
	uvm/uvm_io.c \
	uvm/uvm_km.c \
	uvm/uvm_loan.c \
	uvm/uvm_map.c \
	uvm/uvm_meter.c \
	uvm/uvm_mmap.c \
	uvm/uvm_mremap.c \
	uvm/uvm_object.c \
	uvm/uvm_page.c \
	uvm/uvm_page_array.c \
	uvm/uvm_page_status.c \
	uvm/uvm_pager.c \
	uvm/uvm_pdaemon.c \
	uvm/uvm_pdpolicy_clock.c \
	uvm/uvm_pgflcache.c \
	uvm/uvm_pglist.c \
	uvm/uvm_physseg.c \
	uvm/uvm_readahead.c \
	uvm/uvm_stat.c \
	uvm/uvm_swap.c \
	uvm/uvm_unix.c \
	uvm/uvm_user.c \
	uvm/uvm_vnode.c \
	secmodel/secmodel.c \
	conf/debugsyms.c \
	conf/param.c \
	kern/bufq_disksort.c \
	kern/bufq_fcfs.c \
	kern/compat_stub.c \
	kern/core_elf32.c \
	kern/core_netbsd.c \
	kern/cnmagic.c \
	kern/exec_aout.c \
	kern/exec_elf32.c \
	kern/exec_script.c \
	kern/exec_subr.c \
	kern/init_main.c \
	kern/init_sysctl.c \
	kern/init_sysctl_base.c \
	kern/init_sysent.c \
	kern/kern_acct.c \
	kern/kern_auth.c \
	kern/kern_cfglock.c \
	kern/kern_clock.c \
	kern/kern_condvar.c \
	kern/kern_core.c \
	kern/kern_cpu.c \
	kern/kern_descrip.c \
	kern/kern_entropy.c \
	kern/kern_event.c \
	kern/kern_exec.c \
	kern/kern_exit.c \
	kern/kern_fork.c \
	kern/kern_idle.c \
	kern/kern_hook.c \
	kern/kern_kthread.c \
	kern/kern_ktrace.c \
	kern/kern_ktrace_vfs.c \
	kern/kern_ksyms.c \
	kern/kern_lock.c \
	kern/kern_lwp.c \
	kern/kern_malloc.c \
	kern/kern_module.c \
	kern/kern_module_hook.c \
	kern/kern_module_vfs.c \
	kern/kern_mutex.c \
	kern/kern_mutex_obj.c \
	kern/kern_ntptime.c \
	kern/kern_physio.c \
	kern/kern_pmf.c \
	kern/kern_proc.c \
	kern/kern_prot.c \
	kern/kern_ras.c \
	kern/kern_rate.c \
	kern/kern_reboot.c \
	kern/kern_resource.c \
	kern/kern_runq.c \
	kern/kern_rwlock.c \
	kern/kern_rwlock_obj.c \
	kern/kern_scdebug.c \
	kern/kern_crashme.c \
	kern/kern_sig.c \
	kern/kern_sleepq.c \
	kern/kern_softint.c \
	kern/kern_ssp.c \
	kern/kern_stub.c \
	kern/kern_subr.c \
	kern/kern_synch.c \
	kern/kern_syscall.c \
	kern/kern_sysctl.c \
	kern/kern_tc.c \
	kern/kern_threadpool.c \
	kern/kern_time.c \
	kern/kern_timeout.c \
	kern/kern_turnstile.c \
	kern/kern_todr.c \
	kern/kern_uidinfo.c \
	kern/kern_uuid.c \
	kern/sched_4bsd.c \
	kern/subr_acl_posix1e.c \
	kern/subr_acl_nfs4.c \
	kern/subr_autoconf.c \
	kern/subr_blist.c \
	kern/subr_bufq.c \
	kern/subr_callback.c \
	kern/subr_cprng.c \
	kern/subr_cpu.c \
	kern/subr_cpufreq.c \
	kern/subr_copy.c \
	kern/subr_device.c \
	kern/subr_devsw.c \
	kern/subr_disk.c \
	kern/subr_disklabel.c \
	kern/subr_disk_open.c \
	kern/subr_emul.c \
	kern/subr_evcnt.c \
	kern/subr_exec_fd.c \
	kern/subr_extent.c \
	kern/subr_hash.c \
	kern/subr_humanize.c \
	kern/subr_interrupt.c \
	kern/subr_iostat.c \
	kern/subr_ipi.c \
	kern/subr_kcpuset.c \
	kern/subr_kmem.c \
	kern/subr_kobj.c \
	kern/subr_kobj_vfs.c \
	kern/subr_localcount.c \
	kern/subr_lockdebug.c \
	kern/subr_log.c \
	kern/subr_lwp_specificdata.c \
	kern/subr_once.c \
	kern/subr_optstr.c \
	kern/subr_pcq.c \
	kern/subr_pcu.c \
	kern/subr_percpu.c \
	kern/subr_pool.c \
	kern/subr_prf.c \
	kern/subr_prof.c \
	kern/subr_pserialize.c \
	kern/subr_psref.c \
	kern/subr_specificdata.c \
	kern/subr_time.c \
	kern/subr_thmap.c \
	kern/subr_userconf.c \
	kern/subr_vmem.c \
	kern/subr_workqueue.c \
	kern/subr_xcall.c \
	kern/sys_aio.c \
	kern/sys_descrip.c \
	kern/sys_eventfd.c \
	kern/sys_futex.c \
	kern/sys_generic.c \
	kern/sys_getrandom.c \
	kern/sys_module.c \
	kern/sys_mqueue.c \
	kern/sys_lwp.c \
	kern/sys_pipe.c \
	kern/sys_process.c \
	kern/sys_process_lwpstatus.c \
	kern/sys_ptrace.c \
	kern/sys_ptrace_common.c \
	kern/sys_pset.c \
	kern/sys_select.c \
	kern/sys_sig.c \
	kern/sys_sched.c \
	kern/sys_socket.c \
	kern/sys_timerfd.c \
	kern/sysv_ipc.c \
	kern/sysv_msg.c \
	kern/sysv_sem.c \
	kern/sysv_shm.c \
	kern/tty.c \
	kern/tty_conf.c \
	kern/tty_bsdpty.c \
	kern/tty_pty.c \
	kern/tty_ptm.c \
	kern/tty_subr.c \
	kern/tty_tty.c \
	kern/uipc_accf.c \
	kern/uipc_domain.c \
	kern/uipc_mbuf.c \
	kern/uipc_mbufdebug.c \
	net/link_proto.c \
	kern/uipc_proto.c \
	kern/uipc_sem.c \
	kern/uipc_socket.c \
	kern/uipc_socket2.c \
	kern/uipc_syscalls.c \
	kern/uipc_usrreq.c \
	kern/vfs_acl.c \
	kern/vfs_bio.c \
	kern/vfs_cache.c \
	kern/vfs_cwd.c \
	kern/vfs_dirhash.c \
	kern/vfs_getcwd.c \
	kern/vfs_hooks.c \
	kern/vfs_init.c \
	kern/vfs_lockf.c \
	kern/vfs_lookup.c \
	kern/vfs_mount.c \
	kern/vfs_quotactl.c \
	kern/vfs_subr.c \
	kern/vfs_syscalls.c \
	kern/vfs_trans.c \
	kern/vfs_vnode.c \
	kern/vfs_vnops.c \
	kern/vfs_wapbl.c \
	kern/vfs_xattr.c \
	kern/vnode_if.c \
	miscfs/deadfs/dead_vfsops.c \
	miscfs/deadfs/dead_vnops.c \
	miscfs/fifofs/fifo_vnops.c \
	miscfs/genfs/genfs_io.c \
	miscfs/genfs/genfs_rename.c \
	miscfs/genfs/genfs_vfsops.c \
	miscfs/genfs/genfs_vnops.c \
	miscfs/genfs/layer_subr.c \
	miscfs/genfs/layer_vfsops.c \
	miscfs/genfs/layer_vnops.c \
	miscfs/specfs/spec_vnops.c \
	dev/clock_subr.c \
	dev/clockctl.c \
	dev/dksubr.c \
	dev/dkwedge/dk.c \
	dev/fss.c \
	dev/md.c \
	dev/mm.c \
	dev/random.c \
	net/bpf.c \
	net/bpf_filter.c \
	net/bpf_stub.c \
	net/dl_print.c \
	net/ether_sw_offload.c \
	net/if.c \
	net/if_bridge.c \
	net/bridgestp.c \
	net/if_ethersubr.c \
	net/if_llatbl.c \
	net/if_loop.c \
	net/if_media.c \
	net/if_stats.c \
	net/if_vlan.c \
	net/nd.c \
	net/pfil.c \
	net/pktqueue.c \
	net/net_stats.c \
	net/radix.c \
	net/raw_cb.c \
	net/raw_usrreq.c \
	net/route.c \
	net/rss_config.c \
	net/rtbl.c \
	net/rtsock.c \
	net/toeplitz.c \
	netinet/if_arp.c \
	netinet/in4_cksum.c \
	netinet/in_cksum.c \
	netinet/ip_encap.c \
	netinet/wqinput.c \
	secmodel/suser/secmodel_suser.c \
	secmodel/securelevel/secmodel_securelevel.c \
	secmodel/extensions/secmodel_extensions.c \
	secmodel/extensions/secmodel_extensions_vfs.c \
	secmodel/bsd44/secmodel_bsd44.c \
	net/agr/if_agr.c \
	net/agr/if_agrsoftc.c \
	net/agr/if_agrsubr.c \
	net/agr/if_agrtimer.c \
	net/agr/if_agrmonitor.c \
	net/agr/if_agrether.c \
	net/agr/if_agrether_hash.c \
	net/agr/ieee8023ad_marker.c \
	net/agr/ieee8023ad_lacp.c \
	net/agr/ieee8023ad_lacp_select.c \
	net/agr/ieee8023ad_lacp_timer.c \
	net/agr/ieee8023ad_lacp_sm_rx.c \
	net/agr/ieee8023ad_lacp_sm_mux.c \
	net/agr/ieee8023ad_lacp_sm_ptx.c \
	net/agr/ieee8023ad_lacp_sm_tx.c \
	net/agr/ieee8023ad_lacp_debug.c \
	net/agr/ieee8023_tlv.c \
	devsw.c \
	ioconf.c \
	swapnetbsd.c \

#%OFILES

#%CFILES

#%SFILES

KERNELS+=netbsd

fpu.o: $S/arch/m68k/m68k/fpu.c
	${M68K_KERN_FPU}


##
## (9) port independent kernel machinery
##
.include "$S/conf/Makefile.kern.inc"

##
## (10) Appending make options.
##
OPT.db_memrw.c+= ddb
OPT.procfs_machdep.c+= procfs
OPT.timekeeper.c+= clock
OPT.lcd.c+= lcd
OPT.if_le.c+= le
OPT.sio.c+= sio
OPT.siotty.c+= siotty
OPT.xpbus.c+= xpbus
OPT.xpcmd.c+= xpbus
OPT.xplxfirm.c+= xpbus
OPT.xp.c+= xp
OPT.psgpam.c+= psgpam
OPT.psgpam_enc.c+= psgpam
OPT.psgpam_table.c+= psgpam
OPT.lunaws.c+= ws
OPT.omkbdmap.c+= ws
OPT.lunafb.c+= fb
OPT.omrasops.c+= fb
OPT.spc.c+= spc
OPT.core_machdep.c+= coredump
OPT.cpu_in_cksum.c+= inet inet6 cpu_in_cksum
OPT.db_disasm.c+= ddb
OPT.db_interface.c+= ddb
OPT.db_trace.c+= ddb
OPT.kobj_machdep.c+= modular
OPT.oc_cksum.c+= inet inet6 cpu_in_cksum
OPT.compat_13_machdep.c+= compat_13
OPT.compat_16_machdep.c+= compat_16
OPT.scsipiconf.c+= scsi_core
OPT.scsipi_base.c+= scsi_core
OPT.scsipi_ioctl.c+= scsi_core
OPT.scsi_base.c+= scsi_core
OPT.scsiconf.c+= scsibus
OPT.scsi_subr.c+= scsibus
OPT.cd.c+= cd
OPT.sd.c+= sd
OPT.st.c+= st st_scsibus
OPT.st_scsi.c+= st_scsibus
OPT.aoutm68k_exec.c+= compat_aout_m68k
OPT.aoutm68k_stat.c+= compat_aout_m68k
OPT.aoutm68k_sysent.c+= compat_aout_m68k
OPT.aoutm68k_mod.c+= compat_aout_m68k
OPT.am7990.c+= le24
OPT.lance.c+= le24
OPT.mb89352.c+= spc
OPT.kern_drvctl.c+= drvctl
OPT.compat_exec.c+= compat_utils
OPT.compat_util.c+= compat_utils
OPT.compat_43_mod.c+= compat_43
OPT.kern_exit_43.c+= compat_43
OPT.kern_info_43.c+= compat_43
OPT.kern_resource_43.c+= compat_43
OPT.kern_sig_43.c+= compat_43
OPT.tty_43.c+= compat_43
OPT.uipc_syscalls_43.c+= compat_43
OPT.vfs_syscalls_43.c+= compat_43
OPT.vm_43.c+= compat_43
OPT.if_43.c+= compat_43
OPT.compat_sysctl_09_43.c+= compat_43
OPT.compat_12_mod.c+= compat_12
OPT.kern_xxx_12.c+= compat_12
OPT.vfs_syscalls_12.c+= compat_12
OPT.vm_12.c+= compat_12
OPT.compat_13_mod.c+= compat_13
OPT.kern_sig_13.c+= compat_13
OPT.uvm_13.c+= compat_13
OPT.compat_14_mod.c+= compat_14
OPT.rtsock_14.c+= compat_14
OPT.compat_16_mod.c+= compat_16
OPT.kern_sig_16.c+= compat_16
OPT.compat_20_mod.c+= compat_20
OPT.vfs_syscalls_20.c+= compat_20
OPT.if43_20.c+= compat_20
OPT.ieee80211_20.c+= compat_20
OPT.compat_30_mod.c+= compat_30
OPT.kern_time_30.c+= compat_30
OPT.vfs_syscalls_30.c+= compat_30
OPT.uipc_syscalls_30.c+= compat_30
OPT.bio_30.c+= compat_30
OPT.vnd_30.c+= compat_30
OPT.usb_subr_30.c+= compat_30
OPT.compat_40_mod.c+= compat_40
OPT.vfs_syscalls_40.c+= compat_40
OPT.uipc_syscalls_40.c+= compat_40
OPT.sysmon_power_40.c+= compat_40
OPT.compat_50_mod.c+= compat_50
OPT.kern_50.c+= compat_50
OPT.kern_time_50.c+= compat_50
OPT.kern_select_50.c+= compat_50
OPT.rndpseudo_50.c+= compat_50
OPT.rtsock_50.c+= compat_50
OPT.vfs_syscalls_50.c+= compat_50
OPT.uipc_syscalls_50.c+= compat_50
OPT.vnd_50.c+= compat_50
OPT.clockctl_50.c+= compat_50
OPT.if_spppsubr50.c+= compat_50
OPT.uvm_50.c+= compat_50
OPT.kern_uipc_socket_50.c+= compat_50
OPT.compat_60_mod.c+= compat_60
OPT.kern_sa_60.c+= compat_60
OPT.tty_60.c+= compat_60
OPT.kern_time_60.c+= compat_60
OPT.kern_cpu_60.c+= compat_60
OPT.ccd_60.c+= compat_60
OPT.compat_70_mod.c+= compat_70
OPT.rtsock_70.c+= compat_70
OPT.uipc_usrreq_70.c+= compat_70
OPT.compat_80_mod.c+= compat_80
OPT.kern_mod_80.c+= compat_80
OPT.if_media_80.c+= compat_80
OPT.compat_90_mod.c+= compat_90
OPT.vfs_syscalls_90.c+= compat_90
OPT.compat_100_mod.c+= compat_100
OPT.compat_sysv_14_mod.c+= compat_14 sysvmsg sysvshm sysvsem
OPT.sysv_msg_14.c+= compat_14 sysvmsg
OPT.sysv_sem_14.c+= compat_14 sysvsem
OPT.sysv_shm_14.c+= compat_14 sysvshm
OPT.compat_sysv_50_mod.c+= compat_50 sysvmsg sysvshm sysvsem
OPT.sysv_ipc_50.c+= compat_50 sysvmsg sysvshm sysvsem
OPT.sysv_msg_50.c+= compat_50 sysvmsg
OPT.sysv_sem_50.c+= compat_50 sysvsem
OPT.sysv_shm_50.c+= compat_50 sysvshm
OPT.aes_bear.c+= aes
OPT.aes_ccm.c+= aes
OPT.aes_ccm_mbuf.c+= aes
OPT.aes_ct.c+= aes
OPT.aes_ct_dec.c+= aes
OPT.aes_ct_enc.c+= aes
OPT.aes_impl.c+= aes
OPT.aes_selftest.c+= aes
OPT.chacha_impl.c+= chacha
OPT.chacha_ref.c+= chacha
OPT.chacha_selftest.c+= chacha
OPT.cprng_fast.c+= cprng_fast
OPT.sysmon.c+= sysmon
OPT.sysmon_wdog.c+= sysmon_wdog
OPT.swwdog.c+= swwdog
OPT.igmp.c+= inet
OPT.in.c+= inet
OPT.in_offload.c+= inet
OPT.in_pcb.c+= inet
OPT.in_proto.c+= inet
OPT.ip_icmp.c+= inet
OPT.ip_input.c+= inet
OPT.ip_output.c+= inet
OPT.ip_reass.c+= inet
OPT.raw_ip.c+= inet
OPT.tcp_input.c+= inet inet6
OPT.tcp_output.c+= inet inet6
OPT.tcp_sack.c+= inet inet6
OPT.tcp_subr.c+= inet inet6
OPT.tcp_syncache.c+= inet inet6
OPT.tcp_timer.c+= inet inet6
OPT.tcp_usrreq.c+= inet inet6
OPT.tcp_congctl.c+= inet inet6
OPT.tcp_vtw.c+= inet inet6
OPT.udp_usrreq.c+= inet inet6
OPT.portalgo.c+= inet inet6
OPT.dest6.c+= inet6
OPT.frag6.c+= inet6
OPT.icmp6.c+= inet6
OPT.in6.c+= inet6
OPT.in6_cksum.c+= inet6
OPT.in6_ifattach.c+= inet6
OPT.in6_offload.c+= inet6
OPT.in6_pcb.c+= inet6
OPT.in6_proto.c+= inet6
OPT.in6_src.c+= inet6
OPT.ip6_forward.c+= inet6
OPT.ip6_input.c+= inet6
OPT.ip6_mroute.c+= inet6
OPT.ip6_output.c+= inet6
OPT.mld6.c+= inet6
OPT.nd6.c+= inet6
OPT.nd6_nbr.c+= inet6
OPT.nd6_rtr.c+= inet6
OPT.raw_ip6.c+= inet6
OPT.route6.c+= inet6
OPT.scope6.c+= inet6
OPT.udp6_usrreq.c+= inet6
OPT.db_access.c+= ddb
OPT.db_autoconf.c+= ddb
OPT.db_break.c+= ddb
OPT.db_command.c+= ddb
OPT.db_cpu.c+= ddb
OPT.db_examine.c+= ddb
OPT.db_expr.c+= ddb
OPT.db_input.c+= ddb
OPT.db_kernel.c+= ddb
OPT.db_lex.c+= ddb
OPT.db_lwp.c+= ddb
OPT.db_output.c+= ddb
OPT.db_panic.c+= ddb
OPT.db_proc.c+= ddb
OPT.db_print.c+= ddb
OPT.db_run.c+= ddb
OPT.db_sym.c+= ddb
OPT.db_trap.c+= ddb
OPT.db_variables.c+= ddb
OPT.db_watch.c+= ddb
OPT.db_write_cmd.c+= ddb
OPT.db_xxx.c+= ddb
OPT.alaw.c+= audio
OPT.linear.c+= audio
OPT.audio.c+= audio
OPT.mulaw.c+= audio
OPT.drm_stub.c+=
OPT.wsdisplay.c+= wsdisplay
OPT.wsdisplay_compat_usl.c+= wsdisplay wsdisplay_compat_usl
OPT.wsdisplay_util.c+= wsdisplay
OPT.wsemulconf.c+= wsdisplay
OPT.wsemul_dumb.c+= wsdisplay
OPT.wsemul_vt100.c+= wsdisplay wsemul_vt100
OPT.wsemul_vt100_subr.c+= wsdisplay wsemul_vt100
OPT.wsemul_vt100_chars.c+= wsdisplay wsemul_vt100
OPT.wsemul_vt100_keys.c+= wsdisplay wsemul_vt100
OPT.wsevent.c+= wsdisplay wskbd wsmouse wsmux
OPT.wsevent_50.c+= compat_50
OPT.wskbd.c+= wskbd
OPT.wskbdutil.c+= wskbd
OPT.wsmouse.c+= wsmouse
OPT.wsmux.c+= wsmux
OPT.wsbellmux.c+= wsmux
OPT.wsfont.c+= wsdisplay
OPT.cd9660_bmap.c+= cd9660
OPT.cd9660_lookup.c+= cd9660
OPT.cd9660_node.c+= cd9660
OPT.cd9660_rrip.c+= cd9660
OPT.cd9660_util.c+= cd9660
OPT.cd9660_vfsops.c+= cd9660
OPT.cd9660_vnops.c+= cd9660
OPT.ptyfs_subr.c+= ptyfs
OPT.ptyfs_vfsops.c+= ptyfs
OPT.ptyfs_vnops.c+= ptyfs
OPT.puffs_compat.c+= compat_50
OPT.tmpfs_fifoops.c+= tmpfs
OPT.tmpfs_mem.c+= tmpfs
OPT.tmpfs_rename.c+= tmpfs
OPT.tmpfs_specops.c+= tmpfs
OPT.tmpfs_subr.c+= tmpfs
OPT.tmpfs_vfsops.c+= tmpfs
OPT.tmpfs_vnops.c+= tmpfs
OPT.kernfs_vfsops.c+= kernfs
OPT.kernfs_vnops.c+= kernfs
OPT.null_vfsops.c+= nullfs
OPT.null_vnops.c+= nullfs
OPT.procfs_auxv.c+= procfs
OPT.procfs_cmdline.c+= procfs
OPT.procfs_fd.c+= procfs
OPT.procfs_fpregs.c+= procfs
OPT.procfs_limit.c+= procfs
OPT.procfs_linux.c+= procfs
OPT.procfs_map.c+= procfs
OPT.procfs_mem.c+= procfs
OPT.procfs_note.c+= procfs
OPT.procfs_regs.c+= procfs
OPT.procfs_status.c+= procfs
OPT.procfs_subr.c+= procfs
OPT.procfs_vfsops.c+= procfs
OPT.procfs_vnops.c+= procfs
OPT.krpc_subr.c+= nfs
OPT.nfs_bio.c+= nfs
OPT.nfs_boot.c+= nfs
OPT.nfs_bootdhcp.c+= nfs nfs_boot_dhcp
OPT.nfs_clntsocket.c+= nfs
OPT.nfs_clntsubs.c+= nfs
OPT.nfs_iod.c+= nfs
OPT.nfs_kq.c+= nfs
OPT.nfs_node.c+= nfs
OPT.nfs_socket.c+= nfs
OPT.nfs_subs.c+= nfs
OPT.nfs_vfsops.c+= nfs
OPT.nfs_vnops.c+= nfs
OPT.ffs_alloc.c+= ffs
OPT.ffs_balloc.c+= ffs
OPT.ffs_extattr.c+= ffs
OPT.ffs_inode.c+= ffs
OPT.ffs_snapshot.c+= ffs
OPT.ffs_subr.c+= ffs
OPT.ffs_tables.c+= ffs
OPT.ffs_vfsops.c+= ffs
OPT.ffs_vnops.c+= ffs
OPT.ffs_wapbl.c+= ffs wapbl
OPT.mfs_vfsops.c+= mfs
OPT.mfs_vnops.c+= mfs
OPT.ufs_acl.c+= ufs ffs mfs
OPT.ufs_bmap.c+= ufs ffs mfs
OPT.ufs_inode.c+= ffs mfs
OPT.ufs_lookup.c+= ufs ffs mfs
OPT.ufs_rename.c+= ffs mfs
OPT.ufs_vfsops.c+= ufs ffs mfs
OPT.ufs_vnops.c+= ufs ffs mfs
OPT.uvm_amap.c+= uvm
OPT.uvm_anon.c+= uvm
OPT.uvm_aobj.c+= uvm
OPT.uvm_bio.c+= uvm
OPT.uvm_coredump.c+= uvm coredump
OPT.uvm_device.c+= uvm
OPT.uvm_fault.c+= uvm
OPT.uvm_glue.c+= uvm
OPT.uvm_init.c+= uvm
OPT.uvm_io.c+= uvm
OPT.uvm_km.c+= uvm
OPT.uvm_loan.c+= uvm
OPT.uvm_map.c+= uvm
OPT.uvm_meter.c+= uvm
OPT.uvm_mmap.c+= uvm
OPT.uvm_mremap.c+= uvm
OPT.uvm_object.c+= uvm
OPT.uvm_page.c+= uvm
OPT.uvm_page_array.c+= uvm
OPT.uvm_page_status.c+= uvm
OPT.uvm_pager.c+= uvm
OPT.uvm_pdaemon.c+= uvm
OPT.uvm_pdpolicy_clock.c+=
OPT.uvm_pgflcache.c+= uvm
OPT.uvm_pglist.c+= uvm
OPT.uvm_physseg.c+= uvm
OPT.uvm_readahead.c+= uvm
OPT.uvm_stat.c+= uvm
OPT.uvm_swap.c+= vmswap
OPT.uvm_unix.c+= uvm
OPT.uvm_user.c+= uvm
OPT.uvm_vnode.c+= uvm
OPT.debugsyms.c+= kern
OPT.param.c+= kern
OPT.bufq_disksort.c+= bufq_disksort
OPT.bufq_fcfs.c+= bufq_fcfs
OPT.compat_stub.c+= kern
OPT.core_elf32.c+= exec_elf32 coredump
OPT.core_netbsd.c+= coredump
OPT.cnmagic.c+= kern
OPT.exec_aout.c+= exec_aout
OPT.exec_elf32.c+= exec_elf32
OPT.exec_script.c+= exec_script
OPT.exec_subr.c+= kern
OPT.init_main.c+= kern
OPT.init_sysctl.c+= kern
OPT.init_sysctl_base.c+= kern
OPT.init_sysent.c+= kern
OPT.kern_acct.c+= kern
OPT.kern_auth.c+= kern
OPT.kern_cfglock.c+= kern
OPT.kern_clock.c+= kern
OPT.kern_condvar.c+= kern
OPT.kern_core.c+= coredump
OPT.kern_cpu.c+= kern
OPT.kern_descrip.c+= kern
OPT.kern_entropy.c+= kern
OPT.kern_event.c+= kern
OPT.kern_exec.c+= kern
OPT.kern_exit.c+= kern
OPT.kern_fork.c+= kern
OPT.kern_idle.c+= kern
OPT.kern_hook.c+= kern
OPT.kern_kthread.c+= kern
OPT.kern_ktrace.c+= ktrace
OPT.kern_ktrace_vfs.c+= ktrace
OPT.kern_ksyms.c+= ksyms ddb modular
OPT.kern_lock.c+= kern
OPT.kern_lwp.c+= kern
OPT.kern_malloc.c+= kern
OPT.kern_module.c+= kern
OPT.kern_module_hook.c+= kern
OPT.kern_module_vfs.c+= kern
OPT.kern_mutex.c+= kern
OPT.kern_mutex_obj.c+= kern
OPT.kern_ntptime.c+= kern
OPT.kern_physio.c+= kern
OPT.kern_pmf.c+= kern
OPT.kern_proc.c+= kern
OPT.kern_prot.c+= kern
OPT.kern_ras.c+= kern
OPT.kern_rate.c+= kern
OPT.kern_reboot.c+= kern
OPT.kern_resource.c+= kern
OPT.kern_runq.c+= kern
OPT.kern_rwlock.c+= kern
OPT.kern_rwlock_obj.c+= kern
OPT.kern_scdebug.c+= kern
OPT.kern_crashme.c+= kern
OPT.kern_sig.c+= kern
OPT.kern_sleepq.c+= kern
OPT.kern_softint.c+= kern
OPT.kern_ssp.c+= kern
OPT.kern_stub.c+= kern
OPT.kern_subr.c+= kern
OPT.kern_synch.c+= kern
OPT.kern_syscall.c+= kern
OPT.kern_sysctl.c+= kern
OPT.kern_tc.c+= kern
OPT.kern_threadpool.c+= kern
OPT.kern_time.c+= kern
OPT.kern_timeout.c+= kern
OPT.kern_turnstile.c+= kern
OPT.kern_todr.c+= kern
OPT.kern_uidinfo.c+= kern
OPT.kern_uuid.c+= kern
OPT.sched_4bsd.c+= sched_4bsd
OPT.subr_acl_posix1e.c+= kern
OPT.subr_acl_nfs4.c+= kern
OPT.subr_autoconf.c+= kern
OPT.subr_blist.c+= vmswap
OPT.subr_bufq.c+= kern
OPT.subr_callback.c+= kern
OPT.subr_cprng.c+= kern
OPT.subr_cpu.c+= kern
OPT.subr_cpufreq.c+= kern
OPT.subr_copy.c+= kern
OPT.subr_device.c+= kern
OPT.subr_devsw.c+= kern
OPT.subr_disk.c+= kern
OPT.subr_disklabel.c+= kern
OPT.subr_disk_open.c+= kern
OPT.subr_emul.c+= kern
OPT.subr_evcnt.c+= kern
OPT.subr_exec_fd.c+= kern
OPT.subr_extent.c+= kern
OPT.subr_hash.c+= kern
OPT.subr_humanize.c+= kern
OPT.subr_interrupt.c+= kern
OPT.subr_iostat.c+= kern
OPT.subr_ipi.c+= kern
OPT.subr_kcpuset.c+= kern
OPT.subr_kmem.c+= kern
OPT.subr_kobj.c+= kern
OPT.subr_kobj_vfs.c+= kern
OPT.subr_localcount.c+= kern
OPT.subr_lockdebug.c+= kern
OPT.subr_log.c+= kern
OPT.subr_lwp_specificdata.c+= kern
OPT.subr_once.c+= kern
OPT.subr_optstr.c+= kern
OPT.subr_pcq.c+= kern
OPT.subr_pcu.c+= kern
OPT.subr_percpu.c+= kern
OPT.subr_pool.c+= kern
OPT.subr_prf.c+= kern
OPT.subr_prof.c+= kern
OPT.subr_pserialize.c+= kern
OPT.subr_psref.c+= kern
OPT.subr_specificdata.c+= kern
OPT.subr_time.c+= kern
OPT.subr_thmap.c+= kern
OPT.subr_userconf.c+= userconf
OPT.subr_vmem.c+= kern
OPT.subr_workqueue.c+= kern
OPT.subr_xcall.c+= kern
OPT.sys_aio.c+= aio
OPT.sys_descrip.c+= kern
OPT.sys_eventfd.c+= kern
OPT.sys_futex.c+= kern
OPT.sys_generic.c+= kern
OPT.sys_getrandom.c+= kern
OPT.sys_module.c+= kern
OPT.sys_mqueue.c+= mqueue
OPT.sys_lwp.c+= kern
OPT.sys_pipe.c+=
OPT.sys_process.c+= ptrace_hooks ktrace
OPT.sys_process_lwpstatus.c+= kern
OPT.sys_ptrace.c+= ptrace
OPT.sys_ptrace_common.c+= ptrace procfs
OPT.sys_pset.c+= kern
OPT.sys_select.c+= kern
OPT.sys_sig.c+= kern
OPT.sys_sched.c+= kern
OPT.sys_socket.c+= kern
OPT.sys_timerfd.c+= kern
OPT.sysv_ipc.c+= sysvshm sysvsem sysvmsg
OPT.sysv_msg.c+= sysvmsg
OPT.sysv_sem.c+= sysvsem
OPT.sysv_shm.c+= sysvshm
OPT.tty.c+= kern
OPT.tty_conf.c+= kern
OPT.tty_bsdpty.c+= compat_bsdpty
OPT.tty_pty.c+= pty
OPT.tty_ptm.c+= pty
OPT.tty_subr.c+= kern
OPT.tty_tty.c+= kern
OPT.uipc_accf.c+= kern
OPT.uipc_domain.c+= kern
OPT.uipc_mbuf.c+= kern
OPT.uipc_mbufdebug.c+= kern ether ddb
OPT.link_proto.c+= kern
OPT.uipc_proto.c+= kern
OPT.uipc_sem.c+= kern
OPT.uipc_socket.c+= kern
OPT.uipc_socket2.c+= kern
OPT.uipc_syscalls.c+= kern
OPT.uipc_usrreq.c+= kern
OPT.vfs_acl.c+= vfs
OPT.vfs_bio.c+= vfs
OPT.vfs_cache.c+= vfs
OPT.vfs_cwd.c+= vfs
OPT.vfs_dirhash.c+= vfs
OPT.vfs_getcwd.c+= vfs
OPT.vfs_hooks.c+= vfs
OPT.vfs_init.c+= vfs
OPT.vfs_lockf.c+= vfs
OPT.vfs_lookup.c+= vfs
OPT.vfs_mount.c+= vfs
OPT.vfs_quotactl.c+= vfs
OPT.vfs_subr.c+= vfs
OPT.vfs_syscalls.c+= vfs
OPT.vfs_trans.c+= vfs
OPT.vfs_vnode.c+= vfs
OPT.vfs_vnops.c+= vfs
OPT.vfs_wapbl.c+= wapbl
OPT.vfs_xattr.c+= vfs
OPT.vnode_if.c+= vfs
OPT.dead_vfsops.c+= vfs
OPT.dead_vnops.c+= vfs
OPT.fifo_vnops.c+= vfs
OPT.genfs_io.c+= vfs
OPT.genfs_rename.c+= vfs
OPT.genfs_vfsops.c+= vfs
OPT.genfs_vnops.c+= vfs
OPT.layer_subr.c+= layerfs
OPT.layer_vfsops.c+= layerfs
OPT.layer_vnops.c+= layerfs
OPT.spec_vnops.c+= vfs
OPT.clock_subr.c+= kern
OPT.clockctl.c+= clockctl
OPT.dksubr.c+= sd cd
OPT.dk.c+= kern
OPT.fss.c+= fss
OPT.md.c+= md
OPT.mm.c+= kern
OPT.random.c+= rnd
OPT.bpf.c+= bpfilter
OPT.bpf_filter.c+= bpf_filter
OPT.bpf_stub.c+= net
OPT.ether_sw_offload.c+= bridge
OPT.if.c+= net
OPT.if_bridge.c+= bridge
OPT.bridgestp.c+= bridge
OPT.if_ethersubr.c+= ether
OPT.if_llatbl.c+= inet inet6
OPT.if_loop.c+= loop
OPT.if_media.c+= net
OPT.if_stats.c+= net
OPT.if_vlan.c+= vlan
OPT.nd.c+= arp inet6
OPT.pfil.c+= net
OPT.pktqueue.c+= net
OPT.net_stats.c+= net
OPT.radix.c+= net
OPT.raw_cb.c+= net
OPT.raw_usrreq.c+= net
OPT.route.c+= net
OPT.rss_config.c+= net
OPT.rtbl.c+= net
OPT.rtsock.c+= net
OPT.toeplitz.c+= net
OPT.if_arp.c+= arp
OPT.in4_cksum.c+= inet
OPT.in_cksum.c+= inet
OPT.ip_encap.c+= inet inet6
OPT.wqinput.c+= inet inet6
OPT.secmodel_suser.c+= secmodel_suser
OPT.secmodel_securelevel.c+= secmodel_securelevel
OPT.secmodel_extensions.c+= secmodel_extensions
OPT.secmodel_extensions_vfs.c+= secmodel_extensions
OPT.secmodel_bsd44.c+= secmodel_bsd44
OPT.if_agr.c+= agr
OPT.if_agrsoftc.c+= agr
OPT.if_agrsubr.c+= agr
OPT.if_agrtimer.c+= agr
OPT.if_agrmonitor.c+= agr
OPT.if_agrether.c+= agr ether
OPT.if_agrether_hash.c+= agr ether
OPT.ieee8023ad_marker.c+= agr ether
OPT.ieee8023ad_lacp.c+= agr ether
OPT.ieee8023ad_lacp_select.c+= agr ether
OPT.ieee8023ad_lacp_timer.c+= agr ether
OPT.ieee8023ad_lacp_sm_rx.c+= agr ether
OPT.ieee8023ad_lacp_sm_mux.c+= agr ether
OPT.ieee8023ad_lacp_sm_ptx.c+= agr ether
OPT.ieee8023ad_lacp_sm_tx.c+= agr ether
OPT.ieee8023ad_lacp_debug.c+= agr ether
OPT.ieee8023_tlv.c+= agr ether
