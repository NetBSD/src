/*	$NetBSD: misc.c,v 1.1.1.1.2.2 2009/05/13 18:50:36 jym Exp $	*/

/*****************************************************************
**
**	@(#) misc.c -- helper functions for the dnssec zone key tools
**
**	Copyright (c) Jan 2005, Holger Zuleger HZnet. All rights reserved.
**
**	This software is open source.
**
**	Redistribution and use in source and binary forms, with or without
**	modification, are permitted provided that the following conditions
**	are met:
**
**	Redistributions of source code must retain the above copyright notice,
**	this list of conditions and the following disclaimer.
**
**	Redistributions in binary form must reproduce the above copyright notice,
**	this list of conditions and the following disclaimer in the documentation
**	and/or other materials provided with the distribution.
**
**	Neither the name of Holger Zuleger HZnet nor the names of its contributors may
**	be used to endorse or promote products derived from this software without
**	specific prior written permission.
**
**	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
**	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
**	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
**	PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
**	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
**	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
**	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
**	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
**	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
**	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
**	POSSIBILITY OF SUCH DAMAGE.
**
*****************************************************************/
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <unistd.h>	/* for link(), unlink() */
# include <ctype.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <time.h>
# include <utime.h>
# include <assert.h>
# include <errno.h>
# include <fcntl.h>
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
# include "config_zkt.h"
# include "zconf.h"
# include "log.h"
# include "debug.h"
#define extern
# include "misc.h"
#undef extern

# define	TAINTEDCHARS	"`$@;&<>|"

extern	const	char	*progname;

static	int	inc_soa_serial (FILE *fp, int use_unixtime);
static	int	is_soa_rr (const char *line);
static	const	char	*strfindstr (const char *str, const char *search);

/*****************************************************************
**	getnameappendix (progname, basename)
**	return a pointer to the substring in progname subsequent
**	following basename "-".
*****************************************************************/
const	char	*getnameappendix (const char *progname, const char *basename)
{
	const	char	*p;
	int	baselen;

	assert (progname != NULL);
	assert (basename != NULL);

	if ( (p = strrchr (progname, '/')) != NULL )
		p++;
	else
		p = progname;

	baselen = strlen (basename);
	if ( strncmp (p, basename, baselen-1) == 0 && *(p+baselen) == '-' )
	{
		p += baselen + 1;
		if ( *p )
			return p;
	}

	return NULL;
}

/*****************************************************************
**	getdefconfname (view)
**	returns a pointer to a dynamic string containing the
**	default configuration file name
*****************************************************************/
const	char	*getdefconfname (const char *view)
{
	char	*p;
	char	*file;
	char	*buf;
	int	size;
	
	if ( (file = getenv ("ZKT_CONFFILE")) == NULL )
		file = CONFIG_FILE;
	dbg_val2 ("getdefconfname (%s) file = %s\n", view ? view : "NULL", file);

	if ( view == NULL || *view == '\0' || (p = strrchr (file, '.')) == NULL )
		return strdup (file);

	size = strlen (file) + strlen (view) + 1 + 1;
	if ( (buf = malloc (size)) == NULL )
		return strdup (file);

	dbg_val1 ("0123456789o123456789o123456789\tsize=%d\n", size);
	dbg_val4 ("%.*s-%s%s\n", p - file, file, view, p);

	snprintf (buf, size, "%.*s-%s%s", p - file, file, view, p);
	return buf;	
}

#if 1
/*****************************************************************
**	domain_canonicdup (s)
**	returns NULL or a pointer to a dynamic string containing the
**	canonic (all lower case letters and ending with a '.')
**	domain name
*****************************************************************/
char	*domain_canonicdup (const char *s)
{
	char	*new;
	char	*p;
	int	len;
	int	add_dot;

	if ( s == NULL )
		return NULL;

	add_dot = 0;
	len = strlen (s);
	if ( len > 0 && s[len-1] != '.' )
		add_dot = len++;

	if ( (new = p = malloc (len + 1)) == NULL )
		return NULL;

	while ( *s )
		*p++ = tolower (*s++);
	if ( add_dot )
		*p++ = '.';
	*p = '\0';

	return new;
}
#else
/*****************************************************************
**	str_tolowerdup (s)
*****************************************************************/
char	*str_tolowerdup (const char *s)
{
	char	*new;
	char	*p;

	if ( s == NULL || (new = p = malloc (strlen (s) + 1)) == NULL )
		return NULL;

	while ( *s )
		*p++ = tolower (*s++);
	*p = '\0';

	return new;
}
#endif

