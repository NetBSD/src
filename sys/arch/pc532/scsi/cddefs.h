/*
 * 	$Id: cddefs.h,v 1.1 1994/04/01 23:18:07 phil Exp $
 */

struct cd_data {
	int	flags;
#define	CDVALID		0x02		/* PARAMS LOADED	*/
#define	CDINIT		0x04		/* device has been init'd */
#define	CDWAIT		0x08		/* device has someone waiting */
#define CDHAVELABEL	0x10		/* have read the label */
	struct scsi_switch *sc_sw;	/* address of scsi low level switch */
	int	ctlr;			/* so they know which one we want */
	int	targ;			/* our scsi target ID */
	int	lu;			/* out scsi lu */
	int	cmdscount;		/* cmds allowed outstanding by board*/
	struct  cd_parms {
		int blksize;
		u_long disksize;		/* total number sectors */
	} params;
	struct disklabel	disklabel;
	int	partflags[MAXPARTITIONS];	/* per partition flags */
#define CDOPEN	0x01
	int		openparts;		/* one bit for each open partition */
};

int cdattach(int, struct scsi_switch *, int, int *);
int cdopen(dev_t);
struct scsi_xfer * cd_get_xs(int, int);
void cd_free_xs(int, struct scsi_xfer *, int);
void cdminphys(struct buf *);
void cdstrategy(struct buf *);
void cdstart(int);
int cd_done(int, struct scsi_xfer *);
int cdioctl(dev_t, int, caddr_t, int);
int cdgetdisklabel(int);
int cd_size(int, int);
int cd_req_sense(int, int);
int cd_get_mode(int, struct cd_mode_data *, int);
int cd_set_mode(int, struct cd_mode_data *);
int cd_play(int, int, int);
int cd_play_big(int, int, int);
int cd_play_tracks(int, int, int, int, int);
int cd_pause(int, int);
int cd_reset(int);
int cd_start_unit(int, int, int);
int cd_prevent_unit(int, int, int);
int cd_read_subchannel(int, int, int, int, struct cd_sub_channel_info *, int);
int cd_read_toc(int, int, int, struct cd_toc_entry *, int);
int cd_get_parms(int, int);
int cdclose(dev_t);
int cd_scsi_cmd(int, struct scsi_generic *, int, u_char *, int, int, int);
int cd_interpret_sense(int, struct scsi_xfer *);
int cdsize(dev_t);
int show_mem(unsigned char *, int);
