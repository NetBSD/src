/*	$NetBSD: filename.c,v 1.1.1.2 1997/04/22 13:45:16 mrg Exp $	*/

/*
 * Copyright (c) 1984,1985,1989,1994,1995,1996  Mark Nudelman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice in the documentation and/or other materials provided with 
 *    the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN 
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Routines to mess around with filenames (and files).
 * Much of this is very OS dependent.
 */

#include "less.h"
#if MSDOS_COMPILER
#include <dos.h>
#if MSDOS_COMPILER==WIN32C
#include <dir.h>
#endif
#endif
#ifdef _OSK
#include <rbf.h>
#ifndef _OSK_MWC32
#include <modes.h>
#endif
#endif

extern int force_open;
extern int secure;
extern IFILE curr_ifile;
extern IFILE old_ifile;

/*
 * Return a pathname that points to a specified file in a specified directory.
 * Return NULL if the file does not exist in the directory.
 */
	static char *
dirfile(dirname, filename)
	char *dirname;
	char *filename;
{
	char *pathname;
	int f;

	if (dirname == NULL || *dirname == '\0')
		return (NULL);
	/*
	 * Construct the full pathname.
	 */
	pathname = (char *) calloc(strlen(dirname) + strlen(filename) + 2, 
					sizeof(char));
	if (pathname == NULL)
		return (NULL);
	sprintf(pathname, "%s%s%s", dirname, PATHNAME_SEP, filename);
	/*
	 * Make sure the file exists.
	 */
	f = open(pathname, OPEN_READ);
	if (f < 0)
	{
		free(pathname);
		pathname = NULL;
	} else
	{
		close(f);
	}
	return (pathname);
}

/*
 * Return the full pathname of the given file in the "home directory".
 */
	public char *
homefile(filename)
	char *filename;
{
	register char *pathname;

	/*
	 * Try $HOME/filename.
	 */
	pathname = dirfile(lgetenv("HOME"), filename);
	if (pathname != NULL)
		return (pathname);
#if OS2
	/*
	 * Try $INIT/filename.
	 */
	pathname = dirfile(lgetenv("INIT"), filename);
	if (pathname != NULL)
		return (pathname);
#endif
#if MSDOS_COMPILER || OS2
	/*
	 * Look for the file anywhere on search path.
	 */
	pathname = (char *) calloc(_MAX_PATH, sizeof(char));
	_searchenv(filename, "PATH", pathname);
	if (*pathname != '\0')
		return (pathname);
	free(pathname);
#endif
	return (NULL);
}

/*
 * Expand a string, substituting any "%" with the current filename,
 * and any "#" with the previous filename.
 * But a string of N "%"s is just replaced with N-1 "%"s.
 * Likewise for a string of N "#"s.
 * {{ This is a lot of work just to support % and #. }}
 */
	public char *
fexpand(s)
	char *s;
{
	register char *fr, *to;
	register int n;
	register char *e;
	IFILE ifile;

#define	fchar_ifile(c) \
	((c) == '%' ? curr_ifile : \
	 (c) == '#' ? old_ifile : NULL_IFILE)

	/*
	 * Make one pass to see how big a buffer we 
	 * need to allocate for the expanded string.
	 */
	n = 0;
	for (fr = s;  *fr != '\0';  fr++)
	{
		switch (*fr)
		{
		case '%':
		case '#':
			if (fr > s && fr[-1] == *fr)
			{
				/*
				 * Second (or later) char in a string
				 * of identical chars.  Treat as normal.
				 */
				n++;
			} else if (fr[1] != *fr)
			{
				/*
				 * Single char (not repeated).  Treat specially.
				 */
				ifile = fchar_ifile(*fr);
				if (ifile == NULL_IFILE)
					return (save(s));
				n += strlen(get_filename(ifile));
			}
			/*
			 * Else it is the first char in a string of
			 * identical chars.  Just discard it.
			 */
			break;
		default:
			n++;
			break;
		}
	}

	e = (char *) ecalloc(n+1, sizeof(char));

	/*
	 * Now copy the string, expanding any "%" or "#".
	 */
	to = e;
	for (fr = s;  *fr != '\0';  fr++)
	{
		switch (*fr)
		{
		case '%':
		case '#':
			if (fr > s && fr[-1] == *fr)
			{
				*to++ = *fr;
			} else if (fr[1] != *fr)
			{
				ifile = fchar_ifile(*fr);
				strcpy(to, get_filename(ifile));
				to += strlen(to);
			}
			break;
		default:
			*to++ = *fr;
			break;
		}
	}
	*to = '\0';
	return (e);
}

#if TAB_COMPLETE_FILENAME

/*
 * Return a blank-separated list of filenames which "complete"
 * the given string.
 */
	public char *
