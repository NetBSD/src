/*	$NetBSD: io.c,v 1.28 2017/01/10 20:41:40 christos Exp $	*/

/*
 * io.c			 Larn is copyrighted 1986 by Noah Morgan.
 * 
 * Below are the functions in this file:
 * 
 * setupvt100() 	Subroutine to set up terminal in correct mode for game
 * clearvt100()  	Subroutine to clean up terminal when the game is over
 * ttgetch() 		Routine to read in one character from the terminal
 * scbr()			Function to set cbreak -echo for the terminal
 * sncbr()			Function to set -cbreak echo for the terminal
 * newgame() 		Subroutine to save the initial time and seed rnd()
 * 
 * FILE OUTPUT ROUTINES
 * 
 * lprintf(format,args . . .)	printf to the output buffer lprint(integer)
 * end binary integer to output buffer lwrite(buf,len)
 * rite a buffer to the output buffer lprcat(str)
 * ent string to output buffer
 * 
 * FILE OUTPUT MACROS (in header.h)
 * 
 * lprc(character)				put the character into the output
 * buffer
 * 
 * FILE INPUT ROUTINES
 * 
 * long lgetc()				read one character from input buffer
 * long larn_lrint()			read one integer from input buffer
 * lrfill(address,number)		put input bytes into a buffer char
 * *lgetw()				get a whitespace ended word from
 * input char *lgetl()				get a \n or EOF ended line
 * from input
 * 
 * FILE OPEN / CLOSE ROUTINES
 * 
 * lcreat(filename)			create a new file for write
 * lopen(filename)				open a file for read
 * lappend(filename)			open for append to an existing file
 * lrclose()					close the input file
 * lwclose()					close output file lflush()
 * lush the output buffer
 * 
 * Other Routines
 * 
 * cursor(x,y)					position cursor at [x,y]
 * cursors()					position cursor at [1,24]
 * (saves memory) cl_line(x,y)         		Clear line at [1,y] and leave
 * cursor at [x,y] cl_up(x,y)    				Clear screen
 * from [x,1] to current line. cl_dn(x,y)
 * lear screen from [1,y] to end of display. standout(str)
 * rint the string in standout mode. set_score_output()
 * alled when output should be literally printed. * ttputch(ch)
 * rint one character in decoded output buffer. * flush_buf()
 * lush buffer with decoded output. * init_term()
 * erminal initialization -- setup termcap info *	char *tmcapcnv(sd,ss)
 * outine to convert VT100 \33's to termcap format beep()
 * e to emit a beep if enabled (see no-beep in .larnopts)
 * 
 * Note: ** entries are available only in termcap mode.
 */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: io.c,v 1.28 2017/01/10 20:41:40 christos Exp $");
#endif /* not lint */

#include "header.h"
#include "extern.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <term.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#ifdef TERMIO
#include <termio.h>
#define sgttyb termio
#define stty(_a,_b) ioctl(_a,TCSETA,_b)
#define gtty(_a,_b) ioctl(_a,TCGETA,_b)
#endif
#ifdef TERMIOS
#include <termios.h>
#define sgttyb termios
#define stty(_a,_b) tcsetattr(_a,TCSADRAIN,_b)
#define gtty(_a,_b) tcgetattr(_a,_b)
#endif

#if defined(TERMIO) || defined(TERMIOS)
static int      rawflg = 0;
static char     saveeof, saveeol;
#define doraw(_a) \
	if(!rawflg) { \
		++rawflg; \
		saveeof = _a.c_cc[VMIN]; \
		saveeol = _a.c_cc[VTIME]; \
	} \
    	_a.c_cc[VMIN] = 1; \
	_a.c_cc[VTIME] = 1; \
	_a.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL)
#define unraw(_a) \
	_a.c_cc[VMIN] = saveeof; \
	_a.c_cc[VTIME] = saveeol; \
	_a.c_lflag |= ICANON|ECHO|ECHOE|ECHOK|ECHONL

