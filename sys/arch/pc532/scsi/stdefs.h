/*
 *	$Id: stdefs.h,v 1.1 1994/04/01 23:18:22 phil Exp $
 */

#define STQSIZE		4
struct	st_data {
	struct scsi_switch *sc_sw;	/* address of scsi low level switch */
	int flags;
	int ctlr;			/* so they know which one we want */
	int targ;			/* our scsi target ID */
	int lu;				/* our scsi lu */
	int blkmin;			/* min blk size */
	int blkmax;			/* max blk size */
	int numblks;			/* nominal blocks capacity */
	int blksiz;			/* nominal block size */
	int info_valid;			/* the info about the device is valid */
	int initialized;
	struct buf buf[STQSIZE]; 	/* buffer for raw io (one per device) */
	struct buf buf_queue;
	struct scsi_xfer scsi_xfer;
	int blockwait;
};
#define ST_OPEN		0x01
#define	ST_NOREWIND	0x02
#define	ST_WRITTEN	0x04
#define	ST_FIXEDBLOCKS	0x10
#define	ST_AT_FILEMARK	0x20
#define	ST_AT_EOM	0x40

#define	ST_PER_ACTION	(ST_AT_FILEMARK | ST_AT_EOM)
#define	ST_PER_OPEN	(ST_OPEN | ST_NOREWIND | ST_WRITTEN | ST_PER_ACTION)
#define	ST_PER_MEDIA	ST_FIXEDBLOCKS

int stattach(int, struct scsi_switch *, int, int *);
int stopen(dev_t);
int stclose(dev_t);
void stminphys(struct buf *);
void ststrategy(struct buf *);
int ststart(int);
int st_done(int, struct scsi_xfer *);
int stioctl(dev_t, int, caddr_t, int);
int st_req_sense(int, int);
int st_test_ready(int, int);
int st_rd_blk_lim(int, int);
int st_mode_sense(int, int);
int st_mode_select(int, int, int);
int st_space(int, int, int, int);
int st_write_filemarks(int, int, int);
int st_load(int, int, int);
int st_prevent(int, int, int);
int st_rewind(int, int, int);
int st_scsi_cmd(int, struct scsi_generic *, int, u_char *, int, int, int);
int st_interpret_sense(int, struct scsi_xfer *);
int stsize(dev_t);
int stdump(void);

