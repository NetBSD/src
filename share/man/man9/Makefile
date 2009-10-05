#       $NetBSD: Makefile,v 1.294 2009/10/05 23:44:10 rmind Exp $

#	Makefile for section 9 (kernel function and variable) manual pages.

MAN=	accept_filter.9 accf_data.9 accf_http.9 \
	altq.9 arc4random.9 arp.9 audio.9 autoconf.9 \
	bcdtobin.9 bcmp.9 bcopy.9 bluetooth.9 boothowto.9 \
	buffercache.9 bufq.9 bus_dma.9 bus_space.9 byteorder.9 bzero.9 \
	callback.9 callout.9 cardbus.9 cnmagic.9 condvar.9 config.9 \
	cons.9 copy.9 coredump_write.9 \
	cpu_configure.9 cpu_coredump.9 cpu_dumpconf.9 \
	cpu_lwp_fork.9 \
	cpu_idle.9 cpu_initclocks.9 cpu_need_resched.9 \
	cpu_number.9 cpu_reboot.9 cpu_rootconf.9 \
	cpu_startup.9 cpu_swapout.9 cpu_switchto.9 \
	csf.9 \
	curproc.9 \
	delay.9 disk.9 ddc.9 disklabel.9 dofileread.9 \
	dopowerhooks.9 do_setresuid.9 doshutdownhooks.9 driver.9 \
	edid.9 errno.9 ethersubr.9 evcnt.9 extattr.9 extent.9 \
	fetch.9 file.9 fileassoc.9 filedesc.9 firmload.9 fork1.9 fsetown.9 \
	fstrans.9 getiobuf.9 \
	hash.9 hashinit.9 hardclock.9 humanize_number.9 hz.9 \
	ieee80211.9 ieee80211_crypto.9 ieee80211_input.9 ieee80211_ioctl.9 \
	ieee80211_node.9 ieee80211_output.9 ieee80211_proto.9 \
	ieee80211_radiotap.9 iic.9 imax.9 \
	in_getifa.9 \
	in4_cksum.9 inittodr.9 intro.9 ioasic.9 ioctl.9 ipkdb.9 isa.9 \
	isapnp.9 itimerfix.9 kauth.9 kcopy.9 kmem.9 \
	kpause.9 \
	kfilter_register.9 knote.9 \
	kprintf.9 kthread.9 linedisc.9 lock.9 log.9 ltsleep.9 \
	LWP_CACHE_CREDS.9 \
	makeiplcookie.9 \
	malloc.9 mb.9 mbuf.9 mca.9 memcmp.9 memcpy.9 memoryallocators.9 \
	memmove.9 memset.9 \
	microtime.9 microuptime.9 mi_switch.9 \
	mstohz.9 mutex.9 m_tag.9 namecache.9 \
	namei.9 opencrypto.9 optstr.9 \
	panic.9 pci.9 pci_configure_bus.9 pci_intr.9 pckbport.9 \
	pcmcia.9 pfil.9 physio.9 pmap.9 pmatch.9 pmc.9 pmf.9 \
	pool.9 pool_cache.9 powerhook_establish.9 ppsratecheck.9 \
	preempt.9 putter.9 \
	radio.9 ras.9 rasops.9 ratecheck.9 resettodr.9 rnd.9 rssadapt.9 \
	rt_timer.9 rwlock.9 RUN_ONCE.9 \
	sched_4bsd.9 scsipi.9 \
	secmodel.9 secmodel_bsd44.9 secmodel_overlay.9 secmodel_securelevel.9 \
	secmodel_suser.9 setjmp.9 shutdownhook_establish.9 \
	signal.9 sockopt.9 softintr.9 spl.9 splraiseipl.9 \
	store.9 suspendsched.9 \
	sysctl.9 sysmon_envsys.9 tc.9 tcp_congctl.9 timecounter.9 \
	time_second.9 todr.9 tvtohz.9 uiomove.9 ucom.9 userret.9 \
	vattr.9 veriexec.9 vcons.9 vfs.9 vfs_hooks.9 vfsops.9 vfssubr.9 \
	video.9 vme.9 \
	vnfileops.9 vnode.9 vnodeops.9 vnsubr.9 \
	ubc.9 usbdi.9 uvm.9 vmem.9 \
	wdc.9 workqueue.9 \
	wscons.9 wsdisplay.9 wsfont.9 wskbd.9 wsmouse.9

MAN+=	boothowto.9
MLINKS+=boothowto.9 BOOT_FLAG.9

MAN+=	dmover.9
MLINKS+=dmover.9 dmover_backend_register.9 \
	dmover.9 dmover_backend_unregister.9 \
	dmover.9 dmover_session_create.9 \
	dmover.9 dmover_session_destroy.9 \
	dmover.9 dmover_request_alloc.9 \
	dmover.9 dmover_request_free.9 \
	dmover.9 dmover_process.9 \
	dmover.9 dmover_done.9

MAN+=	KASSERT.9
MLINKS+=KASSERT.9 KDASSERT.9

MLINKS+=accept_filter.9 accept_filt_add.9
MLINKS+=accept_filter.9 accept_filt_del.9
MLINKS+=accept_filter.9 accept_filt_generic_mod_event.9
MLINKS+=accept_filter.9 accept_filt_get.9

MLINKS+=arp.9 arp_ifinit.9 arp.9 arpresolve.9 arp.9 arpintr.9
MLINKS+=autoconf.9 config_search_loc.9 \
	autoconf.9 config_search_ia.9 \
	autoconf.9 config_found_sm_loc.9 \
	autoconf.9 config_found_ia.9 \
	autoconf.9 config_found.9 \
	autoconf.9 config_match.9 \
	autoconf.9 config_attach_loc.9 \
	autoconf.9 config_attach.9 \
	autoconf.9 config_attach_pseudo.9 \
	autoconf.9 config_detach.9 \
	autoconf.9 config_deactivate.9 \
	autoconf.9 config_defer.9 \
	autoconf.9 config_interrupts.9 \
	autoconf.9 config_pending_incr.9 \
	autoconf.9 config_pending_decr.9 \
	autoconf.9 config_finalize_register.9
MLINKS+=bcdtobin.9 bintobcd.9
MLINKS+=buffercache.9 bread.9 \
	buffercache.9 breada.9 \
	buffercache.9 breadn.9 \
	buffercache.9 bwrite.9 \
	buffercache.9 bawrite.9 \
	buffercache.9 bdwrite.9 \
	buffercache.9 getblk.9 \
	buffercache.9 geteblk.9 \
	buffercache.9 incore.9 \
	buffercache.9 allocbuf.9 \
	buffercache.9 brelse.9 \
	buffercache.9 biodone.9 \
	buffercache.9 biowait.9
