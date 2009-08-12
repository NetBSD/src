/*	$NetBSD: fortune.c,v 1.52 2009/08/12 06:06:28 dholland Exp $	*/

/*-
 * Copyright (c) 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ken Arnold.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1986, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)fortune.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: fortune.c,v 1.52 2009/08/12 06:06:28 dholland Exp $");
#endif
#endif /* not lint */

# include	<sys/param.h>
# include	<sys/stat.h>
# include	<sys/time.h>
# include	<sys/endian.h>

# include	<dirent.h>
# include	<fcntl.h>
# include	<assert.h>
# include	<unistd.h>
# include	<stdio.h>
# include	<ctype.h>
# include	<stdlib.h>
# include	<string.h>
# include	<err.h>
# include	<time.h>
# include	"strfile.h"
# include	"pathnames.h"

# define	TRUE	1
# define	FALSE	0
# define	bool	short

# define	MINW	6		/* minimum wait if desired */
# define	CPERS	20		/* # of chars for each sec */
# define	SLEN	160		/* # of chars in short fortune */

# define	POS_UNKNOWN	((off_t) -1)	/* pos for file unknown */
# define	NO_PROB		(-1)		/* no prob specified for file */

# ifdef DEBUG
# define	DPRINTF(l,x)	if (Debug >= l) fprintf x; else
# undef		NDEBUG
# else
# define	DPRINTF(l,x)
# define	NDEBUG	1
# endif

typedef struct fd {
	int		percent;
	int		fd, datfd;
	off_t		pos;
	FILE		*inf;
	const char	*name;
	const char	*path;
	char		*datfile, *posfile;
	bool		read_tbl;
	bool		was_pos_file;
	STRFILE		tbl;
	int		num_children;
	struct fd	*child, *parent;
	struct fd	*next, *prev;
} FILEDESC;

static bool Found_one;			/* did we find a match? */
static bool Find_files	= FALSE;	/* just find a list of proper fortune files */
static bool Wait	= FALSE;	/* wait desired after fortune */
static bool Short_only	= FALSE;	/* short fortune desired */
static bool Long_only	= FALSE;	/* long fortune desired */
static bool Offend	= FALSE;	/* offensive fortunes only */
static bool All_forts	= FALSE;	/* any fortune allowed */
static bool Equal_probs	= FALSE;	/* scatter un-allocted prob equally */
#ifndef NO_REGEX
static bool Match	= FALSE;	/* dump fortunes matching a pattern */
#endif
#ifdef DEBUG
static bool Debug = FALSE;		/* print debug messages */
#endif

static char *Fortbuf = NULL;		/* fortune buffer for -m */

static int Fort_len = 0;

static off_t Seekpts[2];		/* seek pointers to fortunes */

static FILEDESC *File_list = NULL,	/* Head of file list */
		*File_tail = NULL;	/* Tail of file list */
static FILEDESC *Fortfile;		/* Fortune file to use */

static STRFILE Noprob_tbl;		/* sum of data for all no prob files */

static int add_dir(FILEDESC *);
static int add_file(int,
	    const char *, const char *, FILEDESC **, FILEDESC **, FILEDESC *);
static void all_forts(FILEDESC *, const char *);
static char *copy(const char *, u_int);
static void rot13(char *line, int len);
static void display(FILEDESC *);
static void do_free(void *);
static void *do_malloc(u_int);
static int form_file_list(char **, int);
static int fortlen(void);
static void get_fort(void);
static void get_pos(FILEDESC *);
static void get_tbl(FILEDESC *);
static void getargs(int, char *[]);
static void init_prob(void);
static int is_dir(const char *);
static int is_fortfile(const char *, char **, char **, int);
static int is_off_name(const char *);
static int max(int, int);
static FILEDESC *new_fp(void);
static char *off_name(const char *);
static void open_dat(FILEDESC *);
static void open_fp(FILEDESC *);
static FILEDESC *pick_child(FILEDESC *);
static void print_file_list(void);
static void print_list(FILEDESC *, int);
static void sum_noprobs(FILEDESC *);
static void sum_tbl(STRFILE *, STRFILE *);
static void usage(void) __dead;
static void zero_tbl(STRFILE *);

int main(int, char *[]);

#ifndef	NO_REGEX
static char *conv_pat(char *);
static int find_matches(void);
static void matches_in_list(FILEDESC *);
static int maxlen_in_list(FILEDESC *);
#endif

#ifndef NO_REGEX
# if HAVE_REGCMP
#  define	RE_INIT(re)
#  define	RE_COMP(re, p)	((re) = regcmp((p), NULL))
#  define	RE_ERROR(re)	"Invalid pattern"
#  define	RE_OK(re)	((re) != NULL)
#  define	RE_EXEC(re, p)	regex((re), (p))
#  define	RE_FREE(re)

char	*Re_pat, *Re_pat13, *Re_use;
char	*regcmp(), *regex();

# elif HAVE_RE_COMP
char	*Re_pat, *Re_pat13, *Re_use;
char	*Re_error;