#else	/* not TERMIO or TERMIOS */

#ifndef BSD
#define CBREAK RAW		/* V7 has no CBREAK */
#endif

#define doraw(_a) (_a.sg_flags |= CBREAK,_a.sg_flags &= ~ECHO)
#define unraw(_a) (_a.sg_flags &= ~CBREAK,_a.sg_flags |= ECHO)
#include <sgtty.h>
#endif	/* not TERMIO or TERMIOS */

#ifndef NOVARARGS	/* if we have varargs */
#include <stdarg.h>
#else	/* NOVARARGS */	/* if we don't have varargs */
typedef char   *va_list;
#define va_dcl int va_alist;
#define va_start(plist) plist = (char *) &va_alist
#define va_end(plist)
#define va_arg(plist,mode) ((mode *)(plist += sizeof(mode)))[-1]
#endif	/* NOVARARGS */

static int ttputch(int ch);
static void flush_buf(void);

#define LINBUFSIZE 128	/* size of the lgetw() and lgetl() buffer */
int             io_outfd; /* output file numbers */
int             io_infd; /* input file numbers */
static struct sgttyb ttx;/* storage for the tty modes */
static int      ipoint = MAXIBUF, iepoint = MAXIBUF;	/* input buffering
							 * pointers    */
static char     lgetwbuf[LINBUFSIZE];	/* get line (word) buffer */

/*
 *	setupvt100() Subroutine to set up terminal in correct mode for game
 *
 *	Attributes off, clear screen, set scrolling region, set tty mode
 */
void
setupvt100(void)
{
	clear();
	setscroll();
	scbr();			/* system("stty cbreak -echo"); */
}

/*
 *	clearvt100() 	Subroutine to clean up terminal when the game is over
 *
 *	Attributes off, clear screen, unset scrolling region, restore tty mode
 */
void
clearvt100(void)
{
	resetscroll();
	clear();
	sncbr();		/* system("stty -cbreak echo"); */
}

/*
 *	ttgetch() 	Routine to read in one character from the terminal
 */
int
ttgetch(void)
{
	char            byt;
#ifdef EXTRA
	c[BYTESIN]++;
#endif
	lflush();		/* be sure output buffer is flushed */
	read(0, &byt, 1);	/* get byte from terminal */
	return (byt);
}

/*
 *	scbr()		Function to set cbreak -echo for the terminal
 *
 *	like: system("stty cbreak -echo")
 */
void
scbr(void)
{
	gtty(0, &ttx);
	doraw(ttx);
	stty(0, &ttx);
}

/*
 *	sncbr()		Function to set -cbreak echo for the terminal
 *
 *	like: system("stty -cbreak echo")
 */
void
sncbr(void)
{
	gtty(0, &ttx);
	unraw(ttx);
	stty(0, &ttx);
}

/*
 *	newgame() 	Subroutine to save the initial time and seed rnd()
 */
void
newgame(void)
{
	long  *p, *pe;
	for (p = c, pe = c + 100; p < pe; *p++ = 0);
	time(&initialtime);
	seedrand(initialtime);
	srandom(initialtime);
	lcreat((char *) 0);	/* open buffering for output to terminal */
}

/*
 *	lprintf(format,args . . .)		printf to the output buffer
 *		char *format;
 *		??? args . . .
 *
 *	Enter with the format string in "format", as per printf() usage
 *		and any needed arguments following it
 *	Note: lprintf() only supports %s, %c and %d, with width modifier and left
 *		or right justification.
 *	No correct checking for output buffer overflow is done, but flushes
 *		are done beforehand if needed.
 *	Returns nothing of value.
 */
void
lprintf(const char *fmt, ...)
{
	va_list ap;
	char buf[BUFBIG/2];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (lpnt >= lpend)
		lflush();

	lprcat(buf);
}

