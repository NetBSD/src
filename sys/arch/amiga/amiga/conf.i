# 1 "conf.c"
 


































# 1 "../../../sys/param.h" 1
 

















































# 1 "../../../sys/types.h" 1
 





































typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		 

typedef	char *	caddr_t;		 
typedef	long	daddr_t;		 
typedef	short	dev_t;			 
typedef	u_long	ino_t;			 
typedef	long	off_t;			 
typedef	u_short	nlink_t;		 
typedef	long	swblk_t;		 
typedef	long	segsz_t;		 
typedef	u_short	uid_t;			 
typedef	u_short	gid_t;			 
typedef	short	pid_t;			 
typedef	u_short	mode_t;			 
typedef u_long	fixpt_t;		 


typedef	struct	_uquad	{ u_long val[2]; } u_quad;
typedef	struct	_quad	{   long val[2]; } quad;
typedef	long *	qaddr_t;	 






# 1 "../compile/amiga/machine/ansi.h" 1
 





































 


















# 69 "../../../sys/types.h" 2


# 1 "../compile/amiga/machine/types.h" 1
 





































typedef struct _physadr {
	short r[1];
} *physadr;

typedef struct label_t {		 
	int val[15];
} label_t;

typedef	u_long	vm_offset_t;
typedef	u_long	vm_size_t;


# 71 "../../../sys/types.h" 2




typedef	unsigned long			clock_t;




typedef	unsigned long			size_t;




typedef	long				time_t;






 









typedef long	fd_mask;






typedef	struct fd_set {
	fd_mask	fds_bits[(((256 )+(( (sizeof(fd_mask) * 8		)	)-1))/( (sizeof(fd_mask) * 8		)	)) ];
} fd_set;







 




struct	proc;
struct	pgrp;
struct	ucred;
struct	rusage;
struct	file;
struct	buf;
struct	tty;
struct	uio;





# 51 "../../../sys/param.h" 2



 






# 1 "../../../sys/syslimits.h" 1
 



























































# 61 "../../../sys/param.h" 2












 

# 1 "../../../sys/cdefs.h" 1
 













































 











# 76 "../../../sys/cdefs.h"



# 75 "../../../sys/param.h" 2

# 1 "../../../sys/errno.h" 1
 




















































					 


























 



 






 













 

















 






 





 















 





# 76 "../../../sys/param.h" 2

# 1 "../../../sys/time.h" 1
 





































 



struct timeval {
	long	tv_sec;		 
	long	tv_usec;	 
};

struct timezone {
	int	tz_minuteswest;	 
	int	tz_dsttime;	 
};








 










 







struct	itimerval {
	struct	timeval it_interval;	 
	struct	timeval it_value;	 
};

# 100 "../../../sys/time.h"



# 77 "../../../sys/param.h" 2

# 1 "../../../sys/resource.h" 1
 





































 









 






struct	rusage {
	struct timeval ru_utime;	 
	struct timeval ru_stime;	 
	long	ru_maxrss;		 

	long	ru_ixrss;		 
	long	ru_idrss;		 
	long	ru_isrss;		 
	long	ru_minflt;		 
	long	ru_majflt;		 
	long	ru_nswap;		 
	long	ru_inblock;		 
	long	ru_oublock;		 
	long	ru_msgsnd;		 
	long	ru_msgrcv;		 
	long	ru_nsignals;		 
	long	ru_nvcsw;		 
	long	ru_nivcsw;		 

};

 
















struct rlimit {
	long	rlim_cur;		 
	long	rlim_max;		 
};

# 110 "../../../sys/resource.h"



# 78 "../../../sys/param.h" 2

# 1 "../../../sys/ucred.h" 1
 





































 


struct ucred {
	u_short	cr_ref;			 
	uid_t	cr_uid;			 
	short	cr_ngroups;		 
	gid_t	cr_groups[	16		];	 
};





struct ucred *crget();
struct ucred *crcopy();
struct ucred *crdup();



# 79 "../../../sys/param.h" 2

# 1 "../../../sys/uio.h" 1
 





































struct iovec {
	caddr_t	iov_base;
	int	iov_len;
};

enum	uio_rw { UIO_READ, UIO_WRITE };

 


enum	uio_seg {
	UIO_USERSPACE,		 
	UIO_SYSSPACE,		 
	UIO_USERISPACE		 
};

