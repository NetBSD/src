#       $NetBSD: Makefile,v 1.67 2000/07/25 21:12:46 pk Exp $

#	Makefile for section 9 (kernel function and variable) manual pages.

MAN=	access.9 arp.9 audio.9 bus_dma.9 bus_space.9 callout.9 copy.9 \
	cpu_reboot.9 ctxsw.9 disk.9 disklabel.9 dopowerhooks.9 \
	doshutdownhooks.9 ethersubr.9 evcnt.9 extent.9 fetch.9 fork1.9 \
	humanize_number.9 inittodr.9 intro.9 ioctl.9 kprintf.9 \
	lock.9 log.9 malloc.9 mbuf.9 microtime.9 panic.9 pfil.9 physio.9 \
	pool.9 powerhook_establish.9 psignal.9 ratecheck.9 resettodr.9 \
	rnd.9 rt_timer.9 shutdownhook_establish.9 \
	sleep.9 softintr.9 spl.9 store.9 time.9 todr.9 uiomove.9 ucom.9 \
	usbdi.9 uvm.9 wdc.9
MLINKS+=arp.9 arp_ifinit.9 arp.9 arpresolve.9 arp.9 arpintr.9
MLINKS+=access.9 kernacc.9 access.9 useracc.9
MLINKS+=bus_space.9 bus_space_alloc.9 bus_space.9 bus_space_barrier.9 \
	bus_space.9 bus_space_copy_region_1.9 \
	bus_space.9 bus_space_copy_region_2.9 \
	bus_space.9 bus_space_copy_region_4.9 \
	bus_space.9 bus_space_copy_region_8.9 \
	bus_space.9 bus_space_free.9 bus_space.9 bus_space_map.9 \
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
	bus_dma.9 bus_dmamem_mmap.9
MLINKS+=callout.9 callout_init.9 callout.9 callout_reset.9 \
	callout.9 callout_stop.9 callout.9 callout_active.9 \
	callout.9 callout_pending.9 callout.9 callout_deactivate.9 \
	callout.9 timeout.9 callout.9 untimeout.9
MLINKS+=copy.9 copyin.9 copy.9 copyout.9 copy.9 copystr.9 \
	copy.9 copyinstr.9 copy.9 copyoutstr.9
MLINKS+=ctxsw.9 cpu_switch.9 ctxsw.9 mi_switch.9
MLINKS+=disk.9 disk_init.9 disk.9 disk_attach.9 disk.9 disk_detatch.9 \
	disk.9 disk_busy.9 disk.9 disk_unbusy.9 disk.9 disk_find.9 \
	disk.9 disk_resetstat.9
MLINKS+=disklabel.9 readdisklabel.9 disklabel.9 writedisklabel.9 \
	disklabel.9 setdisklabel.9 disklabel.9 bounds_check_with_label.9
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
MLINKS+=humanize_number.9 format_bytes.9
MLINKS+=kprintf.9 printf.9 kprintf.9 sprintf.9 kprintf.9 vprintf.9 \
	kprintf.9 uprintf.9 kprintf.9 ttyprintf.9
MLINKS+=lock.9 simple_lock_init.9 lock.9 simple_lock.9 \
	lock.9 simple_lock_try.9 lock.9 simple_lock_unlock.9 \
	lock.9 simple_lock_freecheck.9 lock.9 simple_lock_dump.9  \
	lock.9 lockinit.9 lock.9 lockmgr.9 \
	lock.9 lockstatus.9 lock.9 lockmgr_printinfo.9 \
	lock.9 spinlockinit.9 lock.9 spinlockmgr.9 
MLINKS+=malloc.9 free.9
MLINKS+=mbuf.9 m_get.9 mbuf.9 m_getclr.9 mbuf.9 m_gethdr.9 mbuf.9 m_devget.9 \
	mbuf.9 m_copym.9 mbuf.9 m_copypacket.9 mbuf.9 m_copydata.9 \
	mbuf.9 m_copyback.9 mbuf.9 m_cat.9 mbuf.9 m_prepend.9 \
	mbuf.9 m_pullup.9 mbuf.9 m_split.9 mbuf.9 m_adj.9 mbuf.9 m_free.9 \
	mbuf.9 m_freem.9 mbuf.9 mtod.9 mbuf.9 mtocl.9 mbuf.9 cltom.9 \
	mbuf.9 MGET.9 mbuf.9 MGETHDR.9 mbuf.9 MEXTMALLOC.9 \
	mbuf.9 MEXTADD.9 mbuf.9 MCLGET.9 mbuf.9 M_COPY_PKTHDR.9 \
	mbuf.9 M_ALIGN.9 mbuf.9 MH_ALIGN.9 mbuf.9 M_LEADINGSPACE.9 \
	mbuf.9 M_TRAILINGSPACE.9 mbuf.9 M_PREPEND.9 mbuf.9 MCHTYPE.9 \
	mbuf.9 MEXTREMOVE.9 mbuf.9 MFREE.9