#  define	RE_INIT(re)
#  define	RE_COMP(re, p)	(Re_error = re_comp(p))
#  define	RE_ERROR(re)	Re_error
#  define	RE_OK(re)	(Re_error == NULL)
#  define	RE_EXEC(re, p)	re_exec(p)
#  define	RE_FREE(re)
# elif HAVE_REGCOMP
#  include <regex.h>
static regex_t *Re_pat = NULL, *Re_pat13 = NULL, *Re_use = NULL;
static int  Re_code;
static char Re_error[1024];
#  define	RE_INIT(re)	if ((re) == NULL && \
				    ((re) = calloc(sizeof(*(re)), 1)) \
				    == NULL) err(1, NULL)
#  define	RE_COMP(re, p)	(Re_code = regcomp((re), (p), REG_EXTENDED))
#  define	RE_OK(re)	(Re_code == 0)
#  define	RE_EXEC(re, p)	(!regexec((re), (p), 0, NULL, 0))
#  define	RE_ERROR(re)	(regerror(Re_code, (re), Re_error, \
				    sizeof(Re_error)), Re_error)
#  define	RE_FREE(re)	if ((re) != NULL) do { regfree((re)); \
				    (re) = NULL; } while (0)
# else
	#error "Need to define HAVE_REGCMP, HAVE_RE_COMP, or HAVE_REGCOMP"
# endif
#endif

#ifndef NAMLEN
#define		NAMLEN(d)	((d)->d_namlen)
#endif

int
main(ac, av)
	int	ac;
	char	*av[];
{
	struct timeval tv;
#ifdef	OK_TO_WRITE_DISK
	int	fd;
#endif	/* OK_TO_WRITE_DISK */

	getargs(ac, av);

#ifndef NO_REGEX
	if (Match)
		exit(find_matches() != 0);
#endif

	init_prob();
	if (gettimeofday(&tv, NULL) != 0)
		err(1, "gettimeofday()");
	srandom(((unsigned long)tv.tv_sec)    *
                ((unsigned long)tv.tv_usec+1) *
	        ((unsigned long)getpid()+1)   *
                ((unsigned long)getppid()+1));
	do {
		get_fort();
	} while ((Short_only && fortlen() > SLEN) ||
		 (Long_only && fortlen() <= SLEN));

	display(Fortfile);

#ifdef	OK_TO_WRITE_DISK
	if ((fd = creat(Fortfile->posfile, 0666)) < 0)
		err(1, "Can't create `%s'", Fortfile->posfile);
#ifdef	LOCK_EX
	/*
	 * if we can, we exclusive lock, but since it isn't very
	 * important, we just punt if we don't have easy locking
	 * available.
	 */
	(void) flock(fd, LOCK_EX);
#endif	/* LOCK_EX */
	write(fd, (char *) &Fortfile->pos, sizeof Fortfile->pos);
	if (!Fortfile->was_pos_file)
		(void) chmod(Fortfile->path, 0666);
#ifdef	LOCK_EX
	(void) flock(fd, LOCK_UN);
#endif	/* LOCK_EX */
#endif	/* OK_TO_WRITE_DISK */
	if (Wait) {
		if (Fort_len == 0)
			(void) fortlen();
		sleep((unsigned int) max(Fort_len / CPERS, MINW));
	}
	return(0);
}

static void
rot13(line, len)
	char *line;
	int len;
{
	char	*p, ch;

	if (len == 0)
		len = strlen(line);

	for (p = line; (ch = *p) != 0; ++p)
		if (isupper((unsigned char)ch))
			*p = 'A' + (ch - 'A' + 13) % 26;
		else if (islower((unsigned char)ch))
			*p = 'a' + (ch - 'a' + 13) % 26;
}

static void
display(fp)
	FILEDESC	*fp;
{
	char	line[BUFSIZ];

	open_fp(fp);
	(void) fseek(fp->inf, (long)Seekpts[0], SEEK_SET);
	for (Fort_len = 0; fgets(line, sizeof line, fp->inf) != NULL &&
	    !STR_ENDSTRING(line, fp->tbl); Fort_len++) {
		if (fp->tbl.str_flags & STR_ROTATED)
			rot13(line, 0);
		fputs(line, stdout);
	}
	(void) fflush(stdout);
}

/*
 * fortlen:
 *	Return the length of the fortune.
 */
static int
fortlen()
{
	int	nchar;
	char	line[BUFSIZ];

	if (!(Fortfile->tbl.str_flags & (STR_RANDOM | STR_ORDERED)))
		nchar = Seekpts[1] - Seekpts[0];
	else {
		open_fp(Fortfile);
		(void) fseek(Fortfile->inf, (long)Seekpts[0], SEEK_SET);
		nchar = 0;
		while (fgets(line, sizeof line, Fortfile->inf) != NULL &&
		       !STR_ENDSTRING(line, Fortfile->tbl))
			nchar += strlen(line);
	}
	Fort_len = nchar;
	return nchar;
}

/*
 *	This routine evaluates the arguments on the command line
 */