/*****************************************************************
**	str_delspace (s)
**	Remove in string 's' all white space char 
*****************************************************************/
char	*str_delspace (char *s)
{
	char	*start;
	char	*p;

	if ( !s )	/* is there a string ? */
		return s;

	start = s;
	for ( p = s; *p; p++ )
		if ( !isspace (*p) )
			*s++ = *p;	/* copy each nonspace */

	*s = '\0';	/* terminate string */

	return start;
}

/*****************************************************************
**	in_strarr (str, arr, cnt)
**	check if string array 'arr' contains the string 'str'
**	return 1 if true or 'arr' or 'str' is empty, otherwise 0
*****************************************************************/
int	in_strarr (const char *str, char *const arr[], int cnt)
{
	if ( arr == NULL || cnt <= 0 )
		return 1;

	if ( str == NULL || *str == '\0' )
		return 0;

	while ( --cnt >= 0 )
		if ( strcmp (str, arr[cnt]) == 0 )
			return 1;

	return 0;
}

/*****************************************************************
**	str_untaint (s)
**	Remove in string 's' all TAINTED chars
*****************************************************************/
char	*str_untaint (char *str)
{
	char	*p;

	assert (str != NULL);

	for ( p = str; *p; p++ )
		if ( strchr (TAINTEDCHARS, *p) )
			*p = ' ';
	return str;
}

/*****************************************************************
**	str_chop (str, c)
**	delete all occurrences of char 'c' at the end of string 's'
*****************************************************************/
char	*str_chop (char *str, char c)
{
	int	len;

	assert (str != NULL);

	len = strlen (str) - 1;
	while ( len >= 0 && str[len] == c )
		str[len--] = '\0';

	return str;
}

/*****************************************************************
**	parseurl (url, &proto, &host, &port, &para )
**	parses the given url (e.g. "proto://host.with.domain:port/para")
**	and set the pointer variables to the corresponding part of the string.
*****************************************************************/
void	parseurl (char *url, char **proto, char **host, char **port, char **para)
{
	char	*start;
	char	*p;

	assert ( url != NULL );

	/* parse protocol */
	if ( (p = strchr (url, ':')) == NULL )	/* no protocol string given ? */
		p = url;
	else					/* looks like a protocol string */
		if ( p[1] == '/' && p[2] == '/' )	/* protocol string ? */
		{
			*p = '\0';
			p += 3;
			if ( proto )
				*proto = url;
		}
		else				/* no protocol string found ! */
			p = url;

	/* parse host */
	if ( *p == '[' )	/* ipv6 address as hostname ? */
	{
		for ( start = ++p; *p && *p != ']'; p++ )
			;
		if ( *p )
			*p++ = '\0';
	}
	else
		for ( start = p; *p && *p != ':' && *p != '/'; p++ )
			;
	if ( host )
		*host = start;

	/* parse port */
	if ( *p == ':' )
	{
		*p++ = '\0';
		for ( start = p; *p && isdigit (*p); p++ )
			;
		if ( *p )
			*p++ = '\0';
		if ( port )
			*port = start;
	}

	if ( *p == '/' )
		*p++ = '\0';

	if ( *p && para )
		*para = p;
}

/*****************************************************************
**	splitpath (path, size, filename)
*****************************************************************/
const	char	*splitpath (char *path, size_t size, const char *filename)
{
	char 	*p;

	if ( !path )
		return filename;

	*path = '\0';
	if ( !filename )
		return filename;

	if ( (p = strrchr (filename, '/')) )	/* file arg contains path ? */
	{
		if ( strlen (filename) > size )
			return filename;

		strcpy (path, filename);
		path[p-filename] = '\0';
		filename = ++p;
	}
	return filename;
}

