/*
 *	$Id: rdb.h,v 1.5 1994/02/11 07:02:02 chopps Exp $
 */

#ifndef _amiga_rdb_h
#define _amiga_rdb_h

#define ULONG	u_long
#define LONG	long
#define UBYTE	u_char

/* definitions for the Amiga RigidDiskBlock layout, which always starts in 
   cylinder 0 of a medium. Taken from page 254f of the RKM: Devices */

struct RigidDiskBlock {
    ULONG   rdb_ID;		/* 4 character identifier */
    ULONG   rdb_SummedLongs;	/* size of this checksummed structure */
    LONG    rdb_ChkSum;		/* block checksum (longword sum to zero) */
    ULONG   rdb_HostID;		/* SCSI Target ID of host */
    ULONG   rdb_BlockBytes;	/* size of disk blocks */
    ULONG   rdb_Flags;		/* see below for defines */
    /* block list heads */
    ULONG   rdb_BadBlockList;	/* optional bad block list */
    ULONG   rdb_PartitionList;	/* optional first partition block */
    ULONG   rdb_FileSysHeaderList; /* optional file system header block */
    ULONG   rdb_DriveInit;	/* optional drive-specific init code */
				/* DriveInit(lun,rdb,ior): "C" stk & d0/a0/a1 */
    ULONG   rdb_Reserved1[6];	/* set to $ffffffff */
    /* physical drive characteristics */
    ULONG   rdb_Cylinders;	/* number of drive cylinders */
    ULONG   rdb_Sectors;	/* sectors per track */
    ULONG   rdb_Heads;		/* number of drive heads */
    ULONG   rdb_Interleave;	/* interleave */
    ULONG   rdb_Park;		/* landing zone cylinder */
    ULONG   rdb_Reserved2[3];
    ULONG   rdb_WritePreComp;	/* starting cylinder: write precompensation */
    ULONG   rdb_ReducedWrite;	/* starting cylinder: reduced write current */
    ULONG   rdb_StepRate;	/* drive step rate */
    ULONG   rdb_Reserved3[5];
    /* logical drive characteristics */
    ULONG   rdb_RDBBlocksLo;	/* low block of range reserved for hardblocks */
    ULONG   rdb_RDBBlocksHi;	/* high block of range for these hardblocks */
    ULONG   rdb_LoCylinder;	/* low cylinder of partitionable disk area */
    ULONG   rdb_HiCylinder;	/* high cylinder of partitionable data area */
    ULONG   rdb_CylBlocks;	/* number of blocks available per cylinder */
    ULONG   rdb_AutoParkSeconds; /* zero for no auto park */
    ULONG   rdb_HighRDSKBlock;	/* highest block used by RDSK */
				/* (not including replacement bad blocks) */
    ULONG   rdb_Reserved4;
    /* drive identification */
    char    rdb_DiskVendor[8];
    char    rdb_DiskProduct[16];
    char    rdb_DiskRevision[4];
    char    rdb_ControllerVendor[8];
    char    rdb_ControllerProduct[16];
    char    rdb_ControllerRevision[4];
    ULONG   rdb_Reserved5[10];
};

#define	IDNAME_RIGIDDISK	0x5244534B	/* 'RDSK' */

#define	RDB_LOCATION_LIMIT	16

#define	RDBFB_LAST	0	/* no disks exist to be configured after */
#define	RDBFF_LAST	0x01L	/*   this one on this controller */
#define	RDBFB_LASTLUN	1	/* no LUNs exist to be configured greater */
#define	RDBFF_LASTLUN	0x02L	/*   than this one at this SCSI Target ID */
#define	RDBFB_LASTTID	2	/* no Target IDs exist to be configured */
#define	RDBFF_LASTTID	0x04L	/*   greater than this one on this SCSI bus */
#define	RDBFB_NORESELECT 3	/* don't bother trying to perform reselection */
#define	RDBFF_NORESELECT 0x08L	/*   when talking to this drive */
#define	RDBFB_DISKID	4	/* rdb_Disk... identification valid */
#define	RDBFF_DISKID	0x10L
#define	RDBFB_CTRLRID	5	/* rdb_Controller... identification valid */
#define	RDBFF_CTRLRID	0x20L
				/* added 7/20/89 by commodore: */
#define RDBFB_SYNCH	6	/* drive supports scsi synchronous mode */
#define RDBFF_SYNCH	0x40L	/* CAN BE DANGEROUS TO USE IF IT DOESN'T! */

