/*	PAO2 Id: if_cnwioctl.h,v 1.1.8.1 1998/12/05 22:47:11 itojun Exp */
struct cnwstatus {
	struct ifreq	ifr;
	u_char		data[0x100];
};

struct cnwstats {
	u_int	nws_rx;
	u_int	nws_rxerr;
	u_int	nws_rxoverflow;
	u_int	nws_rxoverrun;
	u_int	nws_rxcrcerror;
	u_int	nws_rxframe;
	u_int	nws_rxerrors;
	u_int	nws_rxavail;
	u_int	nws_rxone;
	u_int	nws_tx;
	u_int	nws_txokay;
	u_int	nws_txabort;
	u_int	nws_txlostcd;
	u_int	nws_txerrors;
	u_int	nws_txretries[16];
};

struct cnwistats {
	struct ifreq	ifr;
	struct cnwstats stats;
};

struct cnwtrail {
	u_char		what;
	u_char		status;
	u_short		length;
	struct timeval	when;
	struct timeval	done;
};

struct cnwitrail {
	struct ifreq	ifr;
	int		head;
	struct cnwtrail trail[128];
};

#define ifr_domain	ifr_ifru.ifru_flags     /* domain */
#define ifr_key		ifr_ifru.ifru_flags     /* scramble key */

#define SIOCSCNWDOMAIN	_IOW('i', 254, struct ifreq)	/* set domain */
#define SIOCGCNWDOMAIN	_IOWR('i', 253, struct ifreq)	/* get domain */
#define SIOCSCNWKEY	_IOWR('i', 252, struct ifreq)	/* set scramble key */
#define	SIOCGCNWSTATUS	_IOWR('i', 251, struct cnwstatus)/* get raw status */
#define	SIOCGCNWSTATS	_IOWR('i', 250, struct cnwistats)/* get stats */
#define	SIOCGCNWTRAIL	_IOWR('i', 249, struct cnwitrail)/* get trail */
