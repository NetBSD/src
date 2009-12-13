/*	$NetBSD: ntpq.h,v 1.1.1.1 2009/12/13 16:56:26 kardel Exp $	*/

/*
 * ntpq.h - definitions of interest to ntpq
 */
#include "ntp_fp.h"
#include "ntp.h"
#include "ntp_control.h"
#include "ntp_string.h"
#include "ntp_malloc.h"

/*
 * Maximum number of arguments
 */
#define	MAXARGS	4

/*
 * Flags for forming descriptors.
 */
/*
 * Flags for forming descriptors.
 */
#define	OPT		0x80	/* this argument is optional, or'd with type */

#define	NO		0x0
#define	NTP_STR		0x1	/* string argument */
#define	NTP_UINT	0x2	/* unsigned integer */
#define	NTP_INT		0x3	/* signed integer */
#define	NTP_ADD		0x4	/* IP network address */
#define IP_VERSION	0x5	/* IP version */

/*
 * Arguments are returned in a union
 */
typedef union {
	char *string;
	long ival;
	u_long uval;
	sockaddr_u netnum;
} arg_v;

/*
 * Structure for passing parsed command line
 */
struct parse {
	const char *keyword;
	arg_v argval[MAXARGS];
	int nargs;
};

/*
 * ntpdc includes a command parser which could charitably be called
 * crude.  The following structure is used to define the command
 * syntax.
 */
struct xcmd {
  const char *keyword;		/* command key word */
	void (*handler)	(struct parse *, FILE *);	/* command handler */
	u_char arg[MAXARGS];	/* descriptors for arguments */
  const char *desc[MAXARGS];	/* descriptions for arguments */
  const char *comment;
};

/*
 * Structure to hold association data
 */
struct association {
	u_short assid;
	u_short status;
};

#define	MAXASSOC	1024

/*
 * Structure for translation tables between text format
 * variable indices and text format.
 */
struct ctl_var {
	u_short code;
	u_short fmt;
	const char *text;
};

extern int	interactive;	/* are we prompting? */
extern int	old_rv;		/* use old rv behavior? --old-rv */

extern	void	asciize		(int, char *, FILE *);
extern	int	getnetnum	(const char *, sockaddr_u *, char *, int);
extern	void	sortassoc	(void);
extern	int	doquery		(int, int, int, int, char *, u_short *, int *, char **);
extern	char *	nntohost	(sockaddr_u *);
extern	int	decodets	(char *, l_fp *);
extern	int	decodeuint	(char *, u_long *);
extern	int	nextvar		(int *, char **, char **, char **);
extern	int	decodetime	(char *, l_fp *);
extern	void	printvars	(int, char *, int, int, int, FILE *);
extern	int	decodeint	(char *, long *);
extern	int	findvar		(char *, struct ctl_var *, int code);