/*****************************************************************
**	pathname (path, size, dir, file, ext)
**	Concatenate 'dir', 'file' and 'ext' (if not null) to build
**	a pathname, and store the result in the character array
**	with length 'size' pointed to by 'path'.
*****************************************************************/
char	*pathname (char *path, size_t size, const char *dir, const char *file, const char *ext)
{
	int	len;

	if ( path == NULL || file == NULL )
		return path;

	len = strlen (file) + 1;
	if ( dir )
		len += strlen (dir);
	if ( ext )
		len += strlen (ext);
	if ( len > size )
		return path;

	*path = '\0';
	if ( dir && *dir )
	{
		len = sprintf (path, "%s", dir);
		if ( path[len-1] != '/' )
		{
			path[len++] = '/';
			path[len] = '\0';
		}
	}
	strcat (path, file);
	if ( ext )
		strcat (path, ext);
	return path;
}

/*****************************************************************
**	is_directory (name)
**	Check if the given pathname 'name' exists and is a directory.
**	returns 0 | 1
*****************************************************************/
int	is_directory (const char *name)
{
	struct	stat	st;

	if ( !name || !*name )	
		return 0;
	
	return ( stat (name, &st) == 0 && S_ISDIR (st.st_mode) );
}

/*****************************************************************
**	fileexist (name)
**	Check if a file with the given pathname 'name' exists.
**	returns 0 | 1
*****************************************************************/
int	fileexist (const char *name)
{
	struct	stat	st;
	return ( stat (name, &st) == 0 && S_ISREG (st.st_mode) );
}

/*****************************************************************
**	filesize (name)
**	return the size of the file with the given pathname 'name'.
**	returns -1 if the file not exist 
*****************************************************************/
size_t	filesize (const char *name)
{
	struct	stat	st;
	if  ( stat (name, &st) == -1 )
		return -1L;
	return ( st.st_size );
}

/*****************************************************************
**	is_keyfilename (name)
**	Check if the given name looks like a dnssec (public)
**	keyfile name. Returns 0 | 1
*****************************************************************/
int	is_keyfilename (const char *name)
{
	int	len;

	if ( name == NULL || *name != 'K' )
		return 0;

	len = strlen (name);
	if ( len > 4 && strcmp (&name[len - 4], ".key") == 0 ) 
		return 1;

	return 0;
}

/*****************************************************************
**	is_dotfile (name)
**	Check if the given pathname 'name' looks like "." or "..".
**	Returns 0 | 1
*****************************************************************/
int	is_dotfile (const char *name)
{
	if ( name && (
	     (name[0] == '.' && name[1] == '\0') || 
	     (name[0] == '.' && name[1] == '.' && name[2] == '\0')) )
		return 1;

	return 0;
}

/*****************************************************************
**	touch (name, sec)
**	Set the modification time of the given pathname 'fname' to
**	'sec'.	Returns 0 on success.
*****************************************************************/
int	touch (const char *fname, time_t sec)
{
	struct	utimbuf	utb;

	utb.actime = utb.modtime = sec;
	return utime (fname, &utb);
}

/*****************************************************************
**	linkfile (fromfile, tofile)
*****************************************************************/
int	linkfile (const char *fromfile, const char *tofile)
{
	int	ret;

	/* fprintf (stderr, "linkfile (%s, %s)\n", fromfile, tofile); */
	if ( (ret = link (fromfile, tofile)) == -1 && errno == EEXIST )
		if ( unlink (tofile) == 0 )
			ret = link (fromfile, tofile);

	return ret;
}

/*****************************************************************
**	copyfile (fromfile, tofile, dnskeyfile)
*****************************************************************/
int	copyfile (const char *fromfile, const char *tofile, const char *dnskeyfile)
{
	FILE	*infp;
	FILE	*outfp;
	int	c;

	/* fprintf (stderr, "copyfile (%s, %s)\n", fromfile, tofile); */
	if ( (infp = fopen (fromfile, "r")) == NULL )
		return -1;
	if ( (outfp = fopen (tofile, "w")) == NULL )
	{
		fclose (infp);
		return -2;
	}
	while ( (c = getc (infp)) != EOF ) 
		putc (c, outfp);

	fclose (infp);
	if ( dnskeyfile && *dnskeyfile && (infp = fopen (dnskeyfile, "r")) != NULL )
	{
		while ( (c = getc (infp)) != EOF ) 
			putc (c, outfp);
		fclose (infp);
	}
	fclose (outfp);

	return 0;
}