struct uio {
	struct	iovec *uio_iov;
	int	uio_iovcnt;
	off_t	uio_offset;
	int	uio_resid;
	enum	uio_seg uio_segflg;
	enum	uio_rw uio_rw;
	struct	proc *uio_procp;
};

  





# 80 "../../../sys/uio.h"



# 80 "../../../sys/param.h" 2



 
# 1 "../../../sys/signal.h" 1
 








































# 1 "../compile/amiga/machine/trap.h" 1
 









































 





















# 42 "../../../sys/signal.h" 2



















































typedef	void (*sig_t) (int)		;


typedef void (*__sighandler_t) (int)		;
typedef unsigned int sigset_t;

 
int	sigaddset (sigset_t *, int)		;
int	sigdelset (sigset_t *, int)		;
int	sigemptyset (sigset_t *)		;
int	sigfillset (sigset_t *)		;
int	sigismember (const sigset_t *, int)		;
 







 


struct	sigaction {
	__sighandler_t  sa_handler;      
	sigset_t sa_mask;		 
	int	sa_flags;		 
};






 







 



struct	sigvec {
	void	(*sv_handler)();	 
	int	sv_mask;		 
	int	sv_flags;		 
};




 


struct	sigaltstack {
	char	*ss_base;		 
	int	ss_len;			 
	int	ss_onstack;		 
};

 


struct	sigstack {
	char	*ss_sp;			 
	int	ss_onstack;		 
};

 






struct	sigcontext {
	int	sc_onstack;		 
	int	sc_mask;		 
	int	sc_sp;			 
	int	sc_fp;			 
	int	sc_ap;			 
	int	sc_pc;			 
	int	sc_ps;			 
};

 







  



# 221 "../../../sys/signal.h"



# 84 "../../../sys/param.h" 2


 
# 1 "../compile/amiga/machine/param.h" 1
 









































 




 


























 





 


















 

 




 




 


 







 







 











 


# 1 "../compile/amiga/machine/psl.h" 1
 



































 






























 





# 146 "../compile/amiga/machine/param.h" 2











 


















 




int	cpuspeed;







# 87 "../../../sys/param.h" 2

# 1 "../compile/amiga/machine/endian.h" 1
 


































 













 
unsigned long	htonl (unsigned long)		;
unsigned short	htons (unsigned short)		;
unsigned long	ntohl (unsigned long)		;
unsigned short	ntohs (unsigned short)		;
 

 




















# 88 "../../../sys/param.h" 2

# 1 "../compile/amiga/machine/limits.h" 1
 
























































# 89 "../../../sys/param.h" 2


 


























 






















				 



 











 











 





 






 








 

















 














# 36 "conf.c" 2

# 1 "../../../sys/systm.h" 1
 






































extern char *panicstr;		 
extern char version[];		 
extern char copyright[];	 

extern int nblkdev;		 
extern int nchrdev;		 
extern int nswdev;		 
extern int nswap;		 

extern int selwait;		 

extern u_char curpri;		 

extern int maxmem;		 
extern int physmem;		 

extern dev_t dumpdev;		 
extern long dumplo;		 

extern dev_t rootdev;		 
extern struct vnode *rootvp;	 

extern dev_t swapdev;		 
extern struct vnode *swapdev_vp; 

extern struct sysent {		 
	int	sy_narg;	 
	int	(*sy_call)();	 
} sysent[];

extern int boothowto;		 




 



 


int	nullop (void)		;
int	enodev (void)		;
int	enoioctl (void)		;
int	enxio (void)		;
int	eopnotsupp (void)		;
int	selscan (struct proc *p, fd_set *ibits, fd_set *obits,
		int nfd, int *retval)		;
int	seltrue (dev_t dev, int which, struct proc *p)		;

 



void	panic (char *)		;
void	tablefull (char *)		;
void	addlog (const char *, ...)		;
void	log (int, const char *, ...)		;
void	printf (const char *, ...)		;
int	sprintf (char *buf, const char *, ...)		;
void	ttyprintf (struct tty *, const char *, ...)		;

void	bcopy (void *from, void *to, u_int len)		;
void	ovbcopy (void *from, void *to, u_int len)		;
void	bzero (void *buf, u_int len)		;
int	bcmp (void *str1, void *str2, u_int len)		;
int	strlen (const char *string)		;