MLINKS+=bus_space.9 bus_space_alloc.9 bus_space.9 bus_space_barrier.9 \
	bus_space.9 bus_space_copy_region_1.9 \
	bus_space.9 bus_space_copy_region_2.9 \
	bus_space.9 bus_space_copy_region_4.9 \
	bus_space.9 bus_space_copy_region_8.9 \
	bus_space.9 bus_space_free.9 bus_space.9 bus_space_map.9 \
	bus_space.9 bus_space_mmap.9 \
	bus_space.9 bus_space_read_1.9 bus_space.9 bus_space_read_2.9 \
	bus_space.9 bus_space_read_4.9 bus_space.9 bus_space_read_8.9 \
	bus_space.9 bus_space_read_multi_1.9 \
	bus_space.9 bus_space_read_multi_2.9 \
	bus_space.9 bus_space_read_multi_4.9 \
	bus_space.9 bus_space_read_multi_8.9 \
	bus_space.9 bus_space_read_region_1.9 \
	bus_space.9 bus_space_read_region_2.9 \
	bus_space.9 bus_space_read_region_4.9 \
	bus_space.9 bus_space_read_region_8.9 \
	bus_space.9 bus_space_set_region_1.9 \
	bus_space.9 bus_space_set_region_2.9 \
	bus_space.9 bus_space_set_region_4.9 \
	bus_space.9 bus_space_set_region_8.9 \
	bus_space.9 bus_space_subregion.9 bus_space.9 bus_space_unmap.9 \
	bus_space.9 bus_space_vaddr.9 \
	bus_space.9 bus_space_write_1.9 bus_space.9 bus_space_write_2.9 \
	bus_space.9 bus_space_write_4.9 bus_space.9 bus_space_write_8.9 \
	bus_space.9 bus_space_write_multi_1.9 \
	bus_space.9 bus_space_write_multi_2.9 \
	bus_space.9 bus_space_write_multi_4.9 \
	bus_space.9 bus_space_write_multi_8.9 \
	bus_space.9 bus_space_write_region_1.9 \
	bus_space.9 bus_space_write_region_2.9 \
	bus_space.9 bus_space_write_region_4.9 \
	bus_space.9 bus_space_write_region_8.9
MLINKS+=bus_dma.9 bus_dmamap_create.9 bus_dma.9 bus_dmamap_destroy.9 \
	bus_dma.9 bus_dmamap_load.9 bus_dma.9 bus_dmamap_load_mbuf.9 \
	bus_dma.9 bus_dmamap_load_uio.9 bus_dma.9 bus_dmamap_load_raw.9 \
	bus_dma.9 bus_dmamap_unload.9 bus_dma.9 bus_dmamap_sync.9 \
	bus_dma.9 bus_dmamem_alloc.9 bus_dma.9 bus_dmamem_free.9 \
	bus_dma.9 bus_dmamem_map.9 bus_dma.9 bus_dmamem_unmap.9 \
	bus_dma.9 bus_dmamem_mmap.9 \
	bus_dma.9 bus_dmatag_subregion.9 bus_dma.9 bus_dmatag_destroy.9
MLINKS+=byteorder.9 bswap16.9 byteorder.9 bswap32.9 byteorder.9 bswap64.9 \
	byteorder.9 be16toh.9 byteorder.9 be32toh.9 byteorder.9 be64toh.9 \
	byteorder.9 htobe16.9 byteorder.9 htobe32.9 byteorder.9 htobe64.9 \
	byteorder.9 htole16.9 byteorder.9 htole32.9 byteorder.9 htole64.9 \
	byteorder.9 le16toh.9 byteorder.9 le32toh.9 byteorder.9 le64toh.9 \
	byteorder.9 be16dec.9 byteorder.9 be32dec.9 byteorder.9 be64dec.9 \
	byteorder.9 le16dec.9 byteorder.9 le32dec.9 byteorder.9 le64dec.9 \
	byteorder.9 be16enc.9 byteorder.9 be32enc.9 byteorder.9 be64enc.9 \
	byteorder.9 le16enc.9 byteorder.9 le32enc.9 byteorder.9 le64enc.9
MLINKS+=callback.9 callback_head_init.9 callback.9 callback_head_destroy.9 \
        callback.9 callback_register.9 callback.9 callback_unregister.9 \
        callback.9 callback_run_roundrobin.9
MLINKS+=callout.9 callout_init.9 callout.9 callout_destroy.9 \
	callout.9 callout_reset.9 callout.9 callout_schedule.9 \
	callout.9 callout_setfunc.9 callout.9 callout_stop.9 \
	callout.9 callout_pending.9 callout.9 callout_expired.9 \
	callout.9 callout_invoking.9 callout.9 callout_ack.9 \
	callout.9 timeout.9 callout.9 untimeout.9 \
	callout.9 callout_halt.9
MLINKS+=cardbus.9 cardbus_attach_card.9 \
	cardbus.9 cardbus_detach_card.9 \
	cardbus.9 cardbus_function_enable.9 \
	cardbus.9 cardbus_function_disable.9 \
	cardbus.9 cardbus_mapreg_map.9 \
	cardbus.9 cardbus_mapreg_unmap.9 \
	cardbus.9 cardbus_get_capability.9 \
	cardbus.9 cardbus_make_tag.9 \
	cardbus.9 cardbus_free_tag.9 \
	cardbus.9 cardbus_conf_read.9 \
	cardbus.9 cardbus_conf_write.9 \
	cardbus.9 cardbus_intr_establish.9 \
	cardbus.9 cardbus_intr_disestablish.9 \
	cardbus.9 CARDBUS_VENDOR.9 \
	cardbus.9 CARDBUS_PRODUCT.9 \
	cardbus.9 Cardbus_function_enable.9 \
	cardbus.9 Cardbus_function_disable.9 \
	cardbus.9 Cardbus_mapreg_map.9 \
	cardbus.9 Cardbus_mapreg_unmap.9 \
	cardbus.9 Cardbus_make_tag.9 \
	cardbus.9 Cardbus_free_tag.9 \
	cardbus.9 Cardbus_conf_read.9 \
	cardbus.9 Cardbus_conf_write.9
MLINKS+=cnmagic.9 cn_trap.9 \
	cnmagic.9 cn_isconsole.9 \
	cnmagic.9 cn_check_magic.9 \
	cnmagic.9 cn_init_magic.9 \
	cnmagic.9 cn_set_magic.9 \
	cnmagic.9 cn_get_magic.9 \
	cnmagic.9 cn_destroy_magic.9