/*
 *	lprint(long-integer)	send binary integer to output buffer
 *		long integer;
 *
 *		+---------+---------+---------+---------+
 *		|   high  |	    |	      |	  low	|
 *		|  order  |	    |	      |  order	|
 *		|   byte  |	    |	      |	  byte	|
 *		+---------+---------+---------+---------+
 *	        31  ---  24 23 --- 16 15 ---  8 7  ---   0
 *
 *	The save order is low order first, to high order (4 bytes total)
 *	and is written to be system independent.
 *	No checking for output buffer overflow is done, but flushes if needed!
 *	Returns nothing of value.
 */
void
lprint(long x)
{
	if (lpnt >= lpend)
		lflush();
	*lpnt++ = 255 & x;
	*lpnt++ = 255 & (x >> 8);
	*lpnt++ = 255 & (x >> 16);
	*lpnt++ = 255 & (x >> 24);
}

/*
 *	lwrite(buf,len)		write a buffer to the output buffer
 *		char *buf;
 *		int len;
 *
 *	Enter with the address and number of bytes to write out
 *	Returns nothing of value
 */
void
lwrite(char *buf, int len)
{
	char *s;
	u_char *t;
	int num2;

	if (len > 399) {	/* don't copy data if can just write it */
#ifdef EXTRA
		c[BYTESOUT] += len;
#endif

#ifndef VT100
		for (s = buf; len > 0; --len)
			lprc(*s++);
#else	/* VT100 */
		lflush();
		write(io_outfd, buf, len);
#endif	/* VT100 */
	} else
		while (len) {
			if (lpnt >= lpend)
				lflush();	/* if buffer is full flush it	 */
			num2 = lpbuf + BUFBIG - lpnt;	/* # bytes left in
							 * output buffer	 */
			if (num2 > len)
				num2 = len;
			t = lpnt;
			len -= num2;
			while (num2--)
				*t++ = *buf++;	/* copy in the bytes */
			lpnt = t;
		}
}

/*
 *	long lgetc()	Read one character from input buffer
 *
 *  Returns 0 if EOF, otherwise the character
 */
long 
lgetc(void)
{
	int    i;
	if (ipoint != iepoint)
		return (inbuffer[ipoint++]);
	if (iepoint != MAXIBUF)
		return (0);
	if ((i = read(io_infd, inbuffer, MAXIBUF)) <= 0) {
		if (i != 0)
			write(1, "error reading from input file\n", 30);
		iepoint = ipoint = 0;
		return (0);
	}
	ipoint = 1;
	iepoint = i;
	return (*inbuffer);
}

/*
 *	long lrint()	Read one integer from input buffer
 *
 *		+---------+---------+---------+---------+
 *		|   high  |	    |	      |	  low	|
 *		|  order  |	    |	      |  order	|
 *		|   byte  |	    |	      |	  byte	|
 *		+---------+---------+---------+---------+
 *	       31  ---  24 23 --- 16 15 ---  8 7  ---   0
 *
 *	The save order is low order first, to high order (4 bytes total)
 *	Returns the int read
 */
long 
larn_lrint(void)
{
	unsigned long i;
	i = 255 & lgetc();
	i |= (255 & lgetc()) << 8;
	i |= (255 & lgetc()) << 16;
	i |= (255 & lgetc()) << 24;
	return (i);
}

/*
 *	lrfill(address,number)		put input bytes into a buffer
 *		char *address;
 *		int number;
 *
 *	Reads "number" bytes into the buffer pointed to by "address".
 *	Returns nothing of value
 */
void
lrfill(char *adr, int num)
{
	u_char  *pnt;
	int    num2;

	while (num) {
		if (iepoint == ipoint) {
			if (num > 5) {	/* fast way */
				if (read(io_infd, adr, num) != num)
					write(2, "error reading from input file\n", 30);
				num = 0;
			} else {
				*adr++ = lgetc();
				--num;
			}
		} else {
			num2 = iepoint - ipoint;	/* # of bytes left in
							 * the buffer	 */
			if (num2 > num)
				num2 = num;
			pnt = inbuffer + ipoint;
			num -= num2;
			ipoint += num2;
			while (num2--)
				*adr++ = *pnt++;
		}
	}
}