static void
getargs(argc, argv)
	int	argc;
	char	**argv;
{
	int	ignore_case;
# ifndef NO_REGEX
	char	*pat = NULL;
# endif	/* NO_REGEX */
	int ch;

	ignore_case = FALSE;

# ifdef DEBUG
	while ((ch = getopt(argc, argv, "aDefilm:osw")) != -1)
#else
	while ((ch = getopt(argc, argv, "aefilm:osw")) != -1)
#endif /* DEBUG */
		switch(ch) {
		case 'a':		/* any fortune */
			All_forts++;
			break;
# ifdef DEBUG
		case 'D':
			Debug++;
			break;
# endif /* DEBUG */
		case 'e':
			Equal_probs++;	/* scatter un-allocted prob equally */
			break;
		case 'f':		/* find fortune files */
			Find_files++;
			break;
		case 'l':		/* long ones only */
			Long_only++;
			Short_only = FALSE;
			break;
		case 'o':		/* offensive ones only */
			Offend++;
			break;
		case 's':		/* short ones only */
			Short_only++;
			Long_only = FALSE;
			break;
		case 'w':		/* give time to read */
			Wait++;
			break;
# ifdef	NO_REGEX
		case 'i':			/* case-insensitive match */
		case 'm':			/* dump out the fortunes */
			errx(1, "Can't match fortunes on this system (Sorry)");
# else	/* NO_REGEX */
		case 'm':			/* dump out the fortunes */
			Match++;
			pat = optarg;
			break;
		case 'i':			/* case-insensitive match */
			ignore_case++;
			break;
# endif	/* NO_REGEX */
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (!form_file_list(argv, argc))
		exit(1);	/* errors printed through form_file_list() */
#ifdef DEBUG
	if (Debug >= 1)
		print_file_list();
#endif /* DEBUG */
	if (Find_files) {
		print_file_list();
		exit(0);
	}

# ifndef NO_REGEX
	if (pat != NULL) {
		if (ignore_case)
			pat = conv_pat(pat);
		RE_INIT(Re_pat);
		RE_COMP(Re_pat, pat);
		if (!RE_OK(Re_pat)) {
			warnx("%s: `%s'", RE_ERROR(Re_pat), pat);
			RE_FREE(Re_pat);
		}
		rot13(pat, 0);
		RE_INIT(Re_pat13);
		RE_COMP(Re_pat13, pat);
		if (!RE_OK(Re_pat13)) {
			warnx("%s: `%s'", RE_ERROR(Re_pat13), pat);
			RE_FREE(Re_pat13);
		}
	}
# endif	/* NO_REGEX */
}

/*
 * form_file_list:
 *	Form the file list from the file specifications.
 */
static int
form_file_list(files, file_cnt)
	char	**files;
	int	file_cnt;
{
	int	i, percent;
	const char	*sp;

	if (file_cnt == 0) {
		if (All_forts)
			return add_file(NO_PROB, FORTDIR, NULL, &File_list,
					&File_tail, NULL);
		else
			return add_file(NO_PROB, "fortunes", FORTDIR,
					&File_list, &File_tail, NULL);
	}
	for (i = 0; i < file_cnt; i++) {
		percent = NO_PROB;
		if (!isdigit((unsigned char)files[i][0]))
			sp = files[i];
		else {
			percent = 0;
			for (sp = files[i]; isdigit((unsigned char)*sp); sp++)
				percent = percent * 10 + *sp - '0';
			if (percent > 100) {
				warnx("Percentages must be <= 100");
				return FALSE;
			}
			if (*sp == '.') {
				warnx("Percentages must be integers");
				return FALSE;
			}
			/*
			 * If the number isn't followed by a '%', then
			 * it was not a percentage, just the first part
			 * of a file name which starts with digits.
			 */
			if (*sp != '%') {
				percent = NO_PROB;
				sp = files[i];
			}
			else if (*++sp == '\0') {
				if (++i >= file_cnt) {
					warnx("Percentages must precede files");
					return FALSE;
				}
				sp = files[i];
			}
		}
		if (strcmp(sp, "all") == 0)
			sp = FORTDIR;
		if (!add_file(percent, sp, NULL, &File_list, &File_tail, NULL))
			return FALSE;
	}
	return TRUE;
}

/*
 * add_file:
 *	Add a file to the file list.
 */
static int
add_file(percent, file, dir, head, tail, parent)
	int		 percent;
	const char	*file;
	const char	*dir;
	FILEDESC	**head, **tail;
	FILEDESC	*parent;
{
	FILEDESC	*fp;
	int		fd;
	const char	*path;
	char		*tpath, *offensive, *tfile = strdup(file), *tf;
	bool		isdir;

	if (dir == NULL) {
		path = tfile;
		tpath = NULL;
	}
	else {
		tpath = do_malloc((unsigned int) (strlen(dir) + strlen(file) + 2));
		(void) strcat(strcat(strcpy(tpath, dir), "/"), file);
		path = tpath;
	}
	if ((isdir = is_dir(path)) && parent != NULL) {
		if (tpath)
			free(tpath);
		free(tfile);
		return FALSE;	/* don't recurse */
	}
	offensive = NULL;
	if (!isdir && parent == NULL && (All_forts || Offend) &&
	    !is_off_name(path)) {
		offensive = off_name(path);
		if (Offend) {
			if (tpath) {
				free(tpath);
				tpath = NULL;
			}
			path = offensive;
			tf = off_name(tfile);
			free(tfile);
			tfile = tf;
		}
	}

	DPRINTF(1, (stderr, "adding file \"%s\"\n", path));
over:
	if ((fd = open(path, O_RDONLY)) < 0) {
		/*
		 * This is a sneak.  If the user said -a, and if the
		 * file we're given isn't a file, we check to see if
		 * there is a -o version.  If there is, we treat it as
		 * if *that* were the file given.  We only do this for
		 * individual files -- if we're scanning a directory,
		 * we'll pick up the -o file anyway.
		 */
		if (All_forts && offensive != NULL && path != offensive) {
			path = offensive;
			if (tpath) {
				free(tpath);
				tpath = NULL;
			}
			DPRINTF(1, (stderr, "\ttrying \"%s\"\n", tfile));
			tf = off_name(tfile);
			free(tfile);
			tfile = tf;
			goto over;
		}
		if (dir == NULL && tfile[0] != '/') {
			int n = add_file(percent, tfile, FORTDIR, head, tail,
					parent);
			free(tfile);
			if (offensive)
				free(offensive);
			return n;
		}
		if (parent == NULL)
			warn("Cannot open `%s'", path);
		if (tpath) {
			free(tpath);
			tpath = NULL;
		}
		free(tfile);
		if (offensive)
			free(offensive);
		return FALSE;
	}

	DPRINTF(2, (stderr, "path = \"%s\"\n", path));

	fp = new_fp();
	fp->fd = fd;
	fp->percent = percent;
	fp->name = tfile;
	fp->path = path;
	fp->parent = parent;

	if ((isdir && !add_dir(fp)) ||
	    (!isdir &&
	     !is_fortfile(path, &fp->datfile, &fp->posfile, (parent != NULL))))
	{
		if (parent == NULL)
			warnx("`%s' not a fortune file or directory", path);
		if (tpath) {
			free(tpath);
			tpath = NULL;
		}
		do_free(fp->datfile);
		do_free(fp->posfile);
		free(fp);
		do_free(offensive);
		return FALSE;
	}
	/*
	 * If the user said -a, we need to make this node a pointer to
	 * both files, if there are two.  We don't need to do this if
	 * we are scanning a directory, since the scan will pick up the
	 * -o file anyway.
	 */
	if (All_forts && parent == NULL && !is_off_name(path) && offensive)
		all_forts(fp, offensive);
	if (*head == NULL)
		*head = *tail = fp;
	else if (fp->percent == NO_PROB) {
		(*tail)->next = fp;
		fp->prev = *tail;
		*tail = fp;
	}
	else {
		(*head)->prev = fp;
		fp->next = *head;
		*head = fp;
	}
#ifdef	OK_TO_WRITE_DISK
	fp->was_pos_file = (access(fp->posfile, W_OK) >= 0);
#endif	/* OK_TO_WRITE_DISK */

	return TRUE;
}

/*
 * new_fp:
 *	Return a pointer to an initialized new FILEDESC.
 */
static FILEDESC *
new_fp()
{
	FILEDESC	*fp;

	fp = (FILEDESC *) do_malloc(sizeof *fp);
	fp->datfd = -1;
	fp->pos = POS_UNKNOWN;
	fp->inf = NULL;
	fp->fd = -1;
	fp->percent = NO_PROB;
	fp->read_tbl = FALSE;
	fp->next = NULL;
	fp->prev = NULL;
	fp->child = NULL;
	fp->parent = NULL;
	fp->datfile = NULL;
	fp->posfile = NULL;
	return fp;
}

/*
 * off_name:
 *	Return a pointer to the offensive version of a file of this name.
 */
static char *
off_name(file)
	const char	*file;
{
	char	*new;

	new = copy(file, (unsigned int) (strlen(file) + 2));
	return strcat(new, "-o");
}

/*
 * is_off_name:
 *	Is the file an offensive-style name?
 */
static int
is_off_name(file)
	const char	*file;
{
	int	len;

	len = strlen(file);
	return (len >= 3 && file[len - 2] == '-' && file[len - 1] == 'o');
}

/*
 * all_forts:
 *	Modify a FILEDESC element to be the parent of two children if
 *	there are two children to be a parent of.
 */
static void
all_forts(fp, offensive)
	FILEDESC	*fp;
	const char	*offensive;
{
	char		*sp;
	FILEDESC	*scene, *obscene;
	int		 fd;
	char		*datfile, *posfile;

	posfile = NULL;

	if (fp->child != NULL)	/* this is a directory, not a file */
		return;
	if (!is_fortfile(offensive, &datfile, &posfile, FALSE))
		return;
	if ((fd = open(offensive, O_RDONLY)) < 0)
		return;
	DPRINTF(1, (stderr, "adding \"%s\" because of -a\n", offensive));
	scene = new_fp();
	obscene = new_fp();
	*scene = *fp;

	fp->num_children = 2;
	fp->child = scene;
	scene->next = obscene;
	obscene->next = NULL;
	scene->child = obscene->child = NULL;
	scene->parent = obscene->parent = fp;

	fp->fd = -1;
	scene->percent = obscene->percent = NO_PROB;

	obscene->fd = fd;
	obscene->inf = NULL;
	obscene->path = offensive;
	if ((sp = rindex(offensive, '/')) == NULL)
		obscene->name = offensive;
	else
		obscene->name = ++sp;
	obscene->datfile = datfile;
	obscene->posfile = posfile;
	obscene->read_tbl = FALSE;
#ifdef	OK_TO_WRITE_DISK
	obscene->was_pos_file = (access(obscene->posfile, W_OK) >= 0);
#endif	/* OK_TO_WRITE_DISK */
}

/*
 * add_dir:
 *	Add the contents of an entire directory.
 */
static int
add_dir(fp)
	FILEDESC	*fp;
{
	DIR		*dir;
	struct dirent	*dirent;
	FILEDESC	*tailp;
	char		*name;

	(void) close(fp->fd);
	fp->fd = -1;
	if ((dir = opendir(fp->path)) == NULL) {
		warn("Cannot open `%s'", fp->path);
		return FALSE;
	}
	tailp = NULL;
	DPRINTF(1, (stderr, "adding dir \"%s\"\n", fp->path));
	fp->num_children = 0;
	while ((dirent = readdir(dir)) != NULL) {
		if (NAMLEN(dirent) == 0)
			continue;
		name = copy(dirent->d_name, NAMLEN(dirent));
		if (add_file(NO_PROB, name, fp->path, &fp->child, &tailp, fp))
			fp->num_children++;
		else
			free(name);
	}
	if (fp->num_children == 0) {
		warnx("`%s': No fortune files in directory.", fp->path);
		return FALSE;
	}
	return TRUE;
}

/*
 * is_dir:
 *	Return TRUE if the file is a directory, FALSE otherwise.
 */
static int
is_dir(file)
	const char	*file;
{
	struct stat	sbuf;

	if (stat(file, &sbuf) < 0)
		return FALSE;
	return (S_ISDIR(sbuf.st_mode));
}

/*
 * is_fortfile:
 *	Return TRUE if the file is a fortune database file.  We try and
 *	exclude files without reading them if possible to avoid
 *	overhead.  Files which start with ".", or which have "illegal"
 *	suffixes, as contained in suflist[], are ruled out.
 */
/* ARGSUSED */
static int
is_fortfile(file, datp, posp, check_for_offend)
	const char	*file;
	char		**datp, **posp
# ifndef OK_TO_WRITE_DISK
	__unused
# endif
	;
	int	check_for_offend;
{
	int	i;
	const char	*sp;
	char	*datfile;
	static const char	*const suflist[] = {	/* list of "illegal" suffixes" */
				"dat", "pos", "c", "h", "p", "i", "f",
				"pas", "ftn", "ins.c", "ins,pas",
				"ins.ftn", "sml",
				NULL
			};

	DPRINTF(2, (stderr, "is_fortfile(%s) returns ", file));

	/*
	 * Preclude any -o files for offendable people, and any non -o
	 * files for completely offensive people.
	 */
	if (check_for_offend && !All_forts) {
		i = strlen(file);
		if (Offend ^ (file[i - 2] == '-' && file[i - 1] == 'o'))
			return FALSE;
	}

	if ((sp = rindex(file, '/')) == NULL)
		sp = file;
	else
		sp++;
	if (*sp == '.') {
		DPRINTF(2, (stderr, "FALSE (file starts with '.')\n"));
		return FALSE;
	}
	if ((sp = rindex(sp, '.')) != NULL) {
		sp++;
		for (i = 0; suflist[i] != NULL; i++)
			if (strcmp(sp, suflist[i]) == 0) {
				DPRINTF(2, (stderr, "FALSE (file has suffix \".%s\")\n", sp));
				return FALSE;
			}
	}

	datfile = copy(file, (unsigned int) (strlen(file) + 4)); /* +4 for ".dat" */
	strcat(datfile, ".dat");
	if (access(datfile, R_OK) < 0) {
		free(datfile);
		DPRINTF(2, (stderr, "FALSE (no \".dat\" file)\n"));
		return FALSE;
	}
	if (datp != NULL)
		*datp = datfile;
	else
		free(datfile);
#ifdef	OK_TO_WRITE_DISK
	if (posp != NULL) {
		*posp = copy(file, (unsigned int) (strlen(file) + 4)); /* +4 for ".dat" */
		(void) strcat(*posp, ".pos");
	}
#endif	/* OK_TO_WRITE_DISK */
	DPRINTF(2, (stderr, "TRUE\n"));
	return TRUE;
}

/*
 * copy:
 *	Return a malloc()'ed copy of the string
 */
static char *
copy(str, len)
	const char	*str;
	unsigned int	len;
{
	char	*new, *sp;

	new = do_malloc(len + 1);
	sp = new;
	do {
		*sp++ = *str;
	} while (*str++);
	return new;
}

/*
 * do_malloc:
 *	Do a malloc, checking for NULL return.
 */
static void *
do_malloc(size)
	unsigned int	size;
{
	void	*new;

	if ((new = malloc(size)) == NULL)
		err(1, NULL);
	return new;
}

/*
 * do_free:
 *	Free malloc'ed space, if any.
 */
static void
do_free(ptr)
	void	*ptr;
{
	if (ptr != NULL)
		free(ptr);
}

/*
 * init_prob:
 *	Initialize the fortune probabilities.
 */
static void
init_prob()
{
	FILEDESC	*fp, *last;
	int		percent, num_noprob, frac;

	last = NULL;
	/*
	 * Distribute the residual probability (if any) across all
	 * files with unspecified probability (i.e., probability of 0)
	 * (if any).
	 */

	percent = 0;
	num_noprob = 0;
	for (fp = File_tail; fp != NULL; fp = fp->prev)
		if (fp->percent == NO_PROB) {
			num_noprob++;
			if (Equal_probs)
				last = fp;
		} else
			percent += fp->percent;
	DPRINTF(1, (stderr, "summing probabilities:%d%% with %d NO_PROB's",
		    percent, num_noprob));
	if (percent > 100)
		errx(1, "Probabilities sum to %d%%!", percent);
	else if (percent < 100 && num_noprob == 0)
		errx(1, "No place to put residual probability (%d%%)",
		    100 - percent);
	else if (percent == 100 && num_noprob != 0)
		errx(1, "No probability left to put in residual files");
	percent = 100 - percent;
	if (Equal_probs) {
		if (num_noprob != 0) {
			if (num_noprob > 1) {
				frac = percent / num_noprob;
				DPRINTF(1, (stderr, ", frac = %d%%", frac));
				for (fp = File_tail; fp != last; fp = fp->prev)
					if (fp->percent == NO_PROB) {
						fp->percent = frac;
						percent -= frac;
					}
			}
			last->percent = percent;
			DPRINTF(1, (stderr, ", residual = %d%%", percent));
		}
	} else {
		DPRINTF(1, (stderr,
			    ", %d%% distributed over remaining fortunes\n",
			    percent));
	}
	DPRINTF(1, (stderr, "\n"));

#ifdef DEBUG
	if (Debug >= 1)
		print_file_list();
#endif
}

/*
 * get_fort:
 *	Get the fortune data file's seek pointer for the next fortune.
 */
static void
get_fort()
{
	FILEDESC	*fp;
	int		choice;

	if (File_list->next == NULL || File_list->percent == NO_PROB)
		fp = File_list;
	else {
		choice = random() % 100;
		DPRINTF(1, (stderr, "choice = %d\n", choice));
		for (fp = File_list; fp->percent != NO_PROB; fp = fp->next)
			if (choice < fp->percent)
				break;
			else {
				choice -= fp->percent;
				DPRINTF(1, (stderr,
					    "    skip \"%s\", %d%% (choice = %d)\n",
					    fp->name, fp->percent, choice));
			}
			DPRINTF(1, (stderr,
				    "using \"%s\", %d%% (choice = %d)\n",
				    fp->name, fp->percent, choice));
	}
	if (fp->percent != NO_PROB)
		get_tbl(fp);
	else {
		if (fp->next != NULL) {
			sum_noprobs(fp);
			choice = random() % Noprob_tbl.str_numstr;
			DPRINTF(1, (stderr, "choice = %d (of %d) \n", choice,
				    Noprob_tbl.str_numstr));
			while ((u_int32_t)choice >= fp->tbl.str_numstr) {
				choice -= fp->tbl.str_numstr;
				fp = fp->next;
				DPRINTF(1, (stderr,
					    "    skip \"%s\", %d (choice = %d)\n",
					    fp->name, fp->tbl.str_numstr,
					    choice));
			}
			DPRINTF(1, (stderr, "using \"%s\", %d\n", fp->name,
				    fp->tbl.str_numstr));
		}
		get_tbl(fp);
	}
	if (fp->child != NULL) {
		DPRINTF(1, (stderr, "picking child\n"));
		fp = pick_child(fp);
	}
	Fortfile = fp;
	get_pos(fp);
	open_dat(fp);
	(void) lseek(fp->datfd,
		     (off_t) (sizeof fp->tbl + fp->pos * sizeof Seekpts[0]), SEEK_SET);
	read(fp->datfd, Seekpts, sizeof Seekpts);
	BE64TOH(Seekpts[0]);
	BE64TOH(Seekpts[1]);
}

/*
 * pick_child
 *	Pick a child from a chosen parent.
 */
static FILEDESC *
pick_child(parent)
	FILEDESC	*parent;
{
	FILEDESC	*fp;
	int		 choice;

	if (Equal_probs) {
		choice = random() % parent->num_children;
		DPRINTF(1, (stderr, "    choice = %d (of %d)\n",
			    choice, parent->num_children));
		for (fp = parent->child; choice--; fp = fp->next)
			continue;
		DPRINTF(1, (stderr, "    using %s\n", fp->name));
		return fp;
	}
	else {
		get_tbl(parent);
		choice = random() % parent->tbl.str_numstr;
		DPRINTF(1, (stderr, "    choice = %d (of %d)\n",
			    choice, parent->tbl.str_numstr));
		for (fp = parent->child; (u_int32_t)choice >= fp->tbl.str_numstr;
		     fp = fp->next) {
			choice -= fp->tbl.str_numstr;
			DPRINTF(1, (stderr, "\tskip %s, %d (choice = %d)\n",
				    fp->name, fp->tbl.str_numstr, choice));
		}
		DPRINTF(1, (stderr, "    using %s, %d\n", fp->name,
			    fp->tbl.str_numstr));
		return fp;
	}
}

/*
 * sum_noprobs:
 *	Sum up all the noprob probabilities, starting with fp.
 */
static void
sum_noprobs(fp)
	FILEDESC	*fp;
{
	static bool	did_noprobs = FALSE;

	if (did_noprobs)
		return;
	zero_tbl(&Noprob_tbl);
	while (fp != NULL) {
		get_tbl(fp);
		sum_tbl(&Noprob_tbl, &fp->tbl);
		fp = fp->next;
	}
	did_noprobs = TRUE;
}

static int
max(i, j)
	int	i, j;
{
	return (i >= j ? i : j);
}

/*
 * open_fp:
 *	Assocatiate a FILE * with the given FILEDESC.
 */
static void
open_fp(fp)
	FILEDESC	*fp;
{
	if (fp->inf == NULL && (fp->inf = fdopen(fp->fd, "r")) == NULL)
		err(1, "Cannot open `%s'", fp->path);
}

/*
 * open_dat:
 *	Open up the dat file if we need to.
 */
static void
open_dat(fp)
	FILEDESC	*fp;
{
	if (fp->datfd < 0 && (fp->datfd = open(fp->datfile, O_RDONLY)) < 0)
		err(1, "Cannot open `%s'", fp->datfile);
}

/*
 * get_pos:
 *	Get the position from the pos file, if there is one.  If not,
 *	return a random number.
 */
static void
get_pos(fp)
	FILEDESC	*fp;
{
#ifdef	OK_TO_WRITE_DISK
	int	fd;
#endif /* OK_TO_WRITE_DISK */

	assert(fp->read_tbl);
	if (fp->pos == POS_UNKNOWN) {
#ifdef	OK_TO_WRITE_DISK
		if ((fd = open(fp->posfile, O_RDONLY)) < 0 ||
		    read(fd, &fp->pos, sizeof fp->pos) != sizeof fp->pos)
			fp->pos = random() % fp->tbl.str_numstr;
		else if (fp->pos >= fp->tbl.str_numstr)
			fp->pos %= fp->tbl.str_numstr;
		if (fd >= 0)
			(void) close(fd);
#else
		fp->pos = random() % fp->tbl.str_numstr;
#endif /* OK_TO_WRITE_DISK */
	}
	if ((u_int64_t)++(fp->pos) >= fp->tbl.str_numstr)
		fp->pos -= fp->tbl.str_numstr;
	DPRINTF(1, (stderr, "pos for %s is %lld\n", fp->name,
	    (long long)fp->pos));
}

/*
 * get_tbl:
 *	Get the tbl data file the datfile.
 */
static void
get_tbl(fp)
	FILEDESC	*fp;
{
	int	 	fd;
	FILEDESC	*child;

	if (fp->read_tbl)
		return;
	if (fp->child == NULL) {
		if ((fd = open(fp->datfile, O_RDONLY)) < 0)
			err(1, "Cannot open `%s'", fp->datfile);
		if (read(fd, (char *) &fp->tbl, sizeof fp->tbl) != sizeof fp->tbl) {
			errx(1, "Database `%s' corrupted", fp->path);
		}
		/* BE32TOH(fp->tbl.str_version); */
		BE32TOH(fp->tbl.str_numstr);
		BE32TOH(fp->tbl.str_longlen);
		BE32TOH(fp->tbl.str_shortlen);
		BE32TOH(fp->tbl.str_flags);
		(void) close(fd);
	}
	else {
		zero_tbl(&fp->tbl);
		for (child = fp->child; child != NULL; child = child->next) {
			get_tbl(child);
			sum_tbl(&fp->tbl, &child->tbl);
		}
	}
	fp->read_tbl = TRUE;
}

/*
 * zero_tbl:
 *	Zero out the fields we care about in a tbl structure.
 */
static void
zero_tbl(tp)
	STRFILE	*tp;
{
	tp->str_numstr = 0;
	tp->str_longlen = 0;
	tp->str_shortlen = -1;
}

/*
 * sum_tbl:
 *	Merge the tbl data of t2 into t1.
 */
static void
sum_tbl(t1, t2)
	STRFILE	*t1, *t2;
{
	t1->str_numstr += t2->str_numstr;
	if (t1->str_longlen < t2->str_longlen)
		t1->str_longlen = t2->str_longlen;
	if (t1->str_shortlen > t2->str_shortlen)
		t1->str_shortlen = t2->str_shortlen;
}

#define	STR(str)	((str) == NULL ? "NULL" : (str))

/*
 * print_file_list:
 *	Print out the file list
 */
static void
print_file_list()
{
	print_list(File_list, 0);
}

/*
 * print_list:
 *	Print out the actual list, recursively.
 */
static void
print_list(list, lev)
	FILEDESC	*list;
	int		 lev;
{
	while (list != NULL) {
		fprintf(stderr, "%*s", lev * 4, "");
		if (list->percent == NO_PROB)
			fprintf(stderr, "___%%");
		else
			fprintf(stderr, "%3d%%", list->percent);
		fprintf(stderr, " %s", STR(list->name));
		DPRINTF(1, (stderr, " (%s, %s, %s)\n", STR(list->path),
			    STR(list->datfile), STR(list->posfile)));
		putc('\n', stderr);
		if (list->child != NULL)
			print_list(list->child, lev + 1);
		list = list->next;
	}
}

#ifndef	NO_REGEX
/*
 * conv_pat:
 *	Convert the pattern to an ignore-case equivalent.
 */
static char *
conv_pat(orig)
	char	*orig;
{
	char		*sp;
	unsigned int	 cnt;
	char		*new;

	cnt = 1;	/* allow for '\0' */
	for (sp = orig; *sp != '\0'; sp++)
		if (isalpha((unsigned char)*sp))
			cnt += 4;
		else
			cnt++;
	if ((new = malloc(cnt)) == NULL)
		err(1, NULL);

	for (sp = new; *orig != '\0'; orig++) {
		if (islower((unsigned char)*orig)) {
			*sp++ = '[';
			*sp++ = *orig;
			*sp++ = toupper((unsigned char)*orig);
			*sp++ = ']';
		}
		else if (isupper((unsigned char)*orig)) {
			*sp++ = '[';
			*sp++ = *orig;
			*sp++ = tolower((unsigned char)*orig);
			*sp++ = ']';
		}
		else
			*sp++ = *orig;
	}
	*sp = '\0';
	return new;
}

/*
 * find_matches:
 *	Find all the fortunes which match the pattern we've been given.
 */
static int
find_matches()
{
	Fort_len = maxlen_in_list(File_list);
	DPRINTF(2, (stderr, "Maximum length is %d\n", Fort_len));
	/* extra length, "%\n" is appended */
	Fortbuf = do_malloc((unsigned int) Fort_len + 10);

	Found_one = FALSE;
	matches_in_list(File_list);
	return Found_one;
	/* NOTREACHED */
}

/*
 * maxlen_in_list
 *	Return the maximum fortune len in the file list.
 */
static int
maxlen_in_list(list)
	FILEDESC	*list;
{
	FILEDESC	*fp;
	int		 len, maxlen;

	maxlen = 0;
	for (fp = list; fp != NULL; fp = fp->next) {
		if (fp->child != NULL) {
			if ((len = maxlen_in_list(fp->child)) > maxlen)
				maxlen = len;
		}
		else {
			get_tbl(fp);
			if (fp->tbl.str_longlen > (u_int32_t)maxlen)
				maxlen = fp->tbl.str_longlen;
		}
	}
	return maxlen;
}

/*
 * matches_in_list
 *	Print out the matches from the files in the list.
 */
static void
matches_in_list(list)
	FILEDESC	*list;
{
	char		*sp;
	FILEDESC	*fp;
	int		 in_file;

	if (!RE_OK(Re_pat) || !RE_OK(Re_pat13))
		return;

	for (fp = list; fp != NULL; fp = fp->next) {
		if (fp->child != NULL) {
			matches_in_list(fp->child);
			continue;
		}
		DPRINTF(1, (stderr, "searching in %s\n", fp->path));
		open_fp(fp);
		sp = Fortbuf;
		in_file = FALSE;
		while (fgets(sp, Fort_len, fp->inf) != NULL)
			if (!STR_ENDSTRING(sp, fp->tbl))
				sp += strlen(sp);
			else {
				*sp = '\0';
				if (fp->tbl.str_flags & STR_ROTATED)
					Re_use = Re_pat13;
				else
					Re_use = Re_pat;
				if (RE_EXEC(Re_use, Fortbuf)) {
					printf("%c%c", fp->tbl.str_delim,
					    fp->tbl.str_delim);
					if (!in_file) {
						printf(" (%s)", fp->name);
						Found_one = TRUE;
						in_file = TRUE;
					}
					putchar('\n');
					if (fp->tbl.str_flags & STR_ROTATED)
						rot13(Fortbuf, (sp - Fortbuf));
					(void) fwrite(Fortbuf, 1, (sp - Fortbuf), stdout);
				}
				sp = Fortbuf;
			}
	}
	RE_FREE(Re_pat);
	RE_FREE(Re_pat13);
}
# endif	/* NO_REGEX */

static void
usage()
{

	(void) fprintf(stderr, "Usage: %s [-ae", getprogname());
#ifdef	DEBUG
	(void) fprintf(stderr, "D");
#endif	/* DEBUG */
	(void) fprintf(stderr, "f");
#ifndef	NO_REGEX
	(void) fprintf(stderr, "i");
#endif	/* NO_REGEX */
	(void) fprintf(stderr, "losw]");
#ifndef	NO_REGEX
	(void) fprintf(stderr, " [-m pattern]");
#endif	/* NO_REGEX */
	(void) fprintf(stderr, "[ [#%%] file/directory/all]\n");
	exit(1);
}