/*****************************************************************
**	copyzonefile (fromfile, tofile, dnskeyfile)
**	copy a already signed zonefile and replace all zone DNSKEY
**	resource records by one "$INCLUDE dnskey.db" line
*****************************************************************/
int	copyzonefile (const char *fromfile, const char *tofile, const char *dnskeyfile)
{
	FILE	*infp;
	FILE	*outfp;
	int	len;
	int	dnskeys;
	int	multi_line_dnskey;
	int	bufoverflow;
	char	buf[1024];
	char	*p;

	if ( fromfile == NULL )
		infp = stdin;
	else
		if ( (infp = fopen (fromfile, "r")) == NULL )
			return -1;
	if ( tofile == NULL )
		outfp = stdout;
	else
		if ( (outfp = fopen (tofile, "w")) == NULL )
		{
			if ( fromfile )
				fclose (infp);
			return -2;
		}

	multi_line_dnskey = 0;
	dnskeys = 0;
	bufoverflow = 0;
	while ( fgets (buf, sizeof buf, infp) != NULL ) 
	{
		p = buf;
		if ( !bufoverflow && !multi_line_dnskey && (*p == '@' || isspace (*p)) )	/* check if DNSKEY RR */
		{
			do
				p++;
			while ( isspace (*p) ) ;

			/* skip TTL */
			while ( isdigit (*p) )
				p++;

			while ( isspace (*p) )
				p++;

			/* skip Class */
			if ( strncasecmp (p, "IN", 2) == 0 )
			{
				p += 2;
				while ( isspace (*p) )
					p++;
			}

			if ( strncasecmp (p, "DNSKEY", 6) == 0 )	/* bingo! */
			{
				dnskeys++;
				p += 6;
				while ( *p )
				{
					if ( *p == '(' )
						multi_line_dnskey = 1;
					if ( *p == ')' )
						multi_line_dnskey = 0;
					p++;
				}
				if ( dnskeys == 1 )
					fprintf (outfp, "$INCLUDE %s\n", dnskeyfile);	
			}
			else 
				fputs (buf, outfp);	
		}
		else
		{
			if ( bufoverflow )
				fprintf (stderr, "!! buffer overflow in copyzonefile() !!\n");
			if ( !multi_line_dnskey )
				fputs (buf, outfp);	
			else
			{
				while ( *p && *p != ')' )
					p++;
				if ( *p == ')' )
					multi_line_dnskey = 0;
			}
		}
		
		len = strlen (buf);
		bufoverflow = buf[len-1] != '\n';	/* line too long ? */
	}

	if ( fromfile )
		fclose (infp);
	if ( tofile )
		fclose (outfp);

	return 0;
}

/*****************************************************************
**	cmpfile (file1, file2)
**	returns -1 on error, 1 if the files differ and 0 if they
**	are identical.
*****************************************************************/
int	cmpfile (const char *file1, const char *file2)
{
	FILE	*fp1;
	FILE	*fp2;
	int	c1;
	int	c2;

	/* fprintf (stderr, "cmpfile (%s, %s)\n", file1, file2); */
	if ( (fp1 = fopen (file1, "r")) == NULL )
		return -1;
	if ( (fp2 = fopen (file2, "r")) == NULL )
	{
		fclose (fp1);
		return -1;
	}

	do {
		c1 = getc (fp1);
		c2 = getc (fp2);
	}  while ( c1 != EOF && c2 != EOF && c1 == c2 );

	fclose (fp1);
	fclose (fp2);

	if ( c1 == c2 )
		return 0;
	return 1;
}

/*****************************************************************
**	file_age (fname)
*****************************************************************/
int	file_age (const char *fname)
{
	time_t	curr = time (NULL);
	time_t	mtime = file_mtime (fname);

	return curr - mtime;
}

/*****************************************************************
**	file_mtime (fname)
*****************************************************************/
time_t	file_mtime (const char *fname)
{
	struct	stat	st;

	if ( stat (fname, &st) < 0 )
		return 0;
	return st.st_mtime;
}

