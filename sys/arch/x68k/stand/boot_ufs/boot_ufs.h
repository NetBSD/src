/*	$NetBSD: boot_ufs.h,v 1.4.24.1 2009/05/13 17:18:42 jym Exp $	*/

/***************************************************************
 *
 *	file: boot.h
 *
 *	author: chapuni(GBA02750@niftyserve.or.jp)
 *
 */

/* xxboot.S */
__dead void BOOT_ERROR(const char *msg);
int badbaddr(volatile void *adr);
#ifdef SCSI_ADHOC_BOOTPART
void RAW_READ0(void *buf, u_int32_t blkpos, size_t bytelen);
#endif
unsigned B_KEYINP(void);
void B_PUTC(unsigned int c);
void B_PRINT(const char *p);
unsigned B_COLOR(unsigned int w);

extern unsigned ID;		/* target SCSI ID */
extern unsigned BOOT_INFO;	/* result of IOCS(__BOOTINF) */

/* check whether the bootinf is SCSI or floppy */
#define BINF_ISFD(pbinf)	(*((char *)(pbinf) + 1) == 0)

extern unsigned FDMODE;		/* Floppy access mode: PDA x 256 + MODE */
extern struct {
	struct fdfmt{
		unsigned char	N;	/* sector length 0: 128, ..., 3: 1K */
		unsigned char	C;	/* cylinder # */
		unsigned char	H;	/* head # */
		unsigned char	R;	/* sector # */
	} minsec, maxsec;
} FDSECMINMAX;			/* FD format type of the first track */
#ifdef SCSI_ADHOC_BOOTPART
extern u_int32_t SCSI_PARTTOP;	/* top position of boot partition in sector */
extern u_int32_t SCSI_BLKLEN;	/* sector len 0: 256, 1: 512, 2: 1024 */
#endif
