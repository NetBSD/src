/*
 *	$Id: sddefs.h,v 1.1 1994/04/01 23:18:18 phil Exp $
 */

struct sd_data {
	int flags;
#define	SDVALID		0x02		/* PARAMS LOADED	*/
#define	SDINIT		0x04		/* device has been init'd */
#define	SDWAIT		0x08		/* device has someone waiting */
#define SDHAVELABEL	0x10		/* have read the label */
#define SDDOSPART	0x20		/* Have read host-dependent partition table */
#define SDWRITEPROT	0x40		/* Device in readonly mode (S/W)*/
	struct scsi_switch *sc_sw;	/* address of scsi low level switch */
	struct scsi_xfer *freexfer;	/* chain of free ones */
	struct buf sdbuf;
	int formatting;			/* format lock */
	int ctlr;			/* so they know which one we want */
	int targ;			/* our scsi target ID */
	int lu;				/* out scsi lu */
	long int ad_info;		/* info about the adapter */
	int cmdscount;			/* cmds allowed outstanding by board*/
	int wlabel;			/* label is writable */
	struct  disk_parms {
		u_char	heads;		/* Number of heads */
		u_short	cyls;		/* Number of cylinders */
		u_char	sectors;	/* Number of sectors/track */
		u_short	secsiz;		/* Number of bytes/sector */
		u_long	disksize;		/* total number sectors */
	} params;
	unsigned int sd_start_of_unix;	/* unix vs host-dependent partitions */
	struct disklabel disklabel;
	struct cpu_disklabel cpudisklabel;
	int partflags[MAXPARTITIONS];	/* per partition flags */
#define SDOPEN	0x01
	int openparts;		/* one bit for each open partition */
	int blockwait;
};

int sdattach(int, struct scsi_switch *, int, int *);
int sdopen(int);
struct scsi_xfer *sd_get_xs(int, int);
void sd_free_xs(int, struct scsi_xfer *, int);
void sdminphys(struct buf *);
void sdstrategy(struct buf *);
void sdstart(int);
int sd_done(int, struct scsi_xfer *);
int sdioctl(dev_t, int, caddr_t, int);
int sdgetdisklabel(u_char);
int sd_size(int, int);
int sd_test_unit_ready(int, int);
void sd_dump();
int sdsize(dev_t);
int sd_interpret_sense(int, struct scsi_xfer *);
int sd_scsi_cmd(int, struct scsi_generic *, int, u_char *, int, int, int);
int sd_close(dev_t);
int sd_get_parms(int, int);
int sd_reassign_blocks(int, int);
int sd_start_unit(int, int);
int sd_prevent(int, int, int);
