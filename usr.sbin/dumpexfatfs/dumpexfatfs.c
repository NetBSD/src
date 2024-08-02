#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#include <sys/types.h>
#include <sys/time.h>

/*
 * Before including exfat headers with function definitions,
 * redefine vnode and buf to the strutures we'll be using
 */
#define vnode uvnode
#define buf ubuf

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_balloc.h>
#include <fs/exfatfs/exfatfs_cksum.h>
#include <fs/exfatfs/exfatfs_conv.h>
#include <fs/exfatfs/exfatfs_dirent.h>
#include <fs/exfatfs/exfatfs_extern.h>

#include "vnode.h"
#include "bufcache.h"

/* What to do when reading a directory */
#define ACTION_PRINT   0
#define ACTION_RECURSE 1
void print_dir(struct exfatfs *fs, uint32_t clust, uint32_t off, int action);

void print_fat(struct exfatfs *fs);
void print_bootblock(struct exfatfs *fs);
void print_bootblock_newfs(struct exfatfs *fs, char *);
void print_bitmap(struct exfatfs *fs);
void print_upcase_table(struct exfatfs *fs, uint32_t clust, uint64_t len);
uint32_t next_fat(struct exfatfs *fs, uint32_t fat);
void dumpexfatfs(int devfd);

extern struct exfatfs *exfatfs_init(int, int);

extern unsigned long dev_bsize;
unsigned long dev_bshift = 9;
int Aflag = 0;
int Bflag = 0;
int Cflag = 0;
int Dflag = 0;
int Fflag = 0;
int Nflag = 0;
int Rflag = 0;
int Uflag = 0;
int verbose = 0;

static uint8_t PRIMARY_IGNORE[2] = { 2, 3 };
static uint8_t PRIMARY_IGNORE_LEN = 2;

#define NEWFS_EXFATFS "newfs_exfatfs"

static void
efun(int eval, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        verr(1, fmt, ap);
        va_end(ap);
}

