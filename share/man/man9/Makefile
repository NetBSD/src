#       $NetBSD: Makefile,v 1.22 1997/05/22 03:46:33 jeremy Exp $

#	Makefile for section 9 (kernel function and variable) manual pages.

MAN=	access.9 arp.9 copy.9 cpu_reboot.9 ctxsw.9 \
	disk.9 disklabel.9 doshutdownhooks.9 \
	extent.9 fetch.9 inittodr.9 intro.9 log.9 malloc.9 panic.9 pfil.9 \
	physio.9 psignal.9 resettodr.9 shutdownhook_establish.9 sleep.9 spl.9 \
	store.9 time.9 timeout.9 vslock.9
MLINKS+=arp.9 arp_ifinit.9 arp.9 arpresolve.9 arp.9 arpintr.9
MLINKS+=access.9 kernacc.9 access.9 useracc.9
MLINKS+=copy.9 copyin.9 copy.9 copyout.9 copy.9 copystr.9 \
	copy.9 copyinstr.9 copy.9 copyoutstr.9
MLINKS+=ctxsw.9 cpu_switch.9 ctxsw.9 mi_switch.9
MLINKS+=disk.9 disk_init.9 disk.9 disk_attach.9 disk.9 disk_detatch.9 \
	disk.9 disk_busy.9 disk.9 disk_unbusy.9 disk.9 disk_find.9 \
	disk.9 disk_resetstat.9
MLINKS+=disklabel.9 readdisklabel.9 disklabel.9 writedisklabel.9 \
	disklabel.9 setdisklabel.9 disklabel.9 bounds_check_with_label.9
MLINKS+=extent.9 extent_create.9 extent.9 extent_destroy.9 \
	extent.9 extent_alloc.9 extent.9 extent_alloc_subregion.9 \
	extent.9 extent_alloc_region.9 extent.9 extent_free.9 \
	extent.9 extent_print.9
MLINKS+=fetch.9 fubyte.9 fetch.9 fuibyte.9 fetch.9 fusword.9 \
	fetch.9 fuswintr.9 fetch.9 fuword.9 fetch.9 fuiword.9
MLINKS+=malloc.9 free.9
MLINKS+=pfil.9 pfil_hook_get.9 pfil.9 pfil_add_hook.9 pfil.9 pfil_remove_hook.9
MLINKS+=psignal.9 pgsignal.9 psignal.9 gsignal.9
MLINKS+=shutdownhook_establish.9 shutdownhook_disestablish.9
MLINKS+=sleep.9 wakeup.9 sleep.9 tsleep.9
MLINKS+=spl.9 spl0.9 spl.9 splbio.9 spl.9 splclock.9 spl.9 splhigh.9 \
	spl.9 splimp.9 spl.9 splnet.9 spl.9 splsched.9 spl.9 splserial.9 \
	spl.9 splsoftclock.9 spl.9 splsoftnet.9 spl.9 splsoftserial.9 \
	spl.9 splstatclock.9 spl.9 spltty.9 spl.9 splx.9
MLINKS+=store.9 subyte.9 store.9 suibyte.9 store.9 susword.9 \
	store.9 suswintr.9 store.9 suword.9 store.9 suiword.9
MLINKS+=time.9 boottime.9 time.9 mono_time.9 time.9 runtime.9
MLINKS+=timeout.9 untimeout.9
MLINKS+=vslock.9 vsunlock.9

SUBDIR=	man9.alpha man9.amiga man9.arm32 man9.atari man9.hp300 man9.i386 \
	man9.mac68k man9.pmax man9.sparc man9.sun3 man9.vax man9.x68k

.include <bsd.prog.mk>