/*****************************************************************
**	is_exec_ok (prog)
**	Check if we are running as root or if the file owner of
**	"prog" do not match the current user or the file permissions
**	allows file modification for others then the owner.
**	The same condition will be checked for the group ownership.
**	return 1 if the execution of the command "prog" will not
**	open a big security whole, 0 otherwise
*****************************************************************/
int	is_exec_ok (const char *prog)
{
	uid_t	curr_uid;
	struct	stat	st;

	if ( stat (prog, &st) < 0 )
		return 0;

	curr_uid = getuid ();
	if ( curr_uid == 0 )			/* don't run the cmd if we are root */
		return 0;

	/* if the file owner and the current user matches and */
	/* the file mode is not writable except for the owner, we are save */
	if ( curr_uid == st.st_uid && (st.st_mode & (S_IWGRP | S_IWOTH)) == 0 )
		return 1;

	/* if the file group and the current group matches and */
	/* the file mode is not writable except for the group, we are also save */
	if ( getgid() != st.st_gid && (st.st_mode & (S_IWUSR | S_IWOTH)) == 0 )
		return 1;

	return 0;
}

/*****************************************************************
**	fatal (fmt, ...)
*****************************************************************/
void fatal (char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        if ( progname )
		fprintf (stderr, "%s: ", progname);
        vfprintf (stderr, fmt, ap);
        va_end(ap);
        exit (127);
}

/*****************************************************************
**	error (fmt, ...)
*****************************************************************/
void error (char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        vfprintf (stderr, fmt, ap);
        va_end(ap);
}

/*****************************************************************
**	logmesg (fmt, ...)
*****************************************************************/
void logmesg (char *fmt, ...)
{
        va_list ap;

#if defined (LOG_WITH_PROGNAME) && LOG_WITH_PROGNAME
        fprintf (stdout, "%s: ", progname);
#endif
        va_start(ap, fmt);
        vfprintf (stdout, fmt, ap);
        va_end(ap);
}

/*****************************************************************
**	verbmesg (verblvl, conf, fmt, ...)
*****************************************************************/
void	verbmesg (int verblvl, const zconf_t *conf, char *fmt, ...)
{
	char	str[511+1];
        va_list ap;

	str[0] = '\0';
	va_start(ap, fmt);
	vsnprintf (str, sizeof (str), fmt, ap);
	va_end(ap);

	//fprintf (stderr, "verbmesg (%d stdout=%d filelog=%d str = :%s:\n", verblvl, conf->verbosity, conf->verboselog, str);
	if ( verblvl <= conf->verbosity )	/* check if we have to print this to stdout */
		logmesg (str);

	str_chop (str, '\n');
	if ( verblvl <= conf->verboselog )	/* check logging to syslog and/or file */
		lg_mesg (LG_DEBUG, str);
}


/*****************************************************************
**	logflush ()
*****************************************************************/
void logflush ()
{
        fflush (stdout);
}

/*****************************************************************
**	timestr2time (timestr)
**	timestr should look like "20071211223901" for 12 dec 2007 22:39:01
*****************************************************************/
time_t	timestr2time (const char *timestr)
{
	struct	tm	t;
	time_t	sec;

	// fprintf (stderr, "timestr = \"%s\"\n", timestr);
	if ( sscanf (timestr, "%4d%2d%2d%2d%2d%2d", 
			&t.tm_year, &t.tm_mon, &t.tm_mday, 
			&t.tm_hour, &t.tm_min, &t.tm_sec) != 6 )
		return 0L;
	t.tm_year -= 1900;
	t.tm_mon -= 1;
	t.tm_isdst = 0;

#if defined(HAS_TIMEGM) && HAS_TIMEGM
	sec = timegm (&t);
#else
	{
	time_t ret;
	char *tz;

	tz = getenv("TZ");
	// setenv("TZ", "", 1);
	setenv("TZ", "UTC", 1);
	tzset();
	sec = mktime(&t);
	if (tz)
		setenv("TZ", tz, 1);
	else
		unsetenv("TZ");
	tzset();
	}
#endif
	
	return sec < 0L ? 0L : sec;
}