MLINKS+=condvar.9 cv.9 condvar.9 cv_init.9 condvar.9 cv_destroy.9 \
	condvar.9 cv_wait.9 condvar.9 cv_wait_sig.9 \
	condvar.9 cv_timedwait.9 condvar.9 cv_timedwait_sig.9 \
	condvar.9 cv_signal.9 condvar.9 cv_broadcast.9 \
	condvar.9 cv_wakeup.9 condvar.9 cv_has_waiters.9
MLINKS+=cons.9 cnbell.9 cons.9 cngetc.9 cons.9 cngetsn.9 cons.9 cnpollc.9 \
	cons.9 cnputc.9
MLINKS+=copy.9 copyin.9 copy.9 copyout.9 copy.9 copystr.9 \
	copy.9 copyinstr.9 copy.9 copyoutstr.9 \
	copy.9 copyin_proc.9 copy.9 copyout_proc.9 \
	copy.9 ioctl_copyin.9 copy.9 ioctl_copyout.9
MLINKS+=cpu_dumpconf.9 cpu_dump.9 cpu_dumpconf.9 cpu_dumpsize.9 \
	cpu_dumpconf.9 dumpsys.9
MLINKS+=cpu_lwp_fork.9 child_return.9 cpu_lwp_fork.9 proc_trampoline.9
MLINKS+=cpu_swapout.9 cpu_swapin.9
MLINKS+=curproc.9 curcpu.9
MLINKS+=delay.9 DELAY.9
MLINKS+=disk.9 disk_attach.9 disk.9 disk_detach.9 \
	disk.9 disk_busy.9 disk.9 disk_unbusy.9 disk.9 disk_find.9 \
	disk.9 disk_init.9 disk.9 disk_destroy.9 disk.9 disk_begindetach.9
MLINKS+=disklabel.9 readdisklabel.9 disklabel.9 writedisklabel.9 \
	disklabel.9 setdisklabel.9 disklabel.9 bounds_check_with_label.9
MLINKS+=dofileread.9 dofilewrite.9 \
	dofileread.9 dofilereadv.9 \
	dofileread.9 dofilewritev.9
MLINKS+=do_setresuid.9 do_setresgid.9
MLINKS+=ethersubr.9 ether_ifattach.9 ethersubr.9 ether_addmulti.9 \
	ethersubr.9 ether_delmulti.9 ethersubr.9 ETHER_IS_MULTICAST.9 \
	ethersubr.9 ETHER_FIRST_MULTI.9 ethersubr.9 ETHER_NEXT_MULTI.9 \
	ethersubr.9 fddi_ifattach.9 \
	ethersubr.9 fddi_addmulti.9 ethersubr.9 fddi_delmulti.9
MLINKS+=evcnt.9 evcnt_attach_dynamic.9 evcnt.9 evcnt_attach_static.9 \
	evcnt.9 evcnt_detach.9
MLINKS+=extent.9 extent_create.9 extent.9 extent_destroy.9 \
	extent.9 extent_alloc.9 extent.9 extent_alloc_subregion.9 \
	extent.9 extent_alloc_region.9 extent.9 extent_free.9 \
	extent.9 extent_print.9
MLINKS+=fetch.9 fubyte.9 fetch.9 fuibyte.9 fetch.9 fusword.9 \
	fetch.9 fuswintr.9 fetch.9 fuword.9 fetch.9 fuiword.9
MLINKS+=file.9 closef.9 \
	file.9 ffree.9 \
	file.9 FILE_IS_USABLE.9 \
	file.9 FILE_USE.9 \
	file.9 FILE_UNUSE.9 \
	file.9 FILE_SET_MATURE.9
MLINKS+=filedesc.9 falloc.9 \
	filedesc.9 fd_getfile.9 \
	filedesc.9 dupfdopen.9 \
	filedesc.9 fdalloc.9 \
	filedesc.9 fdcheckstd.9 \
	filedesc.9 fdclear.9 \
	filedesc.9 fdclone.9 \
	filedesc.9 fdcloseexec.9 \
	filedesc.9 fdcopy.9 \
	filedesc.9 fdexpand.9 \
	filedesc.9 fdfree.9 \
	filedesc.9 fdinit.9 \
	filedesc.9 fdrelease.9 \
	filedesc.9 fdremove.9 \
	filedesc.9 fdshare.9 \
	filedesc.9 fdunshare.9
MLINKS+=firmload.9 firmware_open.9 \
	firmload.9 firmware_close.9 \
	firmload.9 firmware_get_size.9 \
	firmload.9 firmware_read.9 \
	firmload.9 firmware_malloc.9 \
	firmload.9 firmware_free.9
MLINKS+=fsetown.9 fgetown.9 \
	fsetown.9 fownsignal.9
MLINKS+=fstrans.9 fstrans_done.9 \
	fstrans.9 fstrans_is_owner.9 \
	fstrans.9 fstrans_getstate.9 \
	fstrans.9 fstrans_setstate.9 \
	fstrans.9 fstrans_start.9 \
	fstrans.9 fstrans_start_nowait.9 \
	fstrans.9 fscow_establish.9 \
	fstrans.9 fscow_disestablish.9 \
	fstrans.9 fscow_run.9
MLINKS+=getiobuf.9 putiobuf.9
MLINKS+=hash.9 hash32_buf.9 hash.9 hash32_str.9 hash.9 hash32_strn.9
MLINKS+=hashinit.9 hashdone.9
MLINKS+=humanize_number.9 format_bytes.9
MLINKS+=ieee80211.9 ieee80211_chan2ieee.9 \
	ieee80211.9 ieee80211_chan2mode.9 \
	ieee80211.9 ieee80211_ieee2mhz.9 \
	ieee80211.9 ieee80211_ifattach.9 \
	ieee80211.9 ieee80211_ifdetach.9 \
	ieee80211.9 ieee80211_media2rate.9 \
	ieee80211.9 ieee80211_media_change.9 \
	ieee80211.9 ieee80211_media_init.9 \
	ieee80211.9 ieee80211_media_status.9 \
	ieee80211.9 ieee80211_mhz2ieee.9 \
	ieee80211.9 ieee80211_rate2media.9 \
	ieee80211.9 ieee80211_setmode.9 \
	ieee80211.9 ieee80211_watchdog.9
MLINKS+=iic.9 iic_acquire_bus.9 \
	iic.9 iic_release_bus.9 \
	iic.9 iic_exec.9 \
	iic.9 iic_smbus_write_byte.9 \
	iic.9 iic_smbus_read_byte.9 \
	iic.9 iic_smbus_receive_byte.9
MLINKS+=imax.9 imin.9
MLINKS+=in4_cksum.9 in_cksum.9 \
	in4_cksum.9 in6_cksum.9