/*
 *	char *lgetw()			Get a whitespace ended word from input
 *
 *	Returns pointer to a buffer that contains word.  If EOF, returns a NULL
 */
char *
lgetw(void)
{
	char  *lgp, cc;
	int    n = LINBUFSIZE, quote = 0;
	lgp = lgetwbuf;
	do
		cc = lgetc();
	while ((cc <= 32) && (cc > '\0'));	/* eat whitespace */
	for (;; --n, cc = lgetc()) {
		if ((cc == '\0') && (lgp == lgetwbuf))
			return (NULL);	/* EOF */
		if ((n <= 1) || ((cc <= 32) && (quote == 0))) {
			*lgp = '\0';
			return (lgetwbuf);
		}
		if (cc != '"')
			*lgp++ = cc;
		else
			quote ^= 1;
	}
}

/*
 *	char *lgetl()	Function to read in a line ended by newline or EOF
 *
 * Returns pointer to a buffer that contains the line.  If EOF, returns NULL
 */
char *
lgetl(void)
{
	int    i = LINBUFSIZE, ch;
	char  *str = lgetwbuf;
	for (;; --i) {
		if ((*str++ = ch = lgetc()) == '\0') {
			if (str == lgetwbuf + 1)
				return (NULL);	/* EOF */
	ot:		*str = '\0';
			return (lgetwbuf);	/* line ended by EOF */
		}
		if ((ch == '\n') || (i <= 1))
			goto ot;/* line ended by \n */
	}
}

/*
 *	lcreat(filename)			Create a new file for write
 *		char *filename;
 *
 *	lcreat((char*)0); means to the terminal
 *	Returns -1 if error, otherwise the file descriptor opened.
 */
int
lcreat(char *str)
{
	lflush();
	lpnt = lpbuf;
	lpend = lpbuf + BUFBIG;
	if (str == NULL)
		return (io_outfd = 1);
	if ((io_outfd = creat(str, 0644)) < 0) {
		io_outfd = 1;
		lprintf("error creating file <%s>: %s\n", str,
			strerror(errno));
		lflush();
		return (-1);
	}
	return (io_outfd);
}

/*
 *	lopen(filename)			Open a file for read
 *		char *filename;
 *
 *	lopen(0) means from the terminal
 *	Returns -1 if error, otherwise the file descriptor opened.
 */
int
lopen(char *str)
{
	ipoint = iepoint = MAXIBUF;
	if (str == NULL)
		return (io_infd = 0);
	if ((io_infd = open(str, O_RDONLY)) < 0) {
		lwclose();
		io_outfd = 1;
		lpnt = lpbuf;
		return (-1);
	}
	return (io_infd);
}

/*
 *	lappend(filename)		Open for append to an existing file
 *		char *filename;
 *
 *	lappend(0) means to the terminal
 *	Returns -1 if error, otherwise the file descriptor opened.
 */
int
lappend(char *str)
{
	lpnt = lpbuf;
	lpend = lpbuf + BUFBIG;
	if (str == NULL)
		return (io_outfd = 1);
	if ((io_outfd = open(str, 2)) < 0) {
		io_outfd = 1;
		return (-1);
	}
	lseek(io_outfd, 0, SEEK_END);	/* seek to end of file */
	return (io_outfd);
}

/*
 *	lrclose() close the input file
 *
 *	Returns nothing of value.
 */
void
lrclose(void)
{
	if (io_infd > 0) {
		close(io_infd);
		io_infd = 0;
	}
}

/*
 *	lwclose() close output file flushing if needed
 *
 *	Returns nothing of value.
 */