struct PartitionBlock {
    ULONG   pb_ID;		/* 4 character identifier */
    ULONG   pb_SummedLongs;	/* size of this checksummed structure */
    LONG    pb_ChkSum;		/* block checksum (longword sum to zero) */
    ULONG   pb_HostID;		/* SCSI Target ID of host */
    ULONG   pb_Next;		/* block number of the next PartitionBlock */
    ULONG   pb_Flags;		/* see below for defines */
    ULONG   pb_Reserved1[2];
    ULONG   pb_DevFlags;	/* preferred flags for OpenDevice */
    UBYTE   pb_DriveName[32];	/* preferred DOS device name: BSTR form */
				/* (not used if this name is in use) */
    ULONG   pb_Reserved2[15];	/* filler to 32 longwords */
    ULONG   pb_Environment[17];	/* environment vector for this partition */
    ULONG   pb_EReserved[15];	/* reserved for future environment vector */
};

#define	IDNAME_PARTITION	0x50415254	/* 'PART' */

#define	PBFB_BOOTABLE	0	/* this partition is intended to be bootable */
#define	PBFF_BOOTABLE	1L	/*   (expected directories and files exist) */
#define	PBFB_NOMOUNT	1	/* do not mount this partition (e.g. manually */
#define	PBFF_NOMOUNT	2L	/*   mounted, but space reserved here) */

/* this is from <dos/filehandler.h>

#define DE_TABLESIZE	0	/* minimum value is 11 (includes NumBuffers) */
#define DE_SIZEBLOCK	1	/* in longwords: standard value is 128 */
#define DE_SECORG	2	/* not used; must be 0 */
#define DE_NUMHEADS	3	/* # of heads (surfaces). drive specific */
#define DE_SECSPERBLK	4	/* not used; must be 1 */
#define DE_BLKSPERTRACK 5	/* blocks per track. drive specific */
#define DE_RESERVEDBLKS 6	/* unavailable blocks at start.	 usually 2 */
#define DE_PREFAC	7	/* not used; must be 0 */
#define DE_INTERLEAVE	8	/* usually 0 */
#define DE_LOWCYL	9	/* starting cylinder. typically 0 */
#define DE_UPPERCYL	10	/* max cylinder.  drive specific */
#define DE_NUMBUFFERS	11	/* starting # of buffers.  typically 5 */
#define DE_MEMBUFTYPE	12	/* type of mem to allocate for buffers. */
#define DE_BUFMEMTYPE	12	/* same as above, better name
				 * 1 is public, 3 is chip, 5 is fast */
#define DE_MAXTRANSFER	13	/* Max number bytes to transfer at a time */
#define DE_MASK		14	/* Address Mask to block out certain memory */
#define DE_BOOTPRI	15	/* Boot priority for autoboot */
#define DE_DOSTYPE	16	/* ASCII (HEX) string showing filesystem type;
				 * 0X444F5300 is old filesystem,
				 * 0X444F5301 is fast file system */
#define DE_BAUD		17	/* Baud rate for serial handler */
#define DE_CONTROL	18	/* Control word for handler/filesystem */
#define DE_BOOTBLOCKS	19	/* Number of blocks containing boot code */


/* NEW ! BSD PARTITION TYPES. 

   These are chosen to more or less resemble the original BSD partition
   layout, but they don't come too close (we're much more flexible!)
   
   there's
   - a root-partition
   - a swap-partition
   - user-partitions
   
   This gives the following layout for the 8 BSD partitions A-H:
   
   A:	root		(first one found with DOSTYPE_BSD_ROOT)
   B:	swap		(first one found with DOSTYPE_BSD_SWAP)
   C:	whole disk	(the traditional way to nuke your whole harddisk...)
   D-H:	user		(DOSTYPE_BSD_D TO DOSTYPE_BSD_H) */

#define DOSTYPE_BSD_ROOT   0x42534452 /* 'BSDR' */
#define DOSTYPE_BSD_SWAP   0x42534453 /* 'BSDS' */
#define DOSTYPE_BSD_D	   0x42534444 /* 'BSDD' */
#define DOSTYPE_BSD_E	   0x42534445 /* 'BSDE' */
#define DOSTYPE_BSD_F	   0x42534446 /* 'BSDF' */
#define DOSTYPE_BSD_G	   0x42534447 /* 'BSDG' */
#define DOSTYPE_BSD_H	   0x42534448 /* 'BSDH' */

#endif _amiga_rdb_h