int	copystr (void *kfaddr, void *kdaddr, u_int len, u_int *done)		;
int	copyinstr (void *udaddr, void *kaddr, u_int len, u_int *done)		;
int	copyoutstr (void *kaddr, void *udaddr, u_int len, u_int *done)		;
int	copyin (void *udaddr, void *kaddr, u_int len)		;
int	copyout (void *kaddr, void *udaddr, u_int len)		;

int	fubyte (void *base)		;



int	subyte (void *base, int byte)		;
int	suibyte (void *base, int byte)		;
int	fuword (void *base)		;
int	fuiword (void *base)		;
int	suword (void *base, int word)		;
int	suiword (void *base, int word)		;

int	scanc (unsigned size, u_char *cp, u_char *table, int mask)		;
int	skpc (int mask, int size, char *cp)		;
int	locc (int mask, char *cp, unsigned size)		;
int	ffs (long value)		;


# 37 "conf.c" 2

# 1 "../../../sys/buf.h" 1
 













































 



















 




struct bufhd
{
	long	b_flags;		 
	struct	buf *b_forw, *b_back;	 
};
struct buf
{
	long	b_flags;		 
	struct	buf *b_forw, *b_back;	 
	struct	buf *av_forw, *av_back;	 
	struct	buf *b_blockf, **b_blockb; 


	long	b_bcount;		 
	long	b_bufsize;		 

	short	b_error;		 
	dev_t	b_dev;			 
	union {
	    caddr_t b_addr;		 
	    int	*b_words;		 
	    struct fs *b_fs;		 
	    struct csum *b_cs;		 
	    struct cg *b_cg;		 
	    struct dinode *b_dino;	 
	    daddr_t *b_daddr;		 
	} b_un;
	daddr_t	b_lblkno;		 
	daddr_t	b_blkno;		 
	long	b_resid;		 

	struct  proc *b_proc;		 
	int	(*b_iodone)();		 
	struct	vnode *b_vp;		 
	int	b_pfcent;		 
	struct	ucred *b_rcred;		 
	struct	ucred *b_wcred;		 
	int	b_dirtyoff;		 
	int	b_dirtyend;		 
	caddr_t	b_saveaddr;		 
};











 








struct	buf *buf;		 
char	*buffers;
int	nbuf;			 
int	bufpages;		 
struct	buf *swbuf;		 
int	nswbuf;
struct	bufhd bufhash[512 ];	 
struct	buf bfreelist[	4		];	 
struct	buf bswlist;		 
struct	buf *bclnlist;		 

void bufinit();
int bread(struct vnode *, daddr_t, int, struct ucred *, struct buf **);
int breada(struct vnode *, daddr_t, int, daddr_t, int, struct ucred *,
	struct buf **);
int bwrite(struct buf *);
void bawrite(struct buf *);
void bdwrite(struct buf *);
void brelse(struct buf *);
struct buf *incore(struct vnode *, daddr_t);
struct buf *getblk(struct vnode *, daddr_t, int);
struct buf *geteblk(int);
int biowait(struct buf *);
int biodone(struct buf *);
void allocbuf(struct buf *, int);



 



























 













 






















 










# 38 "conf.c" 2

# 1 "../../../sys/ioctl.h" 1
 












































 



struct winsize {
	unsigned short	ws_row;		 
	unsigned short	ws_col;		 
	unsigned short	ws_xpixel;	 
	unsigned short	ws_ypixel;	 
};

 


struct ttysize {
	unsigned short	ts_lines;
	unsigned short	ts_cols;
	unsigned short	ts_xxx;
	unsigned short	ts_yyy;
};



 





















 















						 


						 

						 






						 






						 











































 





































# 216 "../../../sys/ioctl.h"




 













# 39 "conf.c" 2

# 1 "../../../sys/tty.h" 1
 






































# 1 "../../../sys/select.h" 1
 































 

struct selinfo {
	pid_t	si_pid;		 
	int	si_coll;	 
};


 
void	selrecord  (struct proc *, struct selinfo *)		;

 
void	selwakeup  (struct selinfo *)		;


# 40 "../../../sys/tty.h" 2

# 1 "../../../sys/termios.h" 1
 





































 



 



















 
















 









 


















 









 
























 

































typedef unsigned long	tcflag_t;
typedef unsigned char	cc_t;
typedef long		speed_t;

struct termios {
	tcflag_t	c_iflag;	 
	tcflag_t	c_oflag;	 
	tcflag_t	c_cflag;	 
	tcflag_t	c_lflag;	 
	cc_t		c_cc[	20 ];	 
	long		c_ispeed;	 
	long		c_ospeed;	 
};

 









 