/*****************************************************************
**	time2str (sec, precison)
**	sec is seconds since 1.1.1970
**	precison is currently either 's' (for seconds) or 'm' (minutes)
*****************************************************************/
char	*time2str (time_t sec, int precision)
{
	struct	tm	*t;
	static	char	timestr[31+1];	/* 27+1 should be enough */
#if defined(HAVE_STRFTIME) && HAVE_STRFTIME
	char	tformat[127+1];

	timestr[0] = '\0';
	if ( sec <= 0L )
		return timestr;
	t = localtime (&sec);
	if ( precision == 's' )
		strcpy (tformat, "%b %d %Y %T");
	else
		strcpy (tformat, "%b %d %Y %R");
# if PRINT_TIMEZONE
	strcat (tformat, " %z");
# endif
	strftime (timestr, sizeof (timestr), tformat, t);

#else	/* no strftime available */
	static	char	*mstr[] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	timestr[0] = '\0';
	if ( sec <= 0L )
		return timestr;
	t = localtime (&sec);
# if PRINT_TIMEZONE
	{
	int	h,	s;

	s = abs (t->tm_gmtoff);
	h = t->tm_gmtoff / 3600;
	s = t->tm_gmtoff % 3600;
	if ( precision == 's' )
		snprintf (timestr, sizeof (timestr), "%s %2d %4d %02d:%02d:%02d %c%02d%02d",
			mstr[t->tm_mon], t->tm_mday, t->tm_year + 1900, 
			t->tm_hour, t->tm_min, t->tm_sec,
			t->tm_gmtoff < 0 ? '-': '+',
			h, s);
	else
		snprintf (timestr, sizeof (timestr), "%s %2d %4d %02d:%02d %c%02d%02d",
			mstr[t->tm_mon], t->tm_mday, t->tm_year + 1900, 
			t->tm_hour, t->tm_min, 
			t->tm_gmtoff < 0 ? '-': '+',
			h, s);
	}
# else
	if ( precision == 's' )
		snprintf (timestr, sizeof (timestr), "%s %2d %4d %02d:%02d:%02d",
			mstr[t->tm_mon], t->tm_mday, t->tm_year + 1900, 
			t->tm_hour, t->tm_min, t->tm_sec);
	else
		snprintf (timestr, sizeof (timestr), "%s %2d %4d %02d:%02d",
			mstr[t->tm_mon], t->tm_mday, t->tm_year + 1900, 
			t->tm_hour, t->tm_min);
# endif
#endif

	return timestr;
}

