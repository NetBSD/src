#       $NetBSD: Makefile,v 1.6 1996/06/16 23:23:20 pk Exp $

#	Makefile for section 9 (kernel function and variable) manual pages.

MAN=	access.9 boot.9 copy.9 disk.9 doshutdownhooks.9 fetch.9 malloc.9 \
	physio.9 inittodr.9 resettodr.9 shutdownhook_establish.9 store.9 \
	time.9 vslock.9
MLINKS+=shutdownhook_establish.9 shutdownhook_disestablish.9
MLINKS+=time.9 boottime.9 time.9 mono_time.9 time.9 runtime.9
MLINKS+=copy.9 copyin.9 copy.9 copyout.9 copy.9 copystr.9 \
	copy.9 copyinstr.9 copy.9 copyoutstr.9
MLINKS+=disk.9 disk_init.9 disk.9 disk_attach.9 disk.9 disk_detatch.9 \
	disk.9 disk_busy.9 disk.9 disk_unbusy.9 disk.9 disk_find.9 \
	disk.9 disk_resetstat.9
MLINKS+=fetch.9 fubyte.9 fetch.9 fuibyte.9 fetch.9 fusword.9 \
	fetch.9 fuswintr.9 fetch.9 fuword.9 fetch.9 fuiword.9
MLINKS+=store.9 subyte.9 store.9 suibyte.9 store.9 susword.9 \
	store.9 suswintr.9 store.9 suword.9 store.9 suiword.9
MLINKS+=vslock.9 vsunlock.9
MLINKS+=access.9 kernacc.9 access.9 useracc.9
MLINKS+=malloc.9 free.9

.include <bsd.prog.mk>