fcomplete(s)
	char *s;
{
	char *fpat;

	if (secure)
		return (NULL);
	/*
	 * Complete the filename "s" by globbing "s*".
	 */
#if MSDOS_COMPILER
	/*
	 * But in DOS, we have to glob "s*.*".
	 * But if the final component of the filename already has
	 * a dot in it, just do "s*".  
	 * (Thus, "FILE" is globbed as "FILE*.*", 
	 *  but "FILE.A" is globbed as "FILE.A*").
	 */
	{
		char *slash;
		for (slash = s+strlen(s)-1;  slash > s;  slash--)
			if (*slash = *PATHNAME_SEP || *slash == '/')
				break;
		fpat = (char *) ecalloc(strlen(s)+4, sizeof(char));
		if (strchr(slash, '.') == NULL)
			sprintf(fpat, "%s*.*", s);
		else
			sprintf(fpat, "%s*", s);
	}
#else
	fpat = (char *) ecalloc(strlen(s)+2, sizeof(char));
	sprintf(fpat, "%s*", s);
#endif
	s = lglob(fpat);
	if (strcmp(s,fpat) == 0)
	{
		/*
		 * The filename didn't expand.
		 */
		free(s);
		s = NULL;
	}
	free(fpat);
	return (s);
}
#endif

/*
 * Try to determine if a file is "binary".
 * This is just a guess, and we need not try too hard to make it accurate.
 */
	public int
bin_file(f)
	int f;
{
	int i;
	int n;
	unsigned char data[64];

	if (!seekable(f))
		return (0);
	if (lseek(f, (off_t)0, 0) == BAD_LSEEK)
		return (0);
	n = read(f, data, sizeof(data));
	for (i = 0;  i < n;  i++)
		if (binary_char(data[i]))
			return (1);
	return (0);
}

/*
 * Try to determine the size of a file by seeking to the end.
 */
	static POSITION
seek_filesize(f)
	int f;
{
	off_t spos;

	spos = lseek(f, (off_t)0, 2);
	if (spos == BAD_LSEEK)
		return (NULL_POSITION);
	return ((POSITION) spos);
}

#if GLOB

FILE *popen();

/*
 * Read a string from a file.
 * Return a pointer to the string in memory.
 */
	static char *
readfd(fd)
	FILE *fd;
{
	int len;
	int ch;
	char *buf;
	char *p;
	
	/* 
	 * Make a guess about how many chars in the string
	 * and allocate a buffer to hold it.
	 */
	len = 100;
	buf = (char *) ecalloc(len, sizeof(char));
	for (p = buf;  ;  p++)
	{
		if ((ch = getc(fd)) == '\n' || ch == EOF)
			break;
		if (p - buf >= len-1)
		{
			/*
			 * The string is too big to fit in the buffer we have.
			 * Allocate a new buffer, twice as big.
			 */
			len *= 2;
			*p = '\0';
			p = (char *) ecalloc(len, sizeof(char));
			strcpy(p, buf);
			free(buf);
			buf = p;
			p = buf + strlen(buf);
		}
		*p = ch;
	}
	*p = '\0';
	return (buf);
}

/*
 * Execute a shell command.
 * Return a pointer to a pipe connected to the shell command's standard output.
 */
	static FILE *
shellcmd(cmd, s1, s2)
	char *cmd;
	char *s1;
	char *s2;
{
	char *scmd;
	char *shell;
	FILE *fd;
	int len;
	
	len = strlen(cmd) + 
		(s1 == NULL ? 0 : strlen(s1)) + 
		(s2 == NULL ? 0 : strlen(s2)) + 1;
	scmd = (char *) ecalloc(len, sizeof(char));
	sprintf(scmd, cmd, s1, s2);
#if HAVE_SHELL
	shell = lgetenv("SHELL");
	if (shell != NULL && *shell != '\0')
	{
		/*
		 * Read the output of <$SHELL -c "cmd">.
		 */
		char *scmd2;
		scmd2 = (char *) ecalloc(strlen(shell) + strlen(scmd) + 7,
					sizeof(char));
		sprintf(scmd2, "%s -c \"%s\"", shell, scmd);
		free(scmd);
		scmd = scmd2;
	}
#endif
	fd = popen(scmd, "r");
	free(scmd);
	return (fd);
}

/*
 * Expand a filename, doing any shell-level substitutions.
 */
	public char *