# 256 "../../../sys/termios.h"






# 1 "../../../sys/ttydefaults.h" 1
 





































 



 








 





















 






 










# 262 "../../../sys/termios.h" 2





# 41 "../../../sys/tty.h" 2


 





typedef short rbchar;

struct ringb {
	rbchar	*rb_hd;	   
	rbchar	*rb_tl;	   
	rbchar	rb_buf[1024 ];	 
};























 






struct tty {
	int	(*t_oproc)();		 
	int	(*t_param)();		 
	struct selinfo t_rsel;		 
	struct selinfo t_wsel;
	caddr_t	T_LINEP; 		 
	caddr_t	t_addr;			 
	dev_t	t_dev;			 
	int	t_flags;		 
	int	t_state;		 
	struct	session *t_session;	 
	struct	pgrp *t_pgrp;		 
	char	t_line;			 
	short	t_col;			 
	short	t_rocount, t_rocol;	 
	short	t_hiwat;		 
	short	t_lowat;		 
	struct	winsize t_winsize;	 
	struct	termios t_termios;	 









	long	t_cancc;		 
	long	t_rawcc;
	long	t_outcc;
	short	t_gen;			 
	short	t_mask;			 
	struct	ringb t_raw;		 
	struct	ringb t_can;
	struct	ringb t_out;
};













extern	struct ttychars ttydefaults;


 









 

 
 

 








 








struct speedtab {
        int sp_speed;
        int sp_code;
};
 








 




 




 








 
extern	 char ttyin[], ttyout[], ttopen[], ttclos[], ttybg[], ttybuf[];



# 40 "conf.c" 2

# 1 "../../../sys/conf.h" 1
 





































 




struct tty;


struct bdevsw {
	int	(*d_open)	(dev_t dev, int oflags, int devtype,
				     struct proc *p)		;
	int	(*d_close)	(dev_t dev, int fflag, int devtype,
				     struct proc *)		;
	int	(*d_strategy)	(struct buf *bp)		;
	int	(*d_ioctl)	(dev_t dev, int cmd, caddr_t data,
				     int fflag, struct proc *p)		;
	int	(*d_dump)	(dev_t dev)		;
	int	(*d_psize)	(dev_t dev)		;
	int	d_flags;
};


struct bdevsw bdevsw[];


struct cdevsw {
	int	(*d_open)	(dev_t dev, int oflags, int devtype,
				     struct proc *p)		;
	int	(*d_close)	(dev_t dev, int fflag, int devtype,
				     struct proc *)		;
	int	(*d_read)	(dev_t dev, struct uio *uio, int ioflag)		;
	int	(*d_write)	(dev_t dev, struct uio *uio, int ioflag)		;
	int	(*d_ioctl)	(dev_t dev, int cmd, caddr_t data,
				     int fflag, struct proc *p)		;
	int	(*d_stop)	(struct tty *tp, int rw)		;
	int	(*d_reset)	(int uban)		;	 
	struct	tty *d_ttys;
	int	(*d_select)	(dev_t dev, int which, struct proc *p)		;
	int	(*d_mmap)	()		;
	int	(*d_strategy)	(struct buf *bp)		;
};


struct cdevsw cdevsw[];

 
extern char devopn[], devio[], devwait[], devin[], devout[];
extern char devioc[], devcls[];


struct linesw {
	int	(*l_open)();
	int	(*l_close)();
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)();
	int	(*l_rint)();
	int	(*l_rend)();
	int	(*l_meta)();
	int	(*l_start)();
	int	(*l_modem)();
};


struct linesw linesw[];


struct swdevt {
	dev_t	sw_dev;
	int	sw_freed;
	int	sw_nblks;
	struct	vnode *sw_vp;
};


struct swdevt swdevt[];



# 41 "conf.c" 2


int	rawread		(dev_t, struct uio *, int)		;
int	rawwrite	(dev_t, struct uio *, int)		;
int	swstrategy	(struct buf *)		;
int	ttselect	(dev_t, int, struct proc *)		;







 







 






















int noopen  (dev_t, int, int, struct proc *)		  ; int noclose  (dev_t, int, int, struct proc *)		  ; int nostrategy  (struct buf *)		  ; int noioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int nodump  (dev_t)		  ; int nosize  (dev_t)		   ;	 