MLINKS+=ioasic.9 ioasic_intr_establish.9 \
	ioasic.9 ioasic_intr_disestablish.9 \
	ioasic.9 ioasic_intr_evcnt.9 \
	ioasic.9 ioasic_attach_devs.9 \
	ioasic.9 ioasic_submatch.9
MLINKS+=ipkdb.9 ipkdb_init.9 ipkdb.9 ipkdb_connect.9 ipkdb.9 ipkdbinit.9 \
	ipkdb.9 ipkdbcmds.9 ipkdb.9 ipkdb_trap.9 ipkdb.9 ipkdb_poll.9 \
	ipkdb.9 ipkdbif_init.9 ipkdb.9 ipkdbfbyte.9 ipkdb.9 ipkdbsbyte.9
MLINKS+=isa.9 isa_intr_alloc.9 \
	isa.9 isa_intr_establish.9 \
	isa.9 isa_intr_disestablish.9 \
	isa.9 isa_intr_evcnt.9 \
	isa.9 isa_dmamap_create.9 \
	isa.9 isa_dmamap_destroy.9 \
	isa.9 isa_dmamem_alloc.9 \
	isa.9 isa_dmamem_free.9 \
	isa.9 isa_dmamem_map.9 \
	isa.9 isa_dmamem_unmap.9 \
	isa.9 isa_malloc.9 \
	isa.9 isa_free.9 \
	isa.9 isa_dmastart.9 \
	isa.9 isa_dmaabort.9 \
	isa.9 isa_dmacount.9 \
	isa.9 isa_dmadone.9 \
	isa.9 isa_dmamaxsize.9 \
	isa.9 isa_drq_isfree.9 \
	isa.9 isa_dmacascade.9 \
	isa.9 isa_mappage.9
MLINKS+=isapnp.9 isapnp_devmatch.9 \
	isapnp.9 isapnp_config.9 \
	isapnp.9 isapnp_unconfig.9
MLINKS+=knote.9 KNOTE.9 \
	kfilter_register.9 kfilter_unregister.9
MLINKS+=kmem.9 kmem_alloc.9 kmem.9 kmem_free.9 kmem.9 kmem_zalloc.9
MAN+=	kpreempt.9
MLINKS+=kpreempt.9 kpreempt_disable.9 \
	kpreempt.9 kpreempt_disabled.9 \
	kpreempt.9 kpreempt_enable.9
MLINKS+=kprintf.9 printf.9 kprintf.9 printf_nolog.9 kprintf.9 sprintf.9 \
	kprintf.9 snprintf.9 kprintf.9 vprintf.9 kprintf.9 vsprintf.9 \
	kprintf.9 vsnprintf.9 kprintf.9 uprintf.9 kprintf.9 ttyprintf.9 \
	kprintf.9 tprintf.9 kprintf.9 tprintf_open.9 kprintf.9 tprintf_close.9 \
	kprintf.9 aprint_normal.9 kprintf.9 aprint_naive.9 \
	kprintf.9 aprint_verbose.9 kprintf.9 aprint_debug.9 \
	kprintf.9 aprint_error.9 \
	kprintf.9 aprint_normal_dev.9 kprintf.9 aprint_naive_dev.9 \
	kprintf.9 aprint_verbose_dev.9 kprintf.9 aprint_debug_dev.9 \
	kprintf.9 aprint_error_dev.9 \
	kprintf.9 aprint_normal_ifnet.9 kprintf.9 aprint_naive_ifnet.9 \
	kprintf.9 aprint_verbose_ifnet.9 kprintf.9 aprint_debug_ifnet.9 \
	kprintf.9 aprint_error_ifnet.9 kprintf.9 aprint_get_error_count.9
MLINKS+=kthread.9 kthread_exit.9 \
	kthread.9 kthread_create.9 \
	kthread.9 kthread_destroy.9
MLINKS+=linedisc.9 ttyldisc_add.9 \
	linedisc.9 ttyldisc_lookup.9 \
	linedisc.9 ttyldisc_remove.9
MLINKS+=lock.9 simple_lock_init.9 lock.9 simple_lock.9 \
	lock.9 simple_lock_try.9 lock.9 simple_unlock.9 \
	lock.9 simple_lock_freecheck.9 lock.9 simple_lock_dump.9  \
	lock.9 lockinit.9 lock.9 lockmgr.9 \
	lock.9 lockstatus.9 lock.9 lockmgr_printinfo.9 lock.9 transferlockers.9 \
	lock.9 spinlockinit.9 lock.9 spinlockmgr.9
MLINKS+=ltsleep.9 wakeup.9 ltsleep.9 tsleep.9
MLINKS+=malloc.9 MALLOC.9 malloc.9 realloc.9 malloc.9 free.9 malloc.9 FREE.9
MLINKS+=malloc.9 malloc_roundup.9 malloc.9 malloc_type_attach.9
MLINKS+=malloc.9 malloc_type_detach.9 malloc.9 malloc_type_setlimit.9
MLINKS+=malloc.9 MALLOC_DEFINE_LIMIT.9 malloc.9 MALLOC_DEFINE.9
MLINKS+=malloc.9 MALLOC_DECLARE.9
MLINKS+=mb.9 mb_memory.9 mb.9 mb_read.9 mb.9 mb_write.9
MLINKS+=mbuf.9 m_get.9 mbuf.9 m_getclr.9 mbuf.9 m_gethdr.9 mbuf.9 m_devget.9 \
	mbuf.9 m_copym.9 mbuf.9 m_copypacket.9 mbuf.9 m_copydata.9 \
	mbuf.9 m_copyback.9 mbuf.9 m_cat.9 mbuf.9 m_dup.9 mbuf.9 m_prepend.9 \
	mbuf.9 m_copyback_cow.9 \
	mbuf.9 m_makewritable.9 \
	mbuf.9 m_pulldown.9 \
	mbuf.9 m_pullup.9 mbuf.9 m_split.9 mbuf.9 m_adj.9 mbuf.9 m_free.9 \
	mbuf.9 m_freem.9 mbuf.9 mtod.9 mbuf.9 mtocl.9 mbuf.9 cltom.9 \
	mbuf.9 MGET.9 mbuf.9 MGETHDR.9 mbuf.9 MEXTMALLOC.9 \
	mbuf.9 MEXTADD.9 mbuf.9 MCLGET.9 mbuf.9 M_COPY_PKTHDR.9 \
	mbuf.9 M_MOVE_PKTHDR.9 \
	mbuf.9 M_ALIGN.9 mbuf.9 MH_ALIGN.9 mbuf.9 M_LEADINGSPACE.9 \
	mbuf.9 M_TRAILINGSPACE.9 mbuf.9 M_PREPEND.9 mbuf.9 MCHTYPE.9 \
	mbuf.9 MFREE.9