void
lwclose(void)
{
	lflush();
	if (io_outfd > 2) {
		close(io_outfd);
		io_outfd = 1;
	}
}

/*
 *	lprcat(string)	append a string to the output buffer
 *			    	avoids calls to lprintf (time consuming)
 */
void
lprcat(const char *str)
{
	u_char  *str2;
	if (lpnt >= lpend)
		lflush();
	str2 = lpnt;
	while ((*str2++ = *str++) != '\0')
		continue;
	lpnt = str2 - 1;
}

#ifdef VT100
/*
 *	cursor(x,y) 		Subroutine to set the cursor position
 *
 *	x and y are the cursor coordinates, and lpbuff is the output buffer where
 *	escape sequence will be placed.
 */
static char    *y_num[] = {
"\33[", "\33[", "\33[2", "\33[3", "\33[4", "\33[5", "\33[6",
"\33[7", "\33[8", "\33[9", "\33[10", "\33[11", "\33[12", "\33[13", "\33[14",
"\33[15", "\33[16", "\33[17", "\33[18", "\33[19", "\33[20", "\33[21", "\33[22",
"\33[23", "\33[24"};

static char    *x_num[] = {
"H", "H", ";2H", ";3H", ";4H", ";5H", ";6H", ";7H", ";8H", ";9H",
";10H", ";11H", ";12H", ";13H", ";14H", ";15H", ";16H", ";17H", ";18H", ";19H",
";20H", ";21H", ";22H", ";23H", ";24H", ";25H", ";26H", ";27H", ";28H", ";29H",
";30H", ";31H", ";32H", ";33H", ";34H", ";35H", ";36H", ";37H", ";38H", ";39H",
";40H", ";41H", ";42H", ";43H", ";44H", ";45H", ";46H", ";47H", ";48H", ";49H",
";50H", ";51H", ";52H", ";53H", ";54H", ";55H", ";56H", ";57H", ";58H", ";59H",
";60H", ";61H", ";62H", ";63H", ";64H", ";65H", ";66H", ";67H", ";68H", ";69H",
";70H", ";71H", ";72H", ";73H", ";74H", ";75H", ";76H", ";77H", ";78H", ";79H",
";80H"};

void
cursor(x, y)
	int             x, y;
{
	char  *p;
	if (lpnt >= lpend)
		lflush();

	p = y_num[y];		/* get the string to print */
	while (*p)
		*lpnt++ = *p++;	/* print the string */

	p = x_num[x];		/* get the string to print */
	while (*p)
		*lpnt++ = *p++;	/* print the string */
}
#else	/* VT100 */
/*
 * cursor(x,y)	  Put cursor at specified coordinates staring at [1,1] (termcap)
 */
void
cursor(int x, int y)
{
	if (lpnt >= lpend)
		lflush();

	*lpnt++ = CURSOR;
	*lpnt++ = x;
	*lpnt++ = y;
}
#endif	/* VT100 */

/*
 *	Routine to position cursor at beginning of 24th line
 */
void
cursors(void)
{
	cursor(1, 24);
}

#ifndef VT100
/*
 * Warning: ringing the bell is control code 7. Don't use in defines.
 * Don't change the order of these defines.
 * Also used in helpfiles. Codes used in helpfiles should be \E[1 to \E[7 with
 * obvious meanings.
 */

static char    *outbuf = 0;     /* translated output buffer */
/*
 * init_term()		Terminal initialization -- setup termcap info
 */
void
init_term(void)
{
	setupterm(NULL, 0, NULL); /* will exit if invalid term */
	if (!cursor_address) {
		fprintf(stderr, "term does not have cursor_address.\n");
		exit(1);
	}
	if (!clr_eol) {
		fprintf(stderr, "term does not have clr_eol.\n");
		exit(1);
	}
	if (!clear_screen) {
		fprintf(stderr, "term does not have clear_screen.\n");
		exit(1);
	}
	if ((outbuf = malloc(BUFBIG + 16)) == 0) {      /* get memory for
							 * decoded output buffer */
	    fprintf(stderr, "Error malloc'ing memory for decoded output buffer\n");
	    died(-285);     /* malloc() failure */
	}

}
#endif	/* VT100 */

