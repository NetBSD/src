/*	$NetBSD: xxboot.h,v 1.1 1998/09/01 20:02:35 itohy Exp $	*/

/***************************************************************
 *
 *	file: boot.h
 *
 *	author: chapuni(GBA02750@niftyserve.or.jp)
 *
 */

/* xxboot.S */
__dead void BOOT_ERROR __P((const char *msg)) __attribute__((noreturn));
int badbaddr __P((volatile void *adr));
int RAW_READ __P((void *buf, u_int32_t blkpos, size_t bytelen));
#ifdef SCSI_ADHOC_BOOTPART
int RAW_READ0 __P((void *buf, u_int32_t blkpos, size_t bytelen));
#endif
unsigned B_KEYINP __P((void));
void B_CLR_ST __P((unsigned x));
void B_PUTC __P((unsigned c));
void B_PRINT __P((const unsigned char *p));
unsigned B_COLOR __P((unsigned w));
unsigned B_LOCATE __P((int x, int y));
#if 0
unsigned JISSFT __P((unsigned c));
#endif
unsigned short B_SFTSNS __P((void));
void printtitle __P((void));
#if 0
int SYS_STAT __P((int flags));
int getcpu __P((void));
#endif

extern unsigned ID;		/* target SCSI ID */
extern unsigned BOOT_INFO;	/* result of IOCS(__BOOTINF) */

/* check whether the bootinf is SCSI or floppy */
#define BINF_ISFD(pbinf)	(*((char *)(pbinf) + 1) == 0)

extern unsigned FDMODE;		/* Floppy access mode: PDA x 256 + MODE */
extern struct fdfmt{
	struct {
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

/* bootufs.c */
int raw_read_queue __P((void *buf, u_int32_t blkpos, size_t len));
void get_superblk __P((void));
int get_inode __P((ino_t ino, struct dinode *pino));
int read_indirect __P((ufs_daddr_t blkno, int level, void **buf, int count));
void read_blocks __P((struct dinode *dp, void *buf, int count));
ino_t search_file __P((ino_t dirino, const char *filename));
unsigned load_ino __P((void *buf, ino_t ino, const char *filename));
unsigned load_name __P((void *buf, ino_t dirino, const char *filename));
void print_hex __P((unsigned x, int l));
void pickup_list __P((ino_t dirino));
void print_list __P((int n, int active, unsigned boothowto));
volatile void bootufs __P((void));

/* eof */