MLINKS+=mca.9 mca_intr_establish.9 \
	mca.9 mca_intr_disestablish.9 \
	mca.9 mca_intr_evcnt.9 \
	mca.9 mca_conf_read.9 \
	mca.9 mca_conf_write.9
MLINKS+=microtime.9 bintime.9 \
	microtime.9 getbintime.9 \
	microtime.9 getmicrotime.9 \
	microtime.9 nanotime.9 \
	microtime.9 getnanotime.9
MLINKS+=microuptime.9 binuptime.9 \
	microuptime.9 getbinuptime.9 \
	microuptime.9 getmicrouptime.9 \
	microuptime.9 nanouptime.9 \
	microuptime.9 getnanouptime.9
MLINKS+=mutex.9 mutex_init.9 mutex.9 mutex_destroy.9 mutex.9 mutex_enter.9 \
	mutex.9 mutex_exit.9 mutex.9 mutex_tryenter.9 mutex.9 mutex_owned.9 \
	mutex.9 mutex_spin_enter.9 mutex.9 mutex_spin_exit.9
MLINKS+=m_tag.9 m_tag_copy.9 \
	m_tag.9 m_tag_copy_chain.9 \
	m_tag.9 m_tag_delete.9 \
	m_tag.9 m_tag_delete_chain.9 \
	m_tag.9 m_tag_delete_nonpersistent.9 \
	m_tag.9 m_tag_find.9 \
	m_tag.9 m_tag_first.9 \
	m_tag.9 m_tag_free.9 \
	m_tag.9 m_tag_get.9 \
	m_tag.9 m_tag_init.9 \
	m_tag.9 m_tag_next.9 \
	m_tag.9 m_tag_prepend.9 \
	m_tag.9 m_tag_unlink.9
MLINKS+=namecache.9 cache_lookup.9 \
	namecache.9 cache_revlookup.9 \
	namecache.9 cache_enter.9 \
	namecache.9 cache_purge.9 \
	namecache.9 cache_purgevfs.9 \
	namecache.9 namecache_print.9
MLINKS+=namei.9 lookup.9 \
	namei.9 relookup.9 \
	namei.9 NDINIT.9
MLINKS+=opencrypto.9 crypto_get_driverid.9 \
	opencrypto.9 crypto_register.9 \
	opencrypto.9 crypto_kregister.9 \
	opencrypto.9 crypto_unregister.9 \
	opencrypto.9 crypto_done.9 \
	opencrypto.9 crypto_kdone.9 \
	opencrypto.9 crypto_newsession.9 \
	opencrypto.9 crypto_freesession.9 \
	opencrypto.9 crypto_dispatch.9 \
	opencrypto.9 crypto_kdispatch.9 \
	opencrypto.9 crypto_getreq.9 \
	opencrypto.9 crypto_freereq.9
MLINKS+=optstr.9 optstr_get.9
MLINKS+=pci.9 pci_conf_read.9 \
	pci.9 pci_conf_write.9 \
	pci.9 pci_conf_print.9 \
	pci.9 pci_find_device.9 \
	pci.9 pci_get_capability.9 \
	pci.9 pci_mapreg_type.9 \
	pci.9 pci_mapreg_map.9 \
	pci.9 pci_mapreg_info.9 \
	pci.9 pci_intr_map.9 \
	pci.9 pci_intr_string.9 \
	pci.9 pci_intr_evcnt.9 \
	pci.9 pci_intr_establish.9 \
	pci.9 pci_intr_disestablish.9 \
	pci.9 pci_get_powerstate.9 \
	pci.9 pci_set_powerstate.9 \
	pci.9 pci_vpd_read.9 \
	pci.9 pci_vpd_write.9 \
	pci.9 pci_make_tag.9 \
	pci.9 pci_decompose_tag.9 \
	pci.9 pci_findvendor.9 \
	pci.9 pci_devinfo.9 \
	pci.9 PCI_VENDOR.9 \
	pci.9 PCI_PRODUCT.9 \
	pci.9 PCI_REVISION.9
MLINKS+=pci_configure_bus.9 pci_conf_hook.9 \
	pci_configure_bus.9 pci_conf_interrupt.9
MLINKS+=pckbport.9 pckbport_attach.9 \
	pckbport.9 pckbport_attach_slot.9 \
	pckbport.9 pckbport_cnattach.9 \
	pckbport.9 pckbportintr.9 \
	pckbport.9 pckbport_set_inputhandler.9 \
	pckbport.9 pckbport_flush.9 \
	pckbport.9 pckbport_poll_cmd.9 \
	pckbport.9 pckbport_enqueue_cmd.9 \
	pckbport.9 pckbport_poll_data.9 \
	pckbport.9 pckbport_set_poll.9 \
	pckbport.9 pckbport_xt_translation.9 \
	pckbport.9 pckbport_slot_enable.9
MLINKS+=pcmcia.9 pcmcia_function_init.9 \
	pcmcia.9 pcmcia_function_enable.9 \
	pcmcia.9 pcmcia_function_disable.9 \
	pcmcia.9 pcmcia_io_alloc.9 \
	pcmcia.9 pcmcia_io_free.9 \
	pcmcia.9 pcmcia_io_map.9 \
	pcmcia.9 pcmcia_io_unmap.9 \
	pcmcia.9 pcmcia_mem_alloc.9 \
	pcmcia.9 pcmcia_mem_free.9 \
	pcmcia.9 pcmcia_mem_map.9 \
	pcmcia.9 pcmcia_mem_unmap.9 \
	pcmcia.9 pcmcia_intr_establish.9 \
	pcmcia.9 pcmcia_intr_disestablish.9 \
	pcmcia.9 pcmcia_cis_read_1.9 \
	pcmcia.9 pcmcia_cis_read_2.9 \
	pcmcia.9 pcmcia_cis_read_3.9 \
	pcmcia.9 pcmcia_cis_read_4.9 \
	pcmcia.9 pcmcia_cis_read_n.9 \
	pcmcia.9 pcmcia_scan_cis.9
MLINKS+=pfil.9 pfil_hook_get.9 \
	pfil.9 pfil_add_hook.9 \
	pfil.9 pfil_remove_hook.9 \
	pfil.9 pfil_head_register.9 \
	pfil.9 pfil_head_unregister.9 \
	pfil.9 pfil_head_get.9 \
	pfil.9 pfil_run_hooks.9