# 1 "../compile/amiga/sd.h" 1

# 87 "conf.c" 2

# 1 "../compile/amiga/st.h" 1

# 88 "conf.c" 2

# 1 "../compile/amiga/vn.h" 1

# 89 "conf.c" 2


int sdopen  (dev_t, int, int, struct proc *)		  ; int sdclose  (dev_t, int, int, struct proc *)		  ; int sdstrategy  (struct buf *)		  ; int sdioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int sddump  (dev_t)		  ; int sdsize  (dev_t)		   ;
int stopen  (dev_t, int, int, struct proc *)		  ; int stclose  (dev_t, int, int, struct proc *)		  ; int ststrategy  (struct buf *)		  ; int stioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int stdump  (dev_t)		  ; int stsize  (dev_t)		   ;
int vnopen  (dev_t, int, int, struct proc *)		  ; int vnclose  (dev_t, int, int, struct proc *)		  ; int vnstrategy  (struct buf *)		  ; int vnioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int vndump  (dev_t)		  ; int vnsize  (dev_t)		   ;


struct bdevsw	bdevsw[] =
{
	{ (0 > 0 ? noopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (0 > 0 ? noclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (0 > 0 ? nostrategy  : (int (*) (struct buf *)		 ) enxio) , (0 > 0 ? noioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (0 > 0 ? nodump  : (int (*) (dev_t)		 ) enxio) , 0, 	0x000400	 }  ,		 
	{ (0 > 0 ? noopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (0 > 0 ? noclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (0 > 0 ? nostrategy  : (int (*) (struct buf *)		 ) enxio) , (0 > 0 ? noioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (0 > 0 ? nodump  : (int (*) (dev_t)		 ) enxio) , 0, 	0x000400	 }  ,		 
	{ (0 > 0 ? noopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (0 > 0 ? noclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (0 > 0 ? nostrategy  : (int (*) (struct buf *)		 ) enxio) , (0 > 0 ? noioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (0 > 0 ? nodump  : (int (*) (dev_t)		 ) enxio) , 0, 	0x000400	 }  ,		 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, int, int, struct proc *)		 ) enodev, swstrategy, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (dev_t)		 ) enodev, 0, 0 } ,	 
	{ (7  > 0 ? sdopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (int (*) (dev_t, int, int, struct proc *)		 ) nullop, (7  > 0 ? sdstrategy  : (int (*) (struct buf *)		 ) enxio) , (7  > 0 ? sdioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (7  > 0 ? sddump  : (int (*) (dev_t)		 ) enxio) , (7  > 0 ? sdsize  : 0) , 0 } ,	 
	{ (0 > 0 ? noopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (0 > 0 ? noclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (0 > 0 ? nostrategy  : (int (*) (struct buf *)		 ) enxio) , (0 > 0 ? noioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (0 > 0 ? nodump  : (int (*) (dev_t)		 ) enxio) , 0, 	0x000400	 }  ,		 
	{ (10  > 0 ? vnopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (int (*) (dev_t, int, int, struct proc *)		 ) nullop, (10  > 0 ? vnstrategy  : (int (*) (struct buf *)		 ) enxio) , (10  > 0 ? vnioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (10  > 0 ? vndump  : (int (*) (dev_t)		 ) enxio) , (10  > 0 ? vnsize  : 0) , 0 } ,	 
	{ (7  > 0 ? stopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (7  > 0 ? stclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (7  > 0 ? ststrategy  : (int (*) (struct buf *)		 ) enxio) , (7  > 0 ? stioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (7  > 0 ? stdump  : (int (*) (dev_t)		 ) enxio) , 0, 	0x000400	 } ,	 
};

int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

 















 






 






 













int noopen  (dev_t, int, int, struct proc *)		  ; int noclose  (dev_t, int, int, struct proc *)		  ; int noread  (dev_t, struct uio *, int)		  ; int nowrite  (dev_t, struct uio *, int)		  ; int noioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int nostop  (struct tty *, int)		  ; int noreset  (int)		  ; int noselect  (dev_t, int, struct proc *)		  ; int nomap  ()		  ; int nostrategy  (struct buf *)		  ; extern struct tty no_tty [] ;			 

int cnopen  (dev_t, int, int, struct proc *)		  ; int cnclose  (dev_t, int, int, struct proc *)		  ; int cnread  (dev_t, struct uio *, int)		  ; int cnwrite  (dev_t, struct uio *, int)		  ; int cnioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int cnstop  (struct tty *, int)		  ; int cnreset  (int)		  ; int cnselect  (dev_t, int, struct proc *)		  ; int cnmap  ()		  ; int cnstrategy  (struct buf *)		  ; extern struct tty cn_tty [] ;
 






int cttyopen  (dev_t, int, int, struct proc *)		  ; int cttyclose  (dev_t, int, int, struct proc *)		  ; int cttyread  (dev_t, struct uio *, int)		  ; int cttywrite  (dev_t, struct uio *, int)		  ; int cttyioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int cttystop  (struct tty *, int)		  ; int cttyreset  (int)		  ; int cttyselect  (dev_t, int, struct proc *)		  ; int cttymap  ()		  ; int cttystrategy  (struct buf *)		  ; extern struct tty ctty_tty [] ;
 






int mmrw (dev_t, struct uio *, int)		 ;
 





 






# 1 "../compile/amiga/pty.h" 1

# 186 "conf.c" 2



int ptsopen  (dev_t, int, int, struct proc *)		  ; int ptsclose  (dev_t, int, int, struct proc *)		  ; int ptsread  (dev_t, struct uio *, int)		  ; int ptswrite  (dev_t, struct uio *, int)		  ; int ptyioctl   (dev_t, int, caddr_t, int, struct proc *)		  ; int ptsstop  (struct tty *, int)		  ; int ptsreset  (int)		  ; int ptsselect  (dev_t, int, struct proc *)		  ; int ptsmap  ()		  ; int ptsstrategy  (struct buf *)		  ; extern struct tty 	pt_tty  [] ;


int ptcopen  (dev_t, int, int, struct proc *)		  ; int ptcclose  (dev_t, int, int, struct proc *)		  ; int ptcread  (dev_t, struct uio *, int)		  ; int ptcwrite  (dev_t, struct uio *, int)		  ; int ptyioctl   (dev_t, int, caddr_t, int, struct proc *)		  ; int ptcstop  (struct tty *, int)		  ; int ptcreset  (int)		  ; int ptcselect  (dev_t, int, struct proc *)		  ; int ptcmap  ()		  ; int ptcstrategy  (struct buf *)		  ; extern struct tty 	pt_tty  [] ;

 






int logopen  (dev_t, int, int, struct proc *)		  ; int logclose  (dev_t, int, int, struct proc *)		  ; int logread  (dev_t, struct uio *, int)		  ; int logwrite  (dev_t, struct uio *, int)		  ; int logioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int logstop  (struct tty *, int)		  ; int logreset  (int)		  ; int logselect  (dev_t, int, struct proc *)		  ; int logmap  ()		  ; int logstrategy  (struct buf *)		  ; extern struct tty log_tty [] ;
 






int stopen  (dev_t, int, int, struct proc *)		  ; int stclose  (dev_t, int, int, struct proc *)		  ; int stread  (dev_t, struct uio *, int)		  ; int stwrite  (dev_t, struct uio *, int)		  ; int stioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int ststop  (struct tty *, int)		  ; int streset  (int)		  ; int stselect  (dev_t, int, struct proc *)		  ; int stmap  ()		  ; int ststrategy  (struct buf *)		  ; extern struct tty st_tty [] ;
int sdopen  (dev_t, int, int, struct proc *)		  ; int sdclose  (dev_t, int, int, struct proc *)		  ; int sdread  (dev_t, struct uio *, int)		  ; int sdwrite  (dev_t, struct uio *, int)		  ; int sdioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int sdstop  (struct tty *, int)		  ; int sdreset  (int)		  ; int sdselect  (dev_t, int, struct proc *)		  ; int sdmap  ()		  ; int sdstrategy  (struct buf *)		  ; extern struct tty sd_tty [] ;

 
int grfopen  (dev_t, int, int, struct proc *)		  ; int grfclose  (dev_t, int, int, struct proc *)		  ; int grfread  (dev_t, struct uio *, int)		  ; int grfwrite  (dev_t, struct uio *, int)		  ; int grfioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int grfstop  (struct tty *, int)		  ; int grfreset  (int)		  ; int grfselect  (dev_t, int, struct proc *)		  ; int grfmap  ()		  ; int grfstrategy  (struct buf *)		  ; extern struct tty grf_tty [] ;
 






# 1 "../compile/amiga/ser.h" 1

# 221 "conf.c" 2

int seropen  (dev_t, int, int, struct proc *)		  ; int serclose  (dev_t, int, int, struct proc *)		  ; int serread  (dev_t, struct uio *, int)		  ; int serwrite  (dev_t, struct uio *, int)		  ; int serioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int serstop  (struct tty *, int)		  ; int serreset  (int)		  ; int serselect  (dev_t, int, struct proc *)		  ; int sermap  ()		  ; int serstrategy  (struct buf *)		  ; extern struct tty ser_tty [] ;

# 1 "../compile/amiga/ite.h" 1

# 224 "conf.c" 2

int iteopen  (dev_t, int, int, struct proc *)		  ; int iteclose  (dev_t, int, int, struct proc *)		  ; int iteread  (dev_t, struct uio *, int)		  ; int itewrite  (dev_t, struct uio *, int)		  ; int iteioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int itestop  (struct tty *, int)		  ; int itereset  (int)		  ; int iteselect  (dev_t, int, struct proc *)		  ; int itemap  ()		  ; int itestrategy  (struct buf *)		  ; extern struct tty ite_tty [] ;
 






int vnopen  (dev_t, int, int, struct proc *)		  ; int vnclose  (dev_t, int, int, struct proc *)		  ; int vnread  (dev_t, struct uio *, int)		  ; int vnwrite  (dev_t, struct uio *, int)		  ; int vnioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int vnstop  (struct tty *, int)		  ; int vnreset  (int)		  ; int vnselect  (dev_t, int, struct proc *)		  ; int vnmap  ()		  ; int vnstrategy  (struct buf *)		  ; extern struct tty vn_tty [] ;
 






int fdopen (dev_t, int, int, struct proc *)		 ;
 







# 1 "../compile/amiga/bpfilter.h" 1

# 250 "conf.c" 2

int bpfopen  (dev_t, int, int, struct proc *)		  ; int bpfclose  (dev_t, int, int, struct proc *)		  ; int bpfread  (dev_t, struct uio *, int)		  ; int bpfwrite  (dev_t, struct uio *, int)		  ; int bpfioctl  (dev_t, int, caddr_t, int, struct proc *)		  ; int bpfstop  (struct tty *, int)		  ; int bpfreset  (int)		  ; int bpfselect  (dev_t, int, struct proc *)		  ; int bpfmap  ()		  ; int bpfstrategy  (struct buf *)		  ; extern struct tty bpf_tty [] ;
 







struct cdevsw	cdevsw[] =
{
	{ (1 > 0 ? cnopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1 > 0 ? cnclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1 > 0 ? cnread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1 > 0 ? cnwrite  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1 > 0 ? cnioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (int (*) (struct tty *, int)		 ) nullop, (int (*) (int)		 ) nullop, 0, (1 > 0 ? cnselect  : (int (*) (dev_t, int, struct proc *)		 ) enxio) , (int (*) ()		 ) enodev, 0 } ,		 
	{ (1 > 0 ? cttyopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (int (*) (dev_t, int, int, struct proc *)		 ) nullop, (1 > 0 ? cttyread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1 > 0 ? cttywrite  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1 > 0 ? cttyioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (int (*) (struct tty *, int)		 ) nullop, (int (*) (int)		 ) nullop, 0, (1 > 0 ? cttyselect  : (int (*) (dev_t, int, struct proc *)		 ) enxio) , (int (*) ()		 ) enodev, 0 } ,		 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) nullop, (int (*) (dev_t, int, int, struct proc *)		 ) nullop, mmrw, mmrw, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) nullop, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, 0 } ,		 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) nullop, (int (*) (dev_t, int, int, struct proc *)		 ) nullop, rawread, rawwrite, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, (int (*) (dev_t, int, struct proc *)		 ) enodev, (int (*) ()		 ) enodev, (1 > 0 ? swstrategy  : (int (*) (struct buf *)		 ) enxio)  } ,		 
	{ (1  > 0 ? ptsopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1  > 0 ? ptsclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1  > 0 ? ptsread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1  > 0 ? ptswrite  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1  > 0 ? ptyioctl   : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (1  > 0 ? ptsstop  : (int (*) (struct tty *, int)		 ) enxio) , (int (*) (int)		 ) nullop, (1  > 0 ? 	pt_tty   : 0) , ttselect, (int (*) ()		 ) enodev, 0 } ,	 
	{ (1  > 0 ? ptcopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1  > 0 ? ptcclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1  > 0 ? ptcread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1  > 0 ? ptcwrite  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1  > 0 ? ptyioctl   : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (int (*) (struct tty *, int)		 ) nullop, (int (*) (int)		 ) nullop, (1  > 0 ? 	pt_tty   : 0) , (1  > 0 ? ptcselect  : (int (*) (dev_t, int, struct proc *)		 ) enxio) , (int (*) ()		 ) enodev, 0 } ,	 
	{ (1 > 0 ? logopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1 > 0 ? logclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1 > 0 ? logread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (int (*) (dev_t, struct uio *, int)		 ) enodev, (1 > 0 ? logioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, (1 > 0 ? logselect  : (int (*) (dev_t, int, struct proc *)		 ) enxio) , (int (*) ()		 ) enodev, 0 } ,		 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, 0 } ,			 
	{ (7  > 0 ? sdopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (int (*) (dev_t, int, int, struct proc *)		 ) nullop, (7  > 0 ? rawread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (7  > 0 ? rawwrite  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (7  > 0 ? sdioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, (7  > 0 ? sdstrategy  : (int (*) (struct buf *)		 ) enxio)  } ,		 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, 0 } ,			 
	{ (1 > 0 ? grfopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1 > 0 ? grfclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (int (*) (dev_t, struct uio *, int)		 ) nullop, (int (*) (dev_t, struct uio *, int)		 ) nullop, (1 > 0 ? grfioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, (1 > 0 ? grfselect  : (int (*) (dev_t, int, struct proc *)		 ) enxio) , (1 > 0 ? grfmap  : (int (*) ()		 ) enxio) , 0 } ,		 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, 0 } ,			 
	{ (1 > 0 ? seropen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1 > 0 ? serclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1 > 0 ? serread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1 > 0 ? serwrite  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1 > 0 ? serioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (1 > 0 ? serstop  : (int (*) (struct tty *, int)		 ) enxio) , (int (*) (int)		 ) nullop, (1 > 0 ? ser_tty  : 0) , ttselect, (int (*) ()		 ) enodev, 0 } ,		 
	{ (1  > 0 ? iteopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1  > 0 ? iteclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (1  > 0 ? iteread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1  > 0 ? itewrite  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (1  > 0 ? iteioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, (1  > 0 ? ite_tty  : 0) , ttselect, (int (*) ()		 ) enodev, 0 } ,	 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, 0 } ,			 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, 0 } ,			 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, 0 } ,			 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, 0 } ,			 
	{ (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, 0 } ,			 
	{ (10  > 0 ? vnopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (int (*) (dev_t, int, int, struct proc *)		 ) nullop, (10  > 0 ? vnread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (10  > 0 ? vnwrite  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (10  > 0 ? vnioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, 0 } ,		 
	{ (7  > 0 ? stopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (7  > 0 ? stclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (7  > 0 ? rawread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (7  > 0 ? rawwrite  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (7  > 0 ? stioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) nullop, 0, seltrue, (int (*) ()		 ) enodev, (7  > 0 ? ststrategy  : (int (*) (struct buf *)		 ) enxio)  } ,		 
	{ (1 > 0 ? fdopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (int (*) (dev_t, int, int, struct proc *)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, struct uio *, int)		 ) enodev, (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enodev, (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) enodev, 0, (int (*) (dev_t, int, struct proc *)		 ) enodev, (int (*) ()		 ) enodev, 0 } ,		 
	{ (16  > 0 ? bpfopen  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (16  > 0 ? bpfclose  : (int (*) (dev_t, int, int, struct proc *)		 ) enxio) , (16  > 0 ? bpfread  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (16  > 0 ? bpfwrite  : (int (*) (dev_t, struct uio *, int)		 ) enxio) , (16  > 0 ? bpfioctl  : (int (*) (dev_t, int, caddr_t, int, struct proc *)		 ) enxio) , (int (*) (struct tty *, int)		 ) enodev, (int (*) (int)		 ) enodev, 0, (16  > 0 ? bpfselect  : (int (*) (dev_t, int, struct proc *)		 ) enxio) , (int (*) ()		 ) enodev, 0 } ,	 
};

int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

int	mem_no = 2; 	 

 








dev_t	swapdev = ((dev_t)(((3)<<8) | ( 0)))	;
