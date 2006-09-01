/*	$NetBSD: boot.h,v 1.1 2006/09/01 21:26:18 uwe Exp $	*/

#ifndef	LANDISK_STAND_BOOT_H__
#define	LANDISK_STAND_BOOT_H__

#ifndef	PCLOCK
#define	PCLOCK	33333333
#endif

#ifndef	NELEMENTS
#define	NELEMENTS(a)	(sizeof(a) / sizeof(a[0]))
#endif

#ifndef	roundup
#define	roundup(v,s)	((v + ((s) - 1)) & (~((s) - 1)))
#endif

int parsebootfile(const char *fname, char **devname,
    uint *unit, uint *partition, const char **file);
int bios2dev(int biosdev, char **devname, u_int *unit, u_int sector,
    u_int *ptnp);

void halt(void);
void reboot(void);

int bioscons_getc(void);
void bioscons_putc(int c);

int biosdisk_read(int dev, uint32_t blkaddr, void *buf, size_t nblks);

void cache_enable(void);
void cache_disable(void);
void cache_flush(void);

int tick_init(void);
void tick_stop(void);
uint32_t gettick(void);
void delay(int ms);

int db_monitor(void);

void puthex(int val, int size);

#endif	/* LANDISK_STAND_BOOT_H__ */