MLINKS+=pmc.9 pmc_get_num_counters.9 \
	pmc.9 pmc_get_counter_type.9 \
	pmc.9 pmc_save_context.9 \
	pmc.9 pmc_restore_context.9 \
	pmc.9 pmc_enable_counter.9 \
	pmc.9 pmc_disable_counter.9 \
	pmc.9 pmc_counter_isrunning.9 \
	pmc.9 pmc_counter_isconfigured.9 \
	pmc.9 pmc_configure_counter.9 \
	pmc.9 pmc_get_counter_value.9 \
	pmc.9 pmc_accumulate.9 \
	pmc.9 pmc_alloc_kernel_counter.9 \
	pmc.9 pmc_free_kernel_counter.9 \
	pmc.9 pmc_start_profiling.9 \
	pmc.9 pmc_stop_profiling.9 \
	pmc.9 PMC_ENABLED.9
MLINKS+=pmf.9 pmf_device_register.9 pmf.9 pmf_device_register1.9 \
	pmf.9 pmf_device_deregister.9 \
	pmf.9 pmf_device_suspend.9 pmf.9 pmf_device_resume.9 \
	pmf.9 pmf_device_recursive_suspend.9 \
	pmf.9 pmf_device_recursive_resume.9 \
	pmf.9 pmf_device_resume_subtree.9 \
	pmf.9 pmf_class_network_register.9 pmf.9 pmf_class_input_register.9 \
	pmf.9 pmf_class_display_register.9 \
	pmf.9 pmf_system_suspend.9 pmf.9 pmf_system_resume.9 \
	pmf.9 pmf_system_shutdown.9 \
	pmf.9 pmf_event_register.9 pmf.9 pmf_event_deregister.9 \
	pmf.9 pmf_event_inject.9 \
	pmf.9 pmf_set_platform.9 pmf.9 pmf_get_platform.9
MLINKS+=pool.9 pool_create.9 pool.9 pool_destroy.9 pool.9 pool_get.9 \
	pool.9 pool_init.9 pool.9 pool_put.9 pool.9 pool_prime.9 \
	pool.9 pool_sethiwat.9 pool.9 pool_setlowat.9
MLINKS+=pool_cache.9 pool_cache_init.9 \
	pool_cache.9 pool_cache_destroy.9 \
	pool_cache.9 pool_cache_get_paddr.9 \
	pool_cache.9 pool_cache_get.9 \
	pool_cache.9 pool_cache_put_paddr.9 \
	pool_cache.9 pool_cache_put.9 \
	pool_cache.9 pool_cache_destruct_object.9 \
	pool_cache.9 pool_cache_invalidate.9
MLINKS+=powerhook_establish.9 powerhook_disestablish.9
MLINKS+=preempt.9 yield.9
MLINKS+=ras.9 ras_lookup.9 \
	ras.9 ras_fork.9 \
	ras.9 ras_purgeall.9
MLINKS+=rasops.9 rasops_init.9 \
	rasops.9 rasops_reconfig.9
MLINKS+=rnd.9 rnd_attach_source.9 rnd.9 rnd_detach_source.9 \
	rnd.9 rnd_add_data.9 rnd.9 rnd_add_uint32.9
MLINKS+=rssadapt.9 ieee80211_rssadapt_choose.9 \
	rssadapt.9 ieee80211_rssadapt_input.9 \
	rssadapt.9 ieee80211_rssadapt_lower_rate.9 \
	rssadapt.9 ieee80211_rssadapt_raise_rate.9 \
	rssadapt.9 ieee80211_rssadapt_updatestats.9
MLINKS+=rt_timer.9 rt_timer_queue_create.9 rt_timer.9 rt_timer_queue_change.9 \
	rt_timer.9 rt_timer_queue_destroy.9 rt_timer.9 rt_timer_add.9 \
	rt_timer.9 rt_timer_remove_all.9
MLINKS+=rwlock.9 rw.9 rwlock.9 rw_init.9 rwlock.9 rw_destroy.9 \
	rwlock.9 rw_enter.9 rwlock.9 rw_exit.9 \
	rwlock.9 rw_tryenter.9 rwlock.9 rw_tryupgrade.9 \
	rwlock.9 rw_downgrade.9 rwlock.9 rw_read_held.9 \
	rwlock.9 rw_write_held.9 rwlock.9 rw_lock_held.9
MAN+=	select.9
MLINKS+=select.9 selinit.9 \
	select.9 seldestroy.9 \
	select.9 selrecord.9 \
	select.9 selnotify.9
MLINKS+=setjmp.9 longjmp.9
MLINKS+=shutdownhook_establish.9 shutdownhook_disestablish.9
MLINKS+=signal.9 siginit.9 \
	signal.9 sigactsinit.9 \
	signal.9 sigactsunshare.9 \
	signal.9 sigactsfree.9 \
	signal.9 execsigs.9 \
	signal.9 sigaction1.9 \
	signal.9 sigprocmask1.9 \
	signal.9 sigpending1.9 \
	signal.9 sigsuspend1.9 \
	signal.9 sigaltstack1.9 \
	signal.9 gsignal.9 \
	signal.9 pgsignal.9 \
	signal.9 trapsignal.9 \
	signal.9 psignal.9 \
	signal.9 sched_psignal.9 \
	signal.9 issignal.9 \
	signal.9 postsig.9 \
	signal.9 killproc.9 \
	signal.9 sigexit.9 \
	signal.9 sigmasked.9 \
	signal.9 sendsig.9 \
	signal.9 sigcode.9 \
	signal.9 sigtramp.9
MLINKS+=sockopt.9 sockopt_init.9 \
	sockopt.9 sockopt_destroy.9 \
	sockopt.9 sockopt_get.9 \
	sockopt.9 sockopt_getint.9 \
	sockopt.9 sockopt_set.9 \
	sockopt.9 sockopt_setint.9
MLINKS+=softintr.9 softintr_establish.9 softintr.9 softintr_disestablish.9 \
	softintr.9 softintr_schedule.9 softintr.9 softint.9 \
	softintr.9 softint_establish.9 softintr.9 softint_disestablish.9 \
	softintr.9 softint_schedule.9
MLINKS+=spl.9 spl0.9 spl.9 splbio.9 spl.9 splclock.9 spl.9 splhigh.9 \
	spl.9 splimp.9 spl.9 spllowersoftclock.9 spl.9 splnet.9 \
	spl.9 splsched.9 spl.9 splserial.9 \
	spl.9 splsoftclock.9 spl.9 splsoftnet.9 spl.9 splsoftserial.9 \
	spl.9 splstatclock.9 spl.9 spltty.9 spl.9 splvm.9 spl.9 splx.9 \
	spl.9 splsoftbio.9
MLINKS+=store.9 subyte.9 store.9 suibyte.9 store.9 susword.9 \
	store.9 suswintr.9 store.9 suword.9 store.9 suiword.9