/*****************************************************************
**	time2isostr (sec, precison)
**	sec is seconds since 1.1.1970
**	precison is currently either 's' (for seconds) or 'm' (minutes)
*****************************************************************/
char	*time2isostr (time_t sec, int precision)
{
	struct	tm	*t;
	static	char	timestr[31+1];	/* 27+1 should be enough */

	timestr[0] = '\0';
	if ( sec <= 0L )
		return timestr;

	t = gmtime (&sec);
	if ( precision == 's' )
		snprintf (timestr, sizeof (timestr), "%4d%02d%02d%02d%02d%02d",
			t->tm_year + 1900, t->tm_mon+1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
	else
		snprintf (timestr, sizeof (timestr), "%4d%02d%02d%02d%02d",
			t->tm_year + 1900, t->tm_mon+1, t->tm_mday,
			t->tm_hour, t->tm_min);

	return timestr;
}

/*****************************************************************
**	age2str (sec)
**	!!Attention: This function is not reentrant 
*****************************************************************/
char	*age2str (time_t sec)
{
	static	char	str[20+1];	/* "2y51w6d23h50m55s" == 16+1 chars */
	int	len;
	int	strsize = sizeof (str);

	len = 0;
# if PRINT_AGE_WITH_YEAR
	if ( sec / (YEARSEC) > 0 )
	{
		len += snprintf (str+len, strsize - len, "%1luy", sec / YEARSEC );
		sec %= (YEARSEC);
	}
	else
		len += snprintf (str+len, strsize - len, "  ");
# endif
	if ( sec / WEEKSEC > 0 )
	{
		len += snprintf (str+len, strsize - len, "%2luw", (ulong) sec / WEEKSEC );
		sec %= WEEKSEC;
	}
	else
		len += snprintf (str+len, strsize - len, "   ");
	if ( sec / DAYSEC > 0 )
	{
		len += snprintf (str+len, strsize - len, "%2lud", sec / (ulong)DAYSEC);
		sec %= DAYSEC;
	}
	else
		len += snprintf (str+len, strsize - len, "   ");
	if ( sec / HOURSEC > 0 )
	{
		len += snprintf (str+len, strsize - len, "%2luh", sec / (ulong)HOURSEC);
		sec %= HOURSEC;
	}
	else
		len += snprintf (str+len, strsize - len, "   ");
	if ( sec / MINSEC > 0 )
	{
		len += snprintf (str+len, strsize - len, "%2lum", sec / (ulong)MINSEC);
		sec %= MINSEC;
	}
	else
		len += snprintf (str+len, strsize - len, "   ");
	if ( sec > 0 )
		snprintf (str+len, strsize - len, "%2lus", (ulong) sec);
	else
		len += snprintf (str+len, strsize - len, "   ");

	return str;
}

/*****************************************************************
**	start_timer ()
*****************************************************************/
time_t	start_timer ()
{
	return (time(NULL));
}

/*****************************************************************
**	stop_timer ()
*****************************************************************/
time_t	stop_timer (time_t start)
{
	time_t	stop = time (NULL);

	return stop - start;
}

/****************************************************************
**
**	int	inc_serial (filename, use_unixtime)
**
**	This function depends on a special syntax formating the
**	SOA record in the zone file!!
**
**	To match the SOA record, the SOA RR must be formatted
**	like this:
**	@ [ttl]   IN  SOA <master.fq.dn.> <hostmaster.fq.dn.> (
**	<SPACEes or TABs>      1234567890; serial number 
**	<SPACEes or TABs>      86400	 ; other values
**				...
**	The space from the first digit of the serial number to
**	the first none white space char or to the end of the line
**	must be at least 10 characters!
**	So you have to left justify the serial number in a field
**	of at least 10 characters like this:
**	<SPACEes or TABs>      1         ; Serial 
**
****************************************************************/
int	inc_serial (const char *fname, int use_unixtime)
{
	FILE	*fp;
	char	buf[4095+1];
	int	error;

	/**
	   since BIND 9.4, there is a dnssec-signzone option available for
	   serial number increment.
	   If the user request "unixtime" than use this mechanism 
	**/
#if defined(BIND_VERSION) && BIND_VERSION >= 940
	if ( use_unixtime )
		return 0;
#endif
	if ( (fp = fopen (fname, "r+")) == NULL )
		return -1;

		/* read until the line matches the beginning of a soa record ... */
	while ( fgets (buf, sizeof buf, fp) && !is_soa_rr (buf) )
		;

	if ( feof (fp) )
	{
		fclose (fp);
		return -2;
	}

	error = inc_soa_serial (fp, use_unixtime);	/* .. inc soa serial no ... */

	if ( fclose (fp) != 0 )
		return -5;
	return error;
}

/*****************************************************************
**	check if line is the beginning of a SOA RR record, thus
**	containing the string "IN .* SOA" and ends with a '('
**	returns 1 if true
*****************************************************************/
static	int	is_soa_rr (const char *line)
{
	const	char	*p;

	assert ( line != NULL );

	if ( (p = strfindstr (line, "IN")) && strfindstr (p+2, "SOA") )	/* line contains "IN" and "SOA" */
	{
		p = line + strlen (line) - 1;
		while ( p > line && isspace (*p) )
			p--;
		if ( *p == '(' )	/* last character have to be a '(' to start a multi line record */
			return 1;
	}

	return 0;
}

/*****************************************************************
**	Find string 'search' in 'str' and ignore case in comparison.
**	returns the position of 'search' in 'str' or NULL if not found.
*****************************************************************/
static	const	char	*strfindstr (const char *str, const char *search)
{
	const	char	*p;
	int		c;

	assert ( str != NULL );
	assert ( search != NULL );

	c = tolower (*search);
	p = str;
	do {
		while ( *p && tolower (*p) != c )
			p++;
		if ( strncasecmp (p, search, strlen (search)) == 0 )
			return p;
		p++;
	} while ( *p );

	return NULL;
}

/*****************************************************************
**	return the serial number of the current day in the form
**	of YYYYmmdd00
*****************************************************************/
static	ulong	today_serialtime ()
{
	struct	tm	*t;
	ulong	serialtime;
	time_t	now;

	now =  time (NULL);
	t = gmtime (&now);
	serialtime = (t->tm_year + 1900) * 10000;
	serialtime += (t->tm_mon+1) * 100;
	serialtime += t->tm_mday;
	serialtime *= 100;

	return serialtime;
}

/*****************************************************************
**	inc_soa_serial (fp, use_unixtime)
**	increment the soa serial number of the file 'fp'
**	'fp' must be opened "r+"
*****************************************************************/
static	int	inc_soa_serial (FILE *fp, int use_unixtime)
{
	int	c;
	long	pos,	eos;
	ulong	serial;
	int	digits;
	ulong	today;

	/* move forward until any non ws reached */
	while ( (c = getc (fp)) != EOF && isspace (c) )
		;
	ungetc (c, fp);		/* push back the last char */

	pos = ftell (fp);	/* mark position */

	serial = 0L;	/* read in the current serial number */
	/* be aware of the trailing space in the format string !! */
	if ( fscanf (fp, "%lu ", &serial) != 1 )	/* try to get serial no */
		return -3;
	eos = ftell (fp);	/* mark first non digit/ws character pos */

	digits = eos - pos;
	if ( digits < 10 )	/* not enough space for serial no ? */
		return -4;

	if ( use_unixtime )
		today = time (NULL);
	else
	{
		today = today_serialtime ();	/* YYYYmmdd00 */
		if ( serial > 1970010100L && serial < today )	
			serial = today;			/* set to current time */
		serial++;			/* increment anyway */
	}

	fseek (fp, pos, SEEK_SET);	/* go back to the beginning */
	fprintf (fp, "%-*lu", digits, serial);	/* write as many chars as before */

	return 1;	/* yep! */
}

/*****************************************************************
**	return the error text of the inc_serial return coode
*****************************************************************/
const	char	*inc_errstr (int err)
{
	switch ( err )
	{
	case -1:	return "couldn't open zone file for modifying";
	case -2:	return "unexpected end of file";
	case -3:	return "no serial number found in zone file";
	case -4:	return "not enough space left for serialno";
	case -5:	return "error on closing zone file";
	}
	return "";
}

#ifdef SOA_TEST
const char *progname;
main (int argc, char *argv[])
{
	ulong	now;
	int	err;
	char	cmd[255];

	progname = *argv;

	now = today_serialtime ();
	printf ("now = %lu\n", now);

	if ( (err = inc_serial (argv[1], 0)) <= 0 )
	{
		error ("can't change serial errno=%d\n", err);
		exit (1);
	}

	snprintf (cmd, sizeof(cmd), "head -15 %s", argv[1]);
	system (cmd);
}
#endif

#ifdef COPYZONE_TEST
const char *progname;
main (int argc, char *argv[])
{
	progname = *argv;

	if ( copyzonefile (argv[1], NULL) < 0 )
		error ("can't copy zone file %s\n", argv[1]);
}
#endif

#ifdef URL_TEST
const char *progname;
main (int argc, char *argv[])
{
	char	*proto;
	char	*host;
	char	*port;
	char	*para;
	char	url[1024];

	progname = *argv;

	proto = host = port = para = NULL;

	if ( --argc <= 0 )
	{
		fprintf (stderr, "usage: url_test <url>\n");
		fprintf (stderr, "e.g.: url_test http://www.hznet.de:80/zkt\n");
		exit (1);
	}
	
	strcpy (url, argv[1]);
	parseurl (url, &proto, &host, &port, &para);

	if ( proto )
		printf ("proto: \"%s\"\n", proto);
	if ( host )
		printf ("host: \"%s\"\n", host);
	if ( port )
		printf ("port: \"%s\"\n", port);
	if ( para )
		printf ("para: \"%s\"\n", para);

}
#endif