lglob(filename)
	char *filename;
{
	char *gfilename;

	filename = fexpand(filename);

	if (secure)
		return (filename);
#if OS2
{
	char **list;
	int cnt;
	int length;

	list = _fnexplode(filename);
	if (list == NULL)
		return (filename);
	length = 1; /* Room for trailing null byte */
	for (cnt = 0;  list[cnt] != NULL;  cnt++)
	  	length += strlen(list[cnt]) + 1;
	gfilename = (char *) ecalloc(length, sizeof(char));
	for (cnt = 0;  list[cnt] != NULL;  cnt++)
	{
		strcat(gfilename, list[cnt]);
	  	strcat(gfilename, " ");
	}
	_fnexplodefree(list);
}
#else
{
	FILE *fd;
	char *s;

	/*
	 * We get the shell to expand the filename for us by passing
	 * an "echo" command to the shell and reading its output.
	 */

	/*
	 * Certain characters will cause problems if passed to the shell,
	 * so we disallow them.
	 * {{ This presumes too much knowlege about the shell, but not
	 *    doing this can cause serious problems.  For example, do 
	 *    "!;TAB" when the first file in the dir is named "rm". }}
	 */
	for (s = filename;  *s != '\0';  s++)
	{
		if (*s == ';' || *s == '\'' || *s == '\"' || *s == '\\')
			return (filename);
	}

	fd = shellcmd("echo %s", filename, (char*)NULL);
	if (fd == NULL)
	{
		/*
		 * Cannot create the pipe.
		 * Just return the original (fexpanded) filename.
		 */
		return (filename);
	}
	gfilename = readfd(fd);
	pclose(fd);
	if (*gfilename == '\0')
	{
		free(gfilename);
		return (filename);
	}
	free(filename);
}
#endif
	return (gfilename);
}

/*
 * See if we should open a "replacement file" 
 * instead of the file we're about to open.
 */
	public char *
open_altfile(filename, pf, pfd)
	char *filename;
	int *pf;
	void **pfd;
{
	char *lessopen;
	char *gfilename;
	FILE *fd;
#if HAVE_FILENO
	int returnfd = 0;
#endif
	
	if (secure)
		return (NULL);
	ch_ungetchar(-1);
	if ((lessopen = lgetenv("LESSOPEN")) == NULL)
		return (NULL);
	if (strcmp(filename, "-") == 0)
		return (NULL);
	if (*lessopen == '|')
	{
		/*
		 * If LESSOPEN starts with a |, it indicates 
		 * a "pipe preprocessor".
		 */
#if HAVE_FILENO
		lessopen++;
		returnfd = 1;
#else
		error("LESSOPEN pipe is not supported", NULL_PARG);
		return (NULL);
#endif
	}
	fd = shellcmd(lessopen, filename, (char*)NULL);
	if (fd == NULL)
	{
		/*
		 * Cannot create the pipe.
		 */
		return (NULL);
	}
#if HAVE_FILENO
	if (returnfd)
	{
		int f;
		char c;

		/*
		 * Read one char to see if the pipe will produce any data.
		 * If it does, push the char back on the pipe.
		 */
		f = fileno(fd);
		if (read(f, &c, 1) != 1)
		{
			/*
			 * Pipe is empty.  This means there is no alt file.
			 */
			pclose(fd);
			return (NULL);
		}
		ch_ungetchar(c);
		*pfd = (void *) fd;
		*pf = f;
		return (save("-"));
	}
#endif
	gfilename = readfd(fd);
	pclose(fd);
	if (*gfilename == '\0')
		/*
		 * Pipe is empty.  This means there is no alt file.
		 */
		return (NULL);
	return (gfilename);
}

/*
 * Close a replacement file.
 */
	public void
close_altfile(altfilename, filename, pipefd)
	char *altfilename;
	char *filename;
	void *pipefd;
{
	char *lessclose;
	FILE *fd;
	
	if (secure)
		return;
	if (pipefd != NULL)
		pclose((FILE*) pipefd);
	if ((lessclose = lgetenv("LESSCLOSE")) == NULL)
	     	return;
	fd = shellcmd(lessclose, filename, altfilename);
	pclose(fd);
}
		
#else
#if MSDOS_COMPILER

/*
 * Define macros for the MS-DOS "find file" interfaces.
 */
#if MSDOS_COMPILER==WIN32C

#define	FIND_FIRST(filename,fndp)	findfirst(filename, fndp, ~0)
#define	FIND_NEXT(fndp)			findnext(fndp)
#define	FND_NAME			ff_name
#define	DECLARE_FIND(fnd,drive,dir,fname,ext) \
					struct ffblk fnd;	\
					char drive[MAXDRIVE];	\
					char dir[MAXDIR];	\
					char fname[MAXFILE];	\
					char ext[MAXEXT];	

#else

#define	FIND_FIRST(filename,fndp)	_dos_findfirst(filename, ~0, fndp)
#define	FIND_NEXT(fndp)			_dos_findnext(fndp)
#define	FND_NAME			name
#define	DECLARE_FIND(fnd,drive,dir,fname,ext) \
					struct find_t fnd;	\
					char drive[_MAX_DRIVE];	\
					char dir[_MAX_DIR];	\
					char fname[_MAX_FNAME];	\
					char ext[_MAX_EXT];	