MLINKS+=sysctl.9 old_sysctl.9 \
	sysctl.9 sysctl_create.9 \
	sysctl.9 sysctl_createv.9 \
	sysctl.9 sysctl_destroy.9 \
	sysctl.9 sysctl_destroyv.9 \
	sysctl.9 sysctl_dispatch.9 \
	sysctl.9 sysctl_free.9 \
	sysctl.9 sysctl_init.9 \
	sysctl.9 sysctl_locate.9 \
	sysctl.9 sysctl_lock.9 \
	sysctl.9 sysctl_lookup.9 \
	sysctl.9 sysctl_needfunc.9 \
	sysctl.9 sysctl_notavail.9 \
	sysctl.9 sysctl_null.9 \
	sysctl.9 sysctl_query.9 \
	sysctl.9 sysctl_teardown.9 \
	sysctl.9 sysctl_unlock.9
MLINKS+=sysmon_envsys.9 sysmon_envsys_create.9 \
	sysmon_envsys.9 sysmon_envsys_destroy.9 \
	sysmon_envsys.9 sysmon_envsys_register.9 \
	sysmon_envsys.9 sysmon_envsys_unregister.9 \
	sysmon_envsys.9 sysmon_envsys_sensor_attach.9 \
	sysmon_envsys.9 sysmon_envsys_sensor_detach.9
MLINKS+=time_second.9 boottime.9 time_second.9 time_uptime.9
MLINKS+=timecounter.9 tc_init.9
MLINKS+=tc.9 tc_intr_establish.9 \
	tc.9 tc_intr_disestablish.9 \
	tc.9 tc_intr_evcnt.9 \
	tc.9 tc_mb.9 \
	tc.9 tc_wmb.9 \
	tc.9 tc_syncbus.9 \
	tc.9 tc_badaddr.9 \
	tc.9 TC_DENSE_TO_SPARSE.9 \
	tc.9 TC_PHYS_TO_UNCACHED.9
MLINKS+=todr.9 todr_gettime.9 todr.9 todr_settime.9 \
	todr.9 clock_ymdhms_to_secs.9 todr.9 clock_secs_to_ymdhms.9
MLINKS+=ubc.9 ubc_alloc.9 ubc.9 ubc_release.9 ubc.9 ubc_uiomove.9
MLINKS+=uvm.9 uvm_init.9 uvm.9 uvm_init_limits.9 uvm.9 uvm_setpagesize.9 \
	uvm.9 uvm_swap_init.9 uvm.9 uvm_map.9 uvm.9 uvm_unmap.9 \
	uvm.9 uvm_map_pageable.9 \
	uvm.9 uvm_map_checkprot.9 uvm.9 uvm_map_protect.9 \
	uvm.9 uvm_deallocate.9 uvm.9 uvmspace_alloc.9 uvm.9 uvmspace_exec.9 \
	uvm.9 uvmspace_fork.9 uvm.9 uvmspace_free.9 uvm.9 uvmspace_share.9 \
	uvm.9 uvmspace_unshare.9 uvm.9 uvm_fault.9 \
	uvm.9 uvm_vnp_setsize.9 \
	uvm.9 uvm_io.9 uvm.9 uvm_km_alloc.9 \
	uvm.9 uvm_km_suballoc.9 uvm.9 uvm_km_free.9 \
	uvm.9 uvm_pagealloc.9 \
	uvm.9 uvm_pagerealloc.9 uvm.9 uvm_pagefree.9 uvm.9 uvm_pglistalloc.9 \
	uvm.9 uvm_pglistfree.9 uvm.9 uvm_page_physload.9 uvm.9 uvm_pageout.9 \
	uvm.9 uvm_scheduler.9 uvm.9 uvm_swapin.9 uvm.9 uao_create.9 \
	uvm.9 uao_detach.9 uvm.9 uao_reference.9 uvm.9 uvm_chgkprot.9 \
	uvm.9 uvm_kernacc.9 uvm.9 uvm_vslock.9 \
	uvm.9 uvm_vsunlock.9 uvm.9 uvm_meter.9 \
	uvm.9 uvm_fork.9 uvm.9 uvm_grow.9 \
	uvm.9 uvn_findpages.9 \
	uvm.9 uvm_loan.9 uvm.9 uvm_unloan.9
MLINKS+=vme.9 vme_probe.9 \
	vme.9 vme_space_map.9 \
	vme.9 vme_space_unmap.9 \
	vme.9 vme_intr_map.9 \
	vme.9 vme_intr_establish.9 \
	vme.9 vme_intr_disestablish.9 \
	vme.9 vme_intr_evcnt.9 \
	vme.9 vme_dmamap_create.9 \
	vme.9 vme_dmamap_destroy.9 \
	vme.9 vme_dmamem_alloc.9 \
	vme.9 vme_dmamem_free.9 \
	vme.9 vme_space_alloc.9 \
	vme.9 vme_space_free.9 \
	vme.9 vme_space_get.9
MLINKS+=vattr.9 vattr_null.9 \
	vattr.9 VATTR_NULL.9
MLINKS+=vfs_hooks.9 vfs_hooks_unmount.9
MLINKS+=vfsops.9 VFS_MOUNT.9 \
	vfsops.9 VFS_START.9 \
	vfsops.9 VFS_UNMOUNT.9 \
	vfsops.9 VFS_ROOT.9 \
	vfsops.9 VFS_QUOTACTL.9 \
	vfsops.9 VFS_STATVFS.9 \
	vfsops.9 VFS_SYNC.9 \
	vfsops.9 VFS_VGET.9 \
	vfsops.9 VFS_FHTOVP.9 \
	vfsops.9 VFS_VPTOFH.9 \
	vfsops.9 VFS_CHECKEXP.9 \
	vfsops.9 VFS_SNAPSHOT.9 \
	vfsops.9 VFS_SUSPENDCTL.9
MLINKS+=vfssubr.9 vfs_getnewfsid.9 \
	vfssubr.9 vfs_getvfs.9 \
	vfssubr.9 vfs_export.9 \
	vfssubr.9 vfs_showexport.9 \
	vfssubr.9 vfs_export_lookup.9 \
	vfssubr.9 vfs_setpublicfs.9 \
	vfssubr.9 vfs_mountedon.9 \
	vfssubr.9 vfs_mountroot.9 \
	vfssubr.9 vfs_unmountall.9 \
	vfssubr.9 vfs_busy.9 \
	vfssubr.9 vfs_unbusy.9 \
	vfssubr.9 vfs_mountalloc.9 \
	vfssubr.9 vfs_rootmountalloc.9 \
	vfssubr.9 vfs_shutdown.9 \
	vfssubr.9 vfs_attach.9 \
	vfssubr.9 vfs_detach.9 \
	vfssubr.9 vfs_reinit.9 \
	vfssubr.9 vfs_getopsbyname.9 \
	vfssubr.9 vfs_resume.9 \
	vfssubr.9 vfs_suspend.9