static void usage(void) {
	fprintf(stderr, "Usage: %s [-abflruv] [-d inum] device\n", getprogname());
	fprintf(stderr, "\t\t-A: equivalent to -abfru\n");
	fprintf(stderr, "\t\t-a: print allocation bitmap\n");
	fprintf(stderr, "\t\t-b: print boot block\n");
	fprintf(stderr, "\t\t-d: list specified directory\n");
	fprintf(stderr, "\t\t-f: print file allocation table\n");
	fprintf(stderr, "\t\t-r: list root directory\n");
	fprintf(stderr, "\t\t-u: print upcase table\n");
	fprintf(stderr, "\t\t-v: increase verbosity\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int devfd, c;
	struct exfatfs *fs;
	char *bootcodefile = NULL;
	char *rdev;

	while ((c = getopt(argc, argv, "AaB:bcd:fnruv")) != -1) {
		switch (c) {
		case 'A':
			Aflag = !Aflag;
			Bflag = !Bflag;
			Fflag = !Fflag;
			Rflag = !Rflag;
			Uflag = !Uflag;
			break;
		case 'a':
			Aflag = !Aflag;
			break;
		case 'B':
			bootcodefile = optarg;
			break;
		case 'b':
			Bflag = !Bflag;
			break;
		case 'c':
			Cflag = !Cflag;
			break;
		case 'd':
			Dflag = strtol(optarg, NULL, 0);
			break;
		case 'f':
			Fflag = !Fflag;
			break;
		case 'n':
			Nflag = !Nflag;
			break;
		case 'r':
			Rflag = !Rflag;
			break;
		case 'u':
			Uflag = !Uflag;
			break;
		case 'v':
			++verbose;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	if (argc < optind)
		usage();
	rdev = argv[optind];

	if (Cflag) {
		struct timespec now;
		uint32_t dostime;
		uint8_t dostimeextra;
		struct timespec converted;
		clock_gettime(CLOCK_REALTIME, &now);
		printf("Unix time: %s\n", ctime(&now.tv_sec));
		exfatfs_unix2dostime(&now, 0, &dostime, &dostimeextra);
		printf("DOS time: 0x%x/0x%hhx\n", dostime, dostimeextra);
		exfatfs_dos2unixtime(dostime, dostimeextra, 0, &converted);
		printf("Reconverted time: %s\n", ctime(&converted.tv_sec));

		/* Now display January 1st, 1980 */
		now.tv_sec = 315561600;
		now.tv_nsec = 0;
		exfatfs_unix2dostime(&now, 0, &dostime, &dostimeextra);
		printf("1980 in DOS time: 0x%x/0x%hhx\n", dostime, dostimeextra);
		
		exit(0);
	}

	if (Aflag + Bflag + Dflag + Fflag + Nflag + Rflag + Uflag == 0)
		Aflag = Bflag = Fflag = Rflag = Uflag = 1;
	
	devfd = open(rdev, O_RDONLY);
	if (devfd <= 0)
		err(1, rdev);
	
	if (bootcodefile != NULL) {
		/*
		 * Extract the bootcode and write it to a file.
		 * exFAT bootcode is 390 bytes starting at offset 120.
		 */
		FILE *fp;
		char buf[390];
		if (pread(devfd, buf, sizeof(buf), 120) < sizeof(buf))
			err(1, rdev);
		fp = fopen(bootcodefile, "wb");
		if (fp == NULL || fwrite(buf, 390, 1, fp) < 1)
			err(1, rdev);
		fclose(fp);
	}
	
	/* Set dev_bshift to match dev_bsize */
	for (dev_bshift = 0; (1 << dev_bshift) < dev_bsize; ++dev_bshift)
		;

	/*
	 * Initialize buffer and vnode cache
	 * and load superblock.
	 */
        esetfunc(efun);
	if (verbose)
		fprintf(stderr, "Initialize exfatfs library...\n");

	fs = exfatfs_init(devfd, verbose);

	if (Nflag) {
		print_bootblock_newfs(fs, rdev);
		printf("\n");
	}

	if (Bflag) {
		print_bootblock(fs);
		printf("\n");
	}

	if (Fflag) {
		print_fat(fs);
		printf("\n");
	}

	if (Rflag) {
		print_dir(fs, ROOTDIRCLUST, ROOTDIRENTRY, ACTION_PRINT);
		printf("\n");
	}

	if (Dflag) {
		print_dir(fs, INO2CLUST(Dflag), INO2ENTRY(Dflag), ACTION_PRINT);
		printf("\n");
	}

	/* Do any required recursion */
	print_dir(fs, ROOTDIRCLUST, ROOTDIRENTRY, ACTION_RECURSE);

	/* All done! */
	close(devfd);
}

void print_fat(struct exfatfs *fs)
{
	struct buf *bp;
	int fatno, i, j;
	int fatsperblk = FATBSIZE(fs) / 4;
	uint32_t *table;

	for (fatno = 0; fatno < fs->xf_NumberOfFats; fatno++) {
		printf("FAT #%d at sector 0x%x:\n", fatno,
		       fs->xf_FatOffset + fatno * fs->xf_FatLength);
		
		for (i = 0; i < fs->xf_ClusterCount + 2; i+= fatsperblk) {
			bread(fs->xf_devvp, EXFATFS_S2D(fs, fs->xf_FatOffset + (i / fatsperblk) + fatno * fs->xf_FatLength),
			      FATBSIZE(fs), 0, &bp);
			
			table = (uint32_t *)bp->b_data;
			for (j = 0; j < FATBSIZE(fs) / 4
				     && i + j < fs->xf_ClusterCount + 2; j++) {
				printf("\t%8.8x: %8.8x\n", (unsigned int)i + j,
				       (unsigned int)table[j]);
			}
			brelse(bp, 0);
		}
	}
}

void print_bootblock(struct exfatfs *fs)
{
	int i;
	
	printf("JumpBoot: %2.2x %2.2x %2.2x\n",
	       fs->xf_JumpBoot[0], fs->xf_JumpBoot[1], fs->xf_JumpBoot[2]);
	printf("FileSystemName: \"%8s\"\n", fs->xf_FileSystemName);
	printf("MustBeZero:");
	for (i = 0; i < sizeof(fs->xf_MustBeZero); i++) {
		if (i > 0 && (i & 0x0f) == 0)
			printf("\n\t   ");
		printf(" %2.2x", fs->xf_MustBeZero[i]);
	}
	printf("\n");
	printf("PartitionOffset: %llu\n",
	       (unsigned long long)fs->xf_PartitionOffset);
	printf("VolumeLength: %llu\n",
	       (unsigned long long)fs->xf_VolumeLength);
	printf("FatOffset: %lu\n", (unsigned long)fs->xf_FatOffset);
	printf("FatLength: %lu\n", (unsigned long)fs->xf_FatLength);
	printf("ClusterHeapOffset: %lu\n",
	       (unsigned long)fs->xf_ClusterHeapOffset);
	printf("ClusterCount: %lu\n", (unsigned long)fs->xf_ClusterCount);
	printf("FirstClusterOfRootDirectory: %lu\n",
	       (unsigned long)fs->xf_FirstClusterOfRootDirectory);
	printf("VolumeSerialNumber: %lu (0x%lx)\n", 
	       (unsigned long)fs->xf_VolumeSerialNumber,
	       (unsigned long)fs->xf_VolumeSerialNumber);
        printf("FileSystemRevision: %hu.%hu\n",
	       EXFATFS_MAJOR(fs), EXFATFS_MINOR(fs));
	printf("VolumeFlags: 0x%4.4hx\n", (unsigned short)fs->xf_VolumeFlags);
	printf("BytesPerSectorShift: %hhu (sector size %u)\n",
	       fs->xf_BytesPerSectorShift, (unsigned)(1 << fs->xf_BytesPerSectorShift));
	printf("SectorsPerClusterShift: %hhu (size %u sectors = %u bytes)\n", 
	       fs->xf_SectorsPerClusterShift,
	       (unsigned)(1 << fs->xf_SectorsPerClusterShift),
	       (unsigned)(1 << (fs->xf_SectorsPerClusterShift + fs->xf_BytesPerSectorShift)));
	printf("NumberOfFats: %u\n", (unsigned)fs->xf_NumberOfFats);
	printf("DriveSelect: 0x%hhx\n", fs->xf_DriveSelect);
        printf("PercentInUse: %hhu\n", fs->xf_PercentInUse);
        printf("Reserved:");
	for (i = 0; i < sizeof(fs->xf_Reserved); i++)
		printf(" %2.2x", fs->xf_Reserved[i]);
	printf("\n");
        printf("BootCode:");
	for (i = 0; i < sizeof(fs->xf_BootCode); i++) {
		if (i > 0 && (i & 0x0f) == 0)
			printf("\n\t ");
		printf(" %2.2x", fs->xf_BootCode[i]);
	}
	printf("\n");
        printf("BootSignature: 0x%hx\n", fs->xf_BootSignature);
}

void print_bootblock_newfs(struct exfatfs *fs, char *rdev)
{
	printf("%s -# 0x%lx -a %lu -c %u -h %lu -S %u -s %lu %s\n",
	       NEWFS_EXFATFS,
	       (unsigned long)fs->xf_VolumeSerialNumber, /* -# */
	       (unsigned long)fs->xf_FatOffset, /* -a */
	       (unsigned)(1 << (fs->xf_SectorsPerClusterShift + fs->xf_BytesPerSectorShift)), /* -c */
	       (unsigned long)fs->xf_ClusterHeapOffset, /* -h */
	       (unsigned)(1 << fs->xf_BytesPerSectorShift), /* -S */
	       (unsigned long)fs->xf_VolumeLength, /* -s */
	       rdev);
}

/*
 * Return the next FAT entry after this one, or -1 if there is none.
 */
uint32_t next_fat(struct exfatfs *fs, uint32_t fat)
{
	struct buf *bp;
	uint32_t retval;

	bread(fs->xf_devvp, EXFATFS_S2D(fs, fs->xf_FatOffset
					    + (fat >> (fs->xf_BytesPerSectorShift - 2))),
	      FATBSIZE(fs), 0, &bp);
	retval = ((uint32_t *)bp->b_data)[fat & (FATBMASK(fs) >> 2)];
	brelse(bp, 0);

	return retval;
}

void print_bitmap(struct exfatfs *fs)
{
	daddr_t lbn;
	uint32_t total;
	uint8_t *data;
	int i;
	struct buf *bp;

	printf("Bitmap beginning at cluster 2:\n");
	total = 0;
	for (lbn = 0; lbn < howmany(fs->xf_ClusterCount,
				    EXFATFS_LSIZE(fs) / sizeof(uint32_t)); ++lbn) {
		/* Retrieve the block */
		bread(fs->xf_bitmapvp, lbn, EXFATFS_LSIZE(fs), 0, &bp);
		printf("--lbn %lu hw 0x%lx\n",
		       (unsigned long)bp->b_lblkno,
		       (unsigned long)bp->b_blkno);
		data = (uint8_t *)bp->b_data;
		
		/* Format it */
		for (i = 0; i < EXFATFS_LSIZE(fs); i++) {
			if ((total & 0x7f) == 0) {
				printf("\t%8.8lx ", (unsigned long)total + 2);
			} else if ((total & 0x3f) == 0) {
				printf(" ");
			}
			printf(" %2.2hhx", data[i]);
			total += 8;
			if (total >= fs->xf_ClusterCount) {
				printf("\n");
				brelse(bp, 0);
				return;
			}
			if ((total & 0x7f) == 0)
				printf("\n");
		}
		brelse(bp, 0);
	}
}

void print_upcase_table(struct exfatfs *fs, uint32_t clust, uint64_t len)
{
	uint32_t total = 0;
	uint16_t *data;
	int i, subclust;
	struct buf *bp;

	printf("Upcase Table (%ld entries):", (long)len / 2);
	do {
		for (subclust = 0; subclust < (1 << fs->xf_SectorsPerClusterShift); ++subclust) {
			/* Retrieve the block */
			bread(fs->xf_devvp, EXFATFS_LC2D(fs, clust) + EXFATFS_S2D(fs, subclust), EXFATFS_LSIZE(fs), 0, &bp);
			data = (uint16_t *)bp->b_data;
			
			/* Format it */
			for (i = 0; i < EXFATFS_LSIZE(fs) >> 1; i++) {
				if ((total & 0x7) == 0) {
					printf("\n\t");
				}
				printf(" %4.4x", data[i]);
				if (++total >= len / 2) {
					printf("\n");
					brelse(bp, 0);
					return;
				}
			}
			brelse(bp, 0);
		}
		clust = next_fat(fs, clust);
	} while (total < len / 2);
}

void print_dir(struct exfatfs *fs, uint32_t dirclust, uint32_t diroff, int action)
{
	uint8_t *data = NULL, *dp, *label;
	int i, j;
	uint64_t inum;
	struct buf *bp = NULL;
	struct exfatfs_dfe *dfp;
	struct exfatfs_dfn *dfnp;
	struct exfatfs_dse *dsp;
	struct timespec ts;
	int nsecreqd=0, nsecfound=0, streamfound=0, namefound=0, namelen=0;
	uint64_t maxlen, data_length;
	uint32_t clust, cksumreqd=0, cksum=0, first_cluster;
	uint16_t filename[EXFATFS_NAMEMAX];
	off_t off;

	if (dirclust == ROOTDIRCLUST) {
		clust = fs->xf_FirstClusterOfRootDirectory;
		maxlen = INT_MAX;
	} else {
		/* Load the directory entry specified */
		daddr_t blkno = EXFATFS_LC2D(fs, dirclust)
			+ EXFATFS_DIRENT2D(fs, diroff);
		struct exfatfs_dse *dse;

		printf("Reading dirent from %d/%d, physical blk 0x%x\n",
		       (int)dirclust, (int)diroff, (unsigned)blkno);
		bread(fs->xf_devvp, blkno, EXFATFS_SSIZE(fs), 0, &bp);
		dse = malloc(sizeof(*dse));
		/* The stream entry is always the second entry in a file set */
		memcpy(dse, ((struct exfatfs_dse *)bp->b_data)
		       + (diroff & (EXFATFS_D2DIRENT(fs, 1) - 1)) + 1,
		       sizeof(*dse));
		brelse(bp, 0);
		bp = NULL;
		clust = dse->xd_firstCluster;
		maxlen = dse->xd_dataLength;
	}
	
	if (action == ACTION_PRINT) {
		printf("Directory entries:\n");
		printf(" Reading from cluster %d (0x%x)\n", clust, clust);
	}
	for (off = 0; off * sizeof(*dfp) < maxlen; ++off) {
		i = off % (EXFATFS_LSIZE(fs) / sizeof(*dfp));
		if (i == 0) {
			if (bp)
				brelse(bp, 0);

			/* Maybe a new cluster, too */
			if (off > 0 && ((off * sizeof(*dfp)) & EXFATFS_CMASK(fs)) == 0) {
				if (bread(fs->xf_devvp, EXFATFS_FATBLK(fs, clust),
					  FATBSIZE(fs), 0, &bp) != 0) {
					err(1, "Read FAT entry 0x%lx\n", (unsigned long)clust);
				}
				clust = ((uint32_t *)bp->b_data)[EXFATFS_FATOFF(clust)];
				brelse(bp, 0);
				bp = NULL;
				if (clust == 0xFFFFFFFF)
					break;
				if (action == ACTION_PRINT)
					printf(" Reading from cluster %d\n", clust);
			}
			
			bread(fs->xf_devvp, EXFATFS_LC2D(fs, clust)
			      + (((off * sizeof(*dfp)) & EXFATFS_CMASK(fs))
				 >> DEV_BSHIFT),
			      EXFATFS_LSIZE(fs), 0, &bp);
			if (action == ACTION_PRINT)
				printf("  lbn %ld at bn 0x%lx\n",
				       (long)(off / (EXFATFS_LSIZE(fs) / sizeof(*dfp))),
				       (unsigned long)bp->b_blkno);
			data = (uint8_t *)bp->b_data;
		}
	
		dp = data + i * 32;

		if ((dp[0] & (XD_ENTRYTYPE_TYPECATEGORY_MASK|XD_ENTRYTYPE_INUSE_MASK)) != (XD_ENTRYTYPE_TYPECATEGORY_MASK|XD_ENTRYTYPE_INUSE_MASK)) {
			nsecfound = 0;
			streamfound = 0;
			namefound = 0;
			first_cluster = 0;
		} else
			++nsecfound;
		
		if (action == ACTION_PRINT)
			printf("Entry %ld: type 0x%2.2hhx\n", (long)off, dp[0]);
		switch (dp[0]) {
		case XD_ENTRYTYPE_EOD:
			if (action == ACTION_PRINT)
				printf("\tEnd of directory\n");
			goto done;
		case XD_ENTRYTYPE_ALLOC_BITMAP:
		case XD_ENTRYTYPE_ALLOC_BITMAP & ~XD_ENTRYTYPE_INUSE_MASK:
			if (action == ACTION_PRINT) {
				printf("\tAllocation Bitmap\n");
				printf("\t\tFlags: %hhx\n",
				       ((struct exfatfs_dirent_allocation_bitmap *)dp)->xd_bitmapFlags);
				printf("\t\tFirst Cluster: 0x%lx\n", (unsigned long)
				       ((struct exfatfs_dirent_allocation_bitmap *)dp)->xd_firstCluster);
				printf("\t\tData Length: %llu\n", (unsigned long long)
				       ((struct exfatfs_dirent_allocation_bitmap *)dp)->xd_dataLength);
			} else {
				if (Aflag) {
					print_bitmap(fs);
				}
			}
			break;
		case XD_ENTRYTYPE_UPCASE_TABLE:
		case XD_ENTRYTYPE_UPCASE_TABLE & ~XD_ENTRYTYPE_INUSE_MASK:
			if (action == ACTION_PRINT) {
				printf("\tUpcase Table\n");
				printf("\t\tChecksum: %lx\n", (unsigned long)
				       ((struct exfatfs_dirent_upcase_table *)dp)->xd_tableChecksum);
				printf("\t\tFirst Cluster: 0x%lx\n", (unsigned long)
				       ((struct exfatfs_dirent_upcase_table *)dp)->xd_firstCluster);
				printf("\t\tData Length: %llu\n", (unsigned long long)
				       ((struct exfatfs_dirent_upcase_table *)dp)->xd_dataLength);
			} else {
				if (Uflag) {
					print_upcase_table(fs, 
							   ((struct exfatfs_dirent_upcase_table *)dp)->xd_firstCluster,
							   ((struct exfatfs_dirent_upcase_table *)dp)->xd_dataLength);
				}
			}
			break;
		case XD_ENTRYTYPE_VOLUME_LABEL:
		case XD_ENTRYTYPE_VOLUME_LABEL & ~XD_ENTRYTYPE_INUSE_MASK:
			if (action == ACTION_PRINT) {
				printf("\tVolume Label\n");
				printf("\t\tLength: %hhu\n",
				       ((struct exfatfs_dirent_volume_label *)dp)->xd_characterCount);
				printf("\t\tVolume Label: ");
				label = ((struct exfatfs_dirent_volume_label *)dp)->xd_volumeLabel;
				for (j = 0; j < ((struct exfatfs_dirent_volume_label *)dp)->xd_characterCount * 2; j++) {
					if (label[j] >= ' ' && label[j] < 0x7f)
						printf("%c", label[j]);
					else if (label[j] != 0x0)
						printf(".");
				}
				printf("\n");
			}
			break;
		case XD_ENTRYTYPE_FILE:
		case (XD_ENTRYTYPE_FILE & ~XD_ENTRYTYPE_INUSE_MASK):
			dfp = (struct exfatfs_dfe *)dp;

			/* Set up to look for secondaries */
			nsecreqd = dfp->xd_secondaryCount;
			nsecfound = 0;
			streamfound = 0;
			namefound = 0;
			cksumreqd = dfp->xd_setChecksum;
			inum = CE2INO(fs, clust, off);
			cksum = exfatfs_cksum16(0, (uint8_t *)dfp, sizeof(*dfp),
						PRIMARY_IGNORE, PRIMARY_IGNORE_LEN);
			
			if (action == ACTION_PRINT) {
				printf("\tFile 0x%llx%s\n",
				       (unsigned long long)inum,
				       ISINUSE(dfp) ? "" : " (not in use)");
				printf("\t\tEntryType: 0x%hhx\n",
				       dfp->xd_entryType);
				printf("\t\tSecondaryCount: %hhu\n",
				       dfp->xd_secondaryCount);
				printf("\t\tSetChecksum: 0x%hx\n",
				       dfp->xd_setChecksum);
				printf("\t\tFileAttributes: 0x%hx (%c%c%c-%c%c)\n",
				       dfp->xd_fileAttributes,
				       (dfp->xd_fileAttributes & 
					XD_FILEATTR_READONLY
					? 'R' : '-'),
				       (dfp->xd_fileAttributes & 
					XD_FILEATTR_HIDDEN
					? 'H' : '-'),
				       (dfp->xd_fileAttributes & 
					XD_FILEATTR_SYSTEM
					? 'S' : '-'),
				       (dfp->xd_fileAttributes & 
					XD_FILEATTR_DIRECTORY
					? 'D' : '-'),
				       (dfp->xd_fileAttributes & 
					XD_FILEATTR_ARCHIVE
					? 'A' : '-'));
				exfatfs_dos2unixtime(dfp->xd_createTimestamp,
						     dfp->xd_create10msIncrement,
						     dfp->xd_createUtcOffset,
						     &ts);
				printf("\t\tCreateTimestamp: 0x%x, %s",
				       (unsigned)dfp->xd_createTimestamp,
				       asctime(localtime(&ts.tv_sec)));
				exfatfs_dos2unixtime(dfp->xd_lastModifiedTimestamp,
						     dfp->xd_lastModified10msIncrement,
						     dfp->xd_lastModifiedUtcOffset,
						     &ts);
				printf("\t\tLastModifiedTimestamp: 0x%x, %s",
				       (unsigned)dfp->xd_lastModifiedTimestamp,
				       asctime(localtime(&ts.tv_sec)));
			
				exfatfs_dos2unixtime(dfp->xd_lastAccessedTimestamp,

						     0,
						     dfp->xd_lastAccessedUtcOffset,
						     &ts);
				printf("\t\tLastAccessedTimestamp: 0x%x, %s",
				       (unsigned)dfp->xd_lastAccessedTimestamp,
				       asctime(localtime(&ts.tv_sec)));

				printf("\t\tCreate10msIncrement: 0x%hhx\n",
				       dfp->xd_create10msIncrement);
				printf("\t\tLastModified10msIncrement: 0x%hhx\n",
				       dfp->xd_lastModified10msIncrement);
				printf("\t\tCreateUtcOffset: 0x%hhx\n",
				       dfp->xd_createUtcOffset);
				printf("\t\tLastModifiedUtcOffset: 0x%hhx\n",
				       dfp->xd_lastModifiedUtcOffset);
				printf("\t\tLastAccessedUtcOffset: 0x%hhx\n",
				       dfp->xd_lastAccessedUtcOffset);
			}
			break;
		case XD_ENTRYTYPE_VOLUME_GUID:
		case XD_ENTRYTYPE_VOLUME_GUID & ~XD_ENTRYTYPE_INUSE_MASK:
			if (action == ACTION_PRINT) {
				printf("\tVolume GUID\n");
			}
			break;
		case XD_ENTRYTYPE_TEXFAT_PADDING:
		case XD_ENTRYTYPE_TEXFAT_PADDING & ~XD_ENTRYTYPE_INUSE_MASK:
			if (action == ACTION_PRINT)
				printf("\tTexFAT padding\n");
			break;
		case XD_ENTRYTYPE_STREAM_EXTENSION:
		case XD_ENTRYTYPE_STREAM_EXTENSION & ~XD_ENTRYTYPE_INUSE_MASK:
			dsp = (struct exfatfs_dse *)dp;
			cksum = exfatfs_cksum16(cksum, (uint8_t *)dsp, sizeof(*dsp),
						NULL, 0);
			first_cluster = dsp->xd_firstCluster;
			data_length = dsp->xd_validDataLength;
			namelen = dsp->xd_nameLength;
			if (action == ACTION_PRINT) {
				printf("\tStream Extension%s\n",
				       ISINUSE(dsp) ? "" : " (not in use)");
				printf("\t\tSecondary Flags: 0x%hhx\n",
				       dsp->xd_generalSecondaryFlags);
				printf("\t\tName Length: %hhu\n",
				       dsp->xd_nameLength);
				printf("\t\tName Hash: 0x%hx\n",
				       dsp->xd_nameHash);
				printf("\t\tValid Data Length: %llu\n",
				       (unsigned long long)dsp->xd_validDataLength);
				printf("\t\tFirst Cluster: 0x%lx\n",
				       (unsigned long)dsp->xd_firstCluster);
				printf("\t\tData Length: %llu\n",
				       (unsigned long long)dsp->xd_dataLength);
			}
			break;
		case XD_ENTRYTYPE_FILE_NAME:
		case XD_ENTRYTYPE_FILE_NAME & ~XD_ENTRYTYPE_INUSE_MASK:
			dfnp = (struct exfatfs_dfn *)dp;
			cksum = exfatfs_cksum16(cksum, (uint8_t *)dfnp, sizeof(*dfnp),
						NULL, 0);
			if (action == ACTION_PRINT) {
				uint8_t tmp[1024];
				memset(tmp, 0, sizeof(tmp));
				memcpy(filename + namefound * 15,
				       dfnp->xd_fileName,
				       sizeof(dfnp->xd_fileName));
				exfatfs_ucs2utf8str(dfnp->xd_fileName, 15, tmp, sizeof(tmp));
				printf("\tFile Name%s: %s\n",
				       ISINUSE(dfnp) ? "" : " (not in use)",
				       tmp);
				++namefound;
				if (nsecfound == nsecreqd) {
					exfatfs_ucs2utf8str(filename, namelen, tmp, sizeof(tmp));
					printf("\t\tComplete File Name: %s\n",
					       tmp);
					if (cksum == cksumreqd)
						printf("\t\tChecksum Match!\n");
					else
						printf("\t\tBAD Checksum: computed %hx expected %hx\n", cksum, cksumreqd);
				}
			} else { /* ACTION_RECURSE */
				if (nsecfound == nsecreqd && streamfound
				    && (dfp->xd_entryType & XD_FILEATTR_DIRECTORY))
					print_dir(fs, first_cluster, data_length, action);
			}
			break;
		case XD_ENTRYTYPE_VENDOR_EXTENSION:
		case XD_ENTRYTYPE_VENDOR_EXTENSION & ~XD_ENTRYTYPE_INUSE_MASK:
			if (action == ACTION_PRINT)
				printf("\tVendor Extension\n");
			break;
		case XD_ENTRYTYPE_VENDOR_ALLOCATION:
		case XD_ENTRYTYPE_VENDOR_ALLOCATION & ~XD_ENTRYTYPE_INUSE_MASK:
			if (action == ACTION_PRINT)
				printf("\tVendor Allocation\n");
			break;
		default:
			printf("\tUnknown type\n");
			break;
		}
	}
done:
	if (bp)
		brelse(bp, 0);
}