#endif
	
	public char *
lglob(filename)
	char *filename;
{
	register char *gfilename;
	register char *p;
	register int len;
	register int n;
	DECLARE_FIND(fnd,drive,dir,fname,ext)
	
	filename = fexpand(filename);

	if (secure)
		return (filename);

	if (FIND_FIRST(filename, &fnd) != 0)
		return (filename);

	_splitpath(filename, drive, dir, fname, ext);
	len = 100;
	gfilename = (char *) ecalloc(len, sizeof(char));
	p = gfilename;
	do {
		n = strlen(drive) + strlen(dir) + strlen(fnd.FND_NAME);
		while (p - gfilename + n+2 >= len)
		{
			/*
			 * No room in current buffer.  Allocate a bigger one.
			 */
			len *= 2;
			*p = '\0';
			p = (char *) ecalloc(len, sizeof(char));
			strcpy(p, gfilename);
			free(gfilename);
			gfilename = p;
			p = gfilename + strlen(gfilename);
		}
		sprintf(p, "%s%s%s", drive, dir, fnd.FND_NAME);
		p += n;
		*p++ = ' ';
	} while (FIND_NEXT(&fnd) == 0);

	/*
	 * Overwrite the final trailing space with a null terminator.
	 */
	*--p = '\0';
	return (gfilename);
}

	public char *
open_altfile(filename)
	char *filename;
{
	return (NULL);
}

	public void
close_altfile(altfilename, filename)
	char *altfilename;
	char *filename;
{
}
		
#else

	public char *
lglob(filename)
	char *filename;
{
	return (fexpand(filename));
}

	
	public char *
open_altfile(filename)
	char *filename;
{
     	return (NULL);
}

	public void
close_altfile(altfilename, filename)
	char *altfilename;
	char *filename;
{
}
		
#endif
#endif


#if HAVE_STAT

#include <sys/stat.h>
#ifndef S_ISDIR
#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define	S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#endif

/*
 * Returns NULL if the file can be opened and
 * is an ordinary file, otherwise an error message
 * (if it cannot be opened or is a directory, etc.)
 */
	public char *
bad_file(filename)
	char *filename;
{
	register char *m;
	struct stat statbuf;

	if (stat(filename, &statbuf) < 0)
		return (errno_message(filename));

	if (force_open)
		return (NULL);

	if (S_ISDIR(statbuf.st_mode))
	{
		static char is_dir[] = " is a directory";
		m = (char *) ecalloc(strlen(filename) + sizeof(is_dir), 
			sizeof(char));
		strcpy(m, filename);
		strcat(m, is_dir);
		return (m);
	}
	if (!S_ISREG(statbuf.st_mode))
	{
		static char not_reg[] = " is not a regular file";
		m = (char *) ecalloc(strlen(filename) + sizeof(not_reg), 
			sizeof(char));
		strcpy(m, filename);
		strcat(m, not_reg);
		return (m);
	}

	return (NULL);
}

/*
 * Return the size of a file, as cheaply as possible.
 * In Unix, we can stat the file.
 */
	public POSITION
filesize(f)
	int f;
{
	struct stat statbuf;

	if (fstat(f, &statbuf) < 0)
		/*
		 * Can't stat; try seeking to the end.
		 */
		return (seek_filesize(f));

	return ((POSITION) statbuf.st_size);
}

#else
#ifdef _OSK

	public char *
bad_file(filename)
	char *filename;
{
	register int f;
	register char *m;

	if ((f = open(filename, S_IREAD | S_IFDIR)) >= 0)
	{
		static char is_dir[] = " is a directory";
		close(f);
		if (force_open)
			return (NULL);
		m = (char *) ecalloc(strlen(filename) + sizeof(is_dir),
			sizeof(char));
		strcpy(m, filename);
		strcat(m, is_dir);
		return (m);
	}
	if ((f = open(filename, S_IREAD)) < 0)
		return (errno_message(filename));
	close(f);
	return (NULL);
}

	public POSITION
filesize(f)
	int f;
{
	long size;

	if ((size = (long)_gs_size(f)) < 0)
		/*
		 * Can't stat; try seeking to the end.
		 */
		return (seek_filesize(f));

	return ((POSITION) size);
}

#else

/*
 * If we have no way to find out, just say the file is good.
 */
	public char *
bad_file(filename)
	char *filename;
{
	return (NULL);
}

/*
 * We can find the file size by seeking.
 */
	public POSITION
filesize(f)
	int f;
{
	return (seek_filesize(f));
}

#endif
#endif