MLINKS+=pool.9 pool_create.9 pool.9 pool_destroy.9 pool.9 pool_get.9 \
	pool.9 pool_put.9 pool.9 pool_prime.9 pool.9 pool_sethiwat.9 \
	pool.9 pool_setlowat.9
MLINKS+=rnd.9 rnd_attach_source.9 rnd.9 rnd_detach_source.9 \
	rnd.9 rnd_add_data.9 rnd.9 rnd_add_uint32.9
MLINKS+=rt_timer.9 rt_timer_queue_create.9 rt_timer.9 rt_timer_queue_change.9 \
	rt_timer.9 rt_timer_queue_destroy.9 rt_timer.9 rt_timer_add.9 \
	rt_timer.9 rt_timer_remove_all.9
MLINKS+=pfil.9 pfil_hook_get.9 pfil.9 pfil_add_hook.9 pfil.9 pfil_remove_hook.9
MLINKS+=powerhook_establish.9 powerhook_disestablish.9
MLINKS+=psignal.9 pgsignal.9 psignal.9 gsignal.9
MLINKS+=shutdownhook_establish.9 shutdownhook_disestablish.9
MLINKS+=sleep.9 wakeup.9 sleep.9 tsleep.9
MLINKS+=softintr.9 softintr_establish.9 softintr.9 softintr_disestablish.9 \
	softintr.9 softintr_schedule.9
MLINKS+=spl.9 spl0.9 spl.9 splbio.9 spl.9 splclock.9 spl.9 splhigh.9 \
	spl.9 splimp.9 spl.9 spllowersoftclock.9 spl.9 splnet.9 \
	spl.9 splsched.9 spl.9 splserial.9 \
	spl.9 splsoftclock.9 spl.9 splsoftnet.9 spl.9 splsoftserial.9 \
	spl.9 splstatclock.9 spl.9 spltty.9 spl.9 splx.9
MLINKS+=store.9 subyte.9 store.9 suibyte.9 store.9 susword.9 \
	store.9 suswintr.9 store.9 suword.9 store.9 suiword.9
MLINKS+=time.9 boottime.9 time.9 mono_time.9 time.9 runtime.9
MLINKS+=uvm.9 uvm_init.9 uvm.9 uvm_init_limits.9 uvm.9 uvm_setpagesize.9 \
	uvm.9 uvm_swap_init.9 uvm.9 uvm_map.9 uvm.9 uvm_map_pageable.9 \
	uvm.9 uvm_map_checkprot.9 uvm.9 uvm_map_protect.9 \
	uvm.9 uvm_deallocate.9 uvm.9 uvmspace_alloc.9 uvm.9 uvmspace_exec.9 \
	uvm.9 uvmspace_fork.9 uvm.9 uvmspace_free.9 uvm.9 uvmspace_share.9 \
	uvm.9 uvmspace_unshare.9 uvm.9 uvm_fault.9 uvm.9 uvn_attach.9 \
	uvm.9 uvm_vnp_setsize.9 uvm.9 uvm_vnp_sync.9 uvm.9 uvm_vnp_terminate.9 \
	uvm.9 uvm_io.9 uvm.9 uvm_km_alloc.9 uvm.9 uvm_km_zalloc.9 \
	uvm.9 uvm_km_alloc1.9 uvm.9 uvm_km_kmemalloc.9 uvm.9 uvm_km_valloc.9 \
	uvm.9 uvm_km_valloc_wait.9 uvm.9 uvm_km_suballoc.9 uvm.9 uvm_km_free.9 \
	uvm.9 uvm_km_free_wakeup.9 uvm.9 uvm_pagealloc.9 \
	uvm.9 uvm_pagerealloc.9 uvm.9 uvm_pagefree.9 uvm.9 uvm_pglistalloc.9 \
	uvm.9 uvm_pglistfree.9 uvm.9 uvm_page_physload.9 uvm.9 uvm_pageout.9 \
	uvm.9 uvm_scheduler.9 uvm.9 uvm_swapin.9 uvm.9 uao_create.9 \
	uvm.9 uao_detach.9 uvm.9 uao_reference.9 uvm.9 uvm_chgkprot.9 \
	uvm.9 uvm_kernacc.9 uvm.9 uvm_useracc.9 uvm.9 uvm_vslock.9 \
	uvm.9 uvm_vsunlock.9 uvm.9 uvm_meter.9 uvm.9 uvm_sysctl.9 \
	uvm.9 uvm_fork.9 uvm.9 uvm_grow.9 uvm.9 uvm_coredump.9
MLINKS+=todr.9 todr_gettime.9 todr.9 todr_settime.9 \
	todr.9 todr_getcal.9 todr.9 todr_setcal.9 \
	todr.9 clock_ymdhms_to_secs.9 todr.9 clock_secs_to_ymdhms.9

SUBDIR= man9.i386 man9.sun3

.include <bsd.man.mk>
.include <bsd.subdir.mk>