MLINKS+=vmem.9 vmem_alloc.9 \
	vmem.9 vmem_create.9 \
	vmem.9 vmem_destroy.9 \
	vmem.9 vmem_free.9 \
	vmem.9 vmem_xalloc.9 \
	vmem.9 vmem_xfree.9
MLINKS+=vnode.9 vcount.9 \
	vnode.9 vref.9 \
	vnode.9 VREF.9 \
	vnode.9 vrele.9 \
	vnode.9 vget.9 \
	vnode.9 vput.9 \
	vnode.9 vhold.9 \
	vnode.9 VHOLD.9 \
	vnode.9 holdrele.9 \
	vnode.9 HOLDRELE.9 \
	vnode.9 getnewvnode.9 \
	vnode.9 ungetnewvnode.9 \
	vnode.9 vrecycle.9 \
	vnode.9 vgone.9 \
	vnode.9 vgonel.9 \
	vnode.9 vflush.9 \
	vnode.9 vaccess.9 \
	vnode.9 checkalias.9 \
	vnode.9 bdevvp.9 \
	vnode.9 cdevvp.9 \
	vnode.9 vfinddev.9 \
	vnode.9 vdevgone.9 \
	vnode.9 vwakeup.9 \
	vnode.9 vflushbuf.9 \
	vnode.9 vinvalbuf.9 \
	vnode.9 vtruncbuf.9 \
	vnode.9 vprint.9
MLINKS+=vnodeops.9 VOP_LOOKUP.9 \
	vnodeops.9 VOP_CREATE.9 \
	vnodeops.9 VOP_MKNOD.9 \
	vnodeops.9 VOP_OPEN.9 \
	vnodeops.9 VOP_CLOSE.9 \
	vnodeops.9 VOP_ACCESS.9 \
	vnodeops.9 VOP_GETATTR.9 \
	vnodeops.9 VOP_SETATTR.9 \
	vnodeops.9 VOP_READ.9 \
	vnodeops.9 VOP_WRITE.9 \
	vnodeops.9 VOP_IOCTL.9 \
	vnodeops.9 VOP_FCNTL.9 \
	vnodeops.9 VOP_POLL.9 \
	vnodeops.9 VOP_REVOKE.9 \
	vnodeops.9 VOP_MMAP.9 \
	vnodeops.9 VOP_FSYNC.9 \
	vnodeops.9 VOP_SEEK.9 \
	vnodeops.9 VOP_REMOVE.9 \
	vnodeops.9 VOP_LINK.9 \
	vnodeops.9 VOP_RENAME.9 \
	vnodeops.9 VOP_MKDIR.9 \
	vnodeops.9 VOP_RMDIR.9 \
	vnodeops.9 VOP_SYMLINK.9 \
	vnodeops.9 VOP_READDIR.9 \
	vnodeops.9 VOP_READLINK.9 \
	vnodeops.9 VOP_ABORTOP.9 \
	vnodeops.9 VOP_INACTIVE.9 \
	vnodeops.9 VOP_RECLAIM.9 \
	vnodeops.9 VOP_LOCK.9 \
	vnodeops.9 VOP_UNLOCK.9 \
	vnodeops.9 VOP_ISLOCKED.9 \
	vnodeops.9 VOP_BMAP.9 \
	vnodeops.9 VOP_PRINT.9 \
	vnodeops.9 VOP_PATHCONF.9 \
	vnodeops.9 VOP_ADVLOCK.9 \
	vnodeops.9 VOP_LEASE.9 \
	vnodeops.9 VOP_WHITEOUT.9 \
	vnodeops.9 VOP_GETPAGES.9 \
	vnodeops.9 VOP_PUTPAGES.9 \
	vnodeops.9 VOP_STRATEGY.9 \
	vnodeops.9 VOP_BWRITE.9 \
	vnodeops.9 VOP_GETEXTATTR.9 \
	vnodeops.9 VOP_SETEXTATTR.9 \
	vnodeops.9 VOP_LISTEXTATTR.9
MLINKS+=vnfileops.9 vn_closefile.9 \
	vnfileops.9 vn_fcntl.9 \
	vnfileops.9 vn_ioctl.9 \
	vnfileops.9 vn_read.9 \
	vnfileops.9 vn_poll.9 \
	vnfileops.9 vn_statfile.9 \
	vnfileops.9 vn_write.9
MLINKS+=vnsubr.9 vn_bwrite.9 \
	vnsubr.9 vn_close.9 \
	vnsubr.9 vn_default_error.9 \
	vnsubr.9 vn_isunder.9 \
	vnsubr.9 vn_lock.9 \
	vnsubr.9 vn_markexec.9 \
	vnsubr.9 vn_marktext.9 \
	vnsubr.9 vn_restorerecurse.9 \
	vnsubr.9 vn_rdwr.9 \
	vnsubr.9 vn_setrecurse.9 \
	vnsubr.9 vn_stat.9 \
	vnsubr.9 vn_open.9 \
	vnsubr.9 vn_writechk.9
MLINKS+=workqueue.9 workqueue_create.9 \
	workqueue.9 workqueue_enqueue.9 \
	workqueue.9 workqueue_destroy.9
MLINKS+=wsdisplay.9 wsdisplay_switchtoconsole.9 \
	wsdisplay.9 wsdisplay_cnattach.9 \
	wsdisplay.9 wsdisplaydevprint.9 \
	wsdisplay.9 wsemuldisplaydevprint.9
MLINKS+=wsfont.9 wsfont_init.9 \
	wsfont.9 wsfont_matches.9 \
	wsfont.9 wsfont_find.9 \
	wsfont.9 wsfont_add.9 \
	wsfont.9 wsfont_remove.9 \
	wsfont.9 wsfont_enum.9 \
	wsfont.9 wsfont_lock.9 \
	wsfont.9 wsfont_unlock.9 \
	wsfont.9 wsfont_getflg.9 \
	wsfont.9 wsfont_map_unichar.9
MLINKS+=wskbd.9 wskbd_input.9 \
	wskbd.9 wskbd_rawinput.9 \
	wskbd.9 wskbd_cnattach.9 \
	wskbd.9 wskbd_cndetach.9 \
	wskbd.9 wskbddevprint.9
MLINKS+=wsmouse.9 wsmouse_input.9 \
	wsmouse.9 wsmousedevprint.9

SUBDIR= man9.i386 man9.sun3

.include <bsd.man.mk>
.include <bsd.subdir.mk>