/*
 * cl_line(x,y)  Clear the whole line indicated by 'y' and leave cursor at [x,y]
 */
void
cl_line(int x, int y)
{
#ifdef VT100
	cursor(x, y);
	lprcat("\33[2K");
#else	/* VT100 */
	cursor(1, y);
	*lpnt++ = CL_LINE;
	cursor(x, y);
#endif	/* VT100 */
}

/*
 * cl_up(x,y) Clear screen from [x,1] to current position. Leave cursor at [x,y]
 */
void
cl_up(int x, int y)
{
#ifdef VT100
	cursor(x, y);
	lprcat("\33[1J\33[2K");
#else	/* VT100 */
	int    i;
	cursor(1, 1);
	for (i = 1; i <= y; i++) {
		*lpnt++ = CL_LINE;
		*lpnt++ = '\n';
	}
	cursor(x, y);
#endif	/* VT100 */
}

/*
 * cl_dn(x,y) 	Clear screen from [1,y] to end of display. Leave cursor at [x,y]
 */
void
cl_dn(int x, int y)
{
#ifdef VT100
	cursor(x, y);
	lprcat("\33[J\33[2K");
#else	/* VT100 */
	int    i;
	cursor(1, y);
	if (!clr_eos) {
		*lpnt++ = CL_LINE;
		for (i = y; i <= 24; i++) {
			*lpnt++ = CL_LINE;
			if (i != 24)
				*lpnt++ = '\n';
		}
		cursor(x, y);
	} else
		*lpnt++ = CL_DOWN;
	cursor(x, y);
#endif	/* VT100 */
}

/*
 * standout(str)	Print the argument string in inverse video (standout mode).
 */
void
standout(const char *str)
{
#ifdef VT100
	setbold();
	while (*str)
		*lpnt++ = *str++;
	resetbold();
#else	/* VT100 */
	*lpnt++ = ST_START;
	while (*str)
		*lpnt++ = *str++;
	*lpnt++ = ST_END;
#endif	/* VT100 */
}

/*
 * set_score_output() 	Called when output should be literally printed.
 */
void
set_score_output(void)
{
	enable_scroll = -1;
}

/*
 *	lflush()	Flush the output buffer
 *
 *	Returns nothing of value.
 *	for termcap version: Flush output in output buffer according to output
 *	status as indicated by `enable_scroll'
 */
#ifndef VT100
static int      scrline = 18;	/* line # for wraparound instead of scrolling
				 * if no DL */
