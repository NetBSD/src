#define	SUNM_RDONLY	0x01	/* mount fs read-only */
#define	SUNM_NOSUID	0x02	/* mount fs with setuid disallowed */
#define	SUNM_NEWTYPE	0x04	/* type is string (char *), not int */
#define	SUNM_GRPID	0x08	/* (bsd semantics; ignored) */
#define	SUNM_REMOUNT	0x10	/* update existing mount */
#define	SUNM_NOSUB	0x20	/* prevent submounts (rejected) */
#define	SUNM_MULTI	0x40	/* (ignored) */
#define	SUNM_SYS5	0x80	/* Sys 5-specific semantics (rejected) */

struct sunos_nfs_args {
	struct	sockaddr_in *addr;	/* file server address */
	caddr_t	fh;			/* file handle to be mounted */
	int	flags;			/* flags */
	int	wsize;			/* write size in bytes */
	int	rsize;			/* read size in bytes */
	int	timeo;			/* initial timeout in .1 secs */
	int	retrans;		/* times to retry send */
	char	*hostname;		/* server's hostname */
	int	acregmin;		/* attr cache file min secs */
	int	acregmax;		/* attr cache file max secs */
	int	acdirmin;		/* attr cache dir min secs */
	int	acdirmax;		/* attr cache dir max secs */
	char	*netname;		/* server's netname */
	struct	pathcnf *pathconf;	/* static pathconf kludge */
};


/*
 * Here is the sun layout.  (Compare the BSD layout in <sys/dirent.h>.)
 * We can assume big-endian, so the BSD d_type field is just the high
 * byte of the SunOS d_namlen field, after adjusting for the extra "long".
 */
struct sunos_dirent {
	long	d_off;
	u_long	d_fileno;
	u_short	d_reclen;
	u_short	d_namlen;
	char	d_name[256];
};


struct sunos_ustat {
	daddr_t	f_tfree;	/* total free */
	ino_t	f_tinode;	/* total inodes free */
	char	f_path[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};

struct sunos_statfs {
	long	f_type;		/* type of info, zero for now */
	long	f_bsize;	/* fundamental file system block size */
	long	f_blocks;	/* total blocks in file system */
	long	f_bfree;	/* free blocks */
	long	f_bavail;	/* free blocks available to non-super-user */
	long	f_files;	/* total file nodes in file system */
	long	f_ffree;	/* free file nodes in fs */
	fsid_t	f_fsid;		/* file system id */
	long	f_spare[7];	/* spare for later */
};


struct sunos_utsname {
	char    sysname[9];
	char    nodename[9];
	char    nodeext[65-9];
	char    release[9];
	char    version[9];
	char    machine[9];
};


struct sunos_ttysize {
	int	ts_row;
	int	ts_col;
};

struct sunos_termio {
	u_short	c_iflag;
	u_short	c_oflag;
	u_short	c_cflag;
	u_short	c_lflag;
	char	c_line;
	unsigned char c_cc[8];
};
#define SUNOS_TCGETA	_IOR('T', 1, struct sunos_termio)
#define SUNOS_TCSETA	_IOW('T', 2, struct sunos_termio)
#define SUNOS_TCSETAW	_IOW('T', 3, struct sunos_termio)
#define SUNOS_TCSETAF	_IOW('T', 4, struct sunos_termio)
#define SUNOS_TCSBRK	_IO('T', 5)

struct sunos_termios {
	u_long	c_iflag;
	u_long	c_oflag;
	u_long	c_cflag;
	u_long	c_lflag;
	char	c_line;
	u_char	c_cc[17];
};
#define SUNOS_TCXONC	_IO('T', 6)
#define SUNOS_TCFLSH	_IO('T', 7)
#define SUNOS_TCGETS	_IOR('T', 8, struct sunos_termios)
#define SUNOS_TCSETS	_IOW('T', 9, struct sunos_termios)
#define SUNOS_TCSETSW	_IOW('T', 10, struct sunos_termios)
#define SUNOS_TCSETSF	_IOW('T', 11, struct sunos_termios)
#define SUNOS_TCSNDBRK	_IO('T', 12)
#define SUNOS_TCDRAIN	_IO('T', 13)