void
lflush(void)
{
	int    lpoint;
	u_char  *str;
	static int      curx = 0;
	static int      cury = 0;

	if ((lpoint = lpnt - lpbuf) > 0) {
#ifdef EXTRA
		c[BYTESOUT] += lpoint;
#endif
		if (enable_scroll <= -1) {
			flush_buf();
			if (write(io_outfd, lpbuf, lpoint) != lpoint)
				write(2, "error writing to output file\n", 29);
			lpnt = lpbuf;	/* point back to beginning of buffer */
			return;
		}
		for (str = lpbuf; str < lpnt; str++) {
			if (*str >= 32) {
				ttputch(*str);
				curx++;
			} else
				switch (*str) {
				case CLEAR:
					tputs(clear_screen, 0, ttputch);
					curx = cury = 0;
					break;

				case CL_LINE:
					tputs(clr_eol, 0, ttputch);
					break;

				case CL_DOWN:
					tputs(clr_eos, 0, ttputch);
					break;

				case ST_START:
					tputs(enter_standout_mode, 0, ttputch);
					break;

				case ST_END:
					tputs(exit_standout_mode, 0, ttputch);
					break;

				case CURSOR:
					curx = *++str - 1;
					cury = *++str - 1;
					tputs(tiparm(cursor_address,
						    cury, curx), 0, ttputch);
					break;

				case '\n':
					if ((cury == 23) && enable_scroll) {
						if (!delete_line ||
						    !insert_line)
						{	/* wraparound or scroll? */
							if (++scrline > 23)
								scrline = 19;

							if (++scrline > 23)
								scrline = 19;
							tputs(tiparm(
							    cursor_address,
							    scrline, 0),
							    0, ttputch);
							tputs(clr_eol, 0,
							    ttputch);

							if (--scrline < 19)
								scrline = 23;
							tputs(tiparm(
							    cursor_address,
							    scrline, 0),
							    0, ttputch);
							tputs(clr_eol, 0,
							    ttputch);
						} else {
							tputs(tiparm(
							    cursor_address,
							    19, 0),
							    0, ttputch);
							tputs(delete_line, 0,
							    ttputch);
							tputs(tiparm(
							    cursor_address,
							    23, 0),
							    0, ttputch);
							/*
							 * tputs (AL, 0,
							 * ttputch);
							 */
						}
					} else {
						ttputch('\n');
						cury++;
					}
					curx = 0;
					break;

				default:
					ttputch(*str);
					curx++;
				};
		}
	}
	lpnt = lpbuf;
	flush_buf();		/* flush real output buffer now */
}
#else	/* VT100 */
/*
 *	lflush()		flush the output buffer
 *
 *	Returns nothing of value.
 */
void
lflush()
{
	int    lpoint;
	if ((lpoint = lpnt - lpbuf) > 0) {
#ifdef EXTRA
		c[BYTESOUT] += lpoint;
#endif
		if (write(io_outfd, lpbuf, lpoint) != lpoint)
			write(2, "error writing to output file\n", 29);
	}
	lpnt = lpbuf;		/* point back to beginning of buffer */
}
#endif	/* VT100 */

#ifndef VT100
static int      vindex = 0;
/*
 * ttputch(ch)		Print one character in decoded output buffer.
 */
static int 
ttputch(int ch)
{
	outbuf[vindex++] = ch;
	if (vindex >= BUFBIG)
		flush_buf();
	return (0);
}

/*
 * flush_buf()			Flush buffer with decoded output.
 */
static void
flush_buf(void)
{
	if (vindex)
		write(io_outfd, outbuf, vindex);
	vindex = 0;
}

/*
 *	char *tmcapcnv(sd,ss)  Routine to convert VT100 escapes to termcap
 *	format
 *	Processes only the \33[#m sequence (converts . files for termcap use
 */
char *
tmcapcnv(char *sd, char *ss)
{
	int    tmstate = 0;	/* 0=normal, 1=\33 2=[ 3=# */
	char            tmdigit = 0;	/* the # in \33[#m */
	while (*ss) {
		switch (tmstate) {
		case 0:
			if (*ss == '\33') {
				tmstate++;
				break;
			}
	ign:		*sd++ = *ss;
	ign2:		tmstate = 0;
			break;
		case 1:
			if (*ss != '[')
				goto ign;
			tmstate++;
			break;
		case 2:
			if (isdigit((u_char)*ss)) {
				tmdigit = *ss - '0';
				tmstate++;
				break;
			}
			if (*ss == 'm') {
				*sd++ = ST_END;
				goto ign2;
			}
			goto ign;
		case 3:
			if (*ss == 'm') {
				if (tmdigit)
					*sd++ = ST_START;
				else
					*sd++ = ST_END;
				goto ign2;
			}
		default:
			goto ign;
		};
		ss++;
	}
	*sd = 0;		/* NULL terminator */
	return (sd);
}
#endif	/* VT100 */

/*
 *	beep()	Routine to emit a beep if enabled (see no-beep in .larnopts)
 */
void
beep(void)
{
	if (!nobeep)
		*lpnt++ = '\7';
}
