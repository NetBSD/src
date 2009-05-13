/* filesubr.c --- subroutines for dealing with files
   Jim Blandy <jimb@cyclic.com>

   This file is part of GNU CVS.

   GNU CVS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

/* These functions were moved out of subr.c because they need different
   definitions under operating systems (like, say, Windows NT) with different
   file system semantics.  */

#include "cvs.h"
#include "lstat.h"
#include "save-cwd.h"
#include "xsize.h"

static int deep_remove_dir (const char *path);

/*
 * Copies "from" to "to".
 */
void
copy_file (const char *from, const char *to)
{
    struct stat sb;
    struct utimbuf t;
    int fdin, fdout;
    ssize_t rsize;

    TRACE (TRACE_FUNCTION, "copy(%s,%s)", from, to);

    if (noexec)
	return;

    /* If the file to be copied is a link or a device, then just create
       the new link or device appropriately. */
    if ((rsize = islink (from)) > 0)
    {
	char *source = Xreadlink (from, rsize);
	symlink (source, to);
	free (source);
	return;
    }

    if (isdevice (from))
    {
#if defined(HAVE_MKNOD) && defined(HAVE_STRUCT_STAT_ST_RDEV)
	if (stat (from, &sb) < 0)
	    error (1, errno, "cannot stat %s", from);
	mknod (to, sb.st_mode, sb.st_rdev);
#else
	error (1, 0, "cannot copy device files on this system (%s)", from);
#endif
    }
    else
    {
	/* Not a link or a device... probably a regular file. */
	if ((fdin = open (from, O_RDONLY)) < 0)
	    error (1, errno, "cannot open %s for copying", from);
	if (fstat (fdin, &sb) < 0)
	    error (1, errno, "cannot fstat %s", from);
	if ((fdout = creat (to, (int) sb.st_mode & 07777)) < 0)
	    error (1, errno, "cannot create %s for copying", to);
	if (sb.st_size > 0)
	{
	    char buf[BUFSIZ];
	    int n;
	    
	    for (;;) 
	    {
		n = read (fdin, buf, sizeof(buf));
		if (n == -1)
		{
#ifdef EINTR
		    if (errno == EINTR)
			continue;
#endif
		    error (1, errno, "cannot read file %s for copying", from);
		}
		else if (n == 0) 
		    break;
		
		if (write(fdout, buf, n) != n) {
		    error (1, errno, "cannot write file %s for copying", to);
		}
	    }
	}

	if (close (fdin) < 0) 
	    error (0, errno, "cannot close %s", from);
	if (close (fdout) < 0)
	    error (1, errno, "cannot close %s", to);
    }

    /* preserve last access & modification times */
    memset ((char *) &t, 0, sizeof (t));
    t.actime = sb.st_atime;
    t.modtime = sb.st_mtime;
    (void) utime (to, &t);
}



/* FIXME-krp: these functions would benefit from caching the char * &
   stat buf.  */

/*
 * Returns true if the argument file is a directory, or is a symbolic
 * link which points to a directory.
 */
bool
isdir (const char *file)
{
    struct stat sb;

    if (stat (file, &sb) < 0)
	return false;
    return S_ISDIR (sb.st_mode);
}



/*
 * Returns 0 if the argument file is not a symbolic link.
 * Returns size of the link if it is a symbolic link.
 */
ssize_t
islink (const char *file)
{
    ssize_t retsize = 0;
#ifdef S_ISLNK
    struct stat sb;

    if ((lstat (file, &sb) >= 0) && S_ISLNK (sb.st_mode))
	retsize = sb.st_size;
#endif
    return retsize;
}



/*
 * Returns true if the argument file is a block or
 * character special device.
 */
bool
isdevice (const char *file)
{
    struct stat sb;

    if (lstat (file, &sb) < 0)
	return false;
#ifdef S_ISBLK
    if (S_ISBLK (sb.st_mode))
	return true;
#endif
#ifdef S_ISCHR
    if (S_ISCHR (sb.st_mode))
	return true;
#endif
    return false;
}



/*
 * Returns true if the argument file exists.
 */
bool
isfile (const char *file)
{
    return isaccessible (file, F_OK);
}

#ifdef SETXID_SUPPORT
int
ingroup(gid_t gid)
{
    gid_t *gidp;
    int i, ngroups;

    if (gid == getegid())
	return 1;

    ngroups = getgroups(0, NULL);
    if (ngroups == -1)
	return 0;

    if ((gidp = malloc(sizeof(gid_t) * ngroups)) == NULL)
	return 0;

    if (getgroups(ngroups, gidp) == -1) {
	free(gidp);
	return 0;
    }

    for (i = 0; i < ngroups; i++)
	if (gid == gidp[i])
	    break;

    free(gidp);
    return i != ngroups;
}
#endif

/*
 * Returns non-zero if the argument file is readable.
 */
bool
isreadable (const char *file)
{
    return isaccessible (file, R_OK);
}



/*
 * Returns non-zero if the argument file is writable.
 */
bool
iswritable (const char *file)
{
    return isaccessible (file, W_OK);
}



/*
 * Returns true if the argument file is accessable according to
 * mode.  If compiled with SETXID_SUPPORT also works if cvs has setxid
 * bits set.
 */
bool
isaccessible (const char *file, const int mode)
{
#ifdef SETXID_SUPPORT
    struct stat sb;
    int umask = 0;
    int gmask = 0;
    int omask = 0;
    int uid, mask;
    
    if (stat (file, &sb)== -1)
	return false;
    if (mode == F_OK)
	return true;

    uid = geteuid();
    if (uid == 0)		/* superuser */
    {
	if (!(mode & X_OK) || (sb.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
	    return true;

	errno = EACCES;
	return false;
    }
	
    if (mode & R_OK)
    {
	umask |= S_IRUSR;
	gmask |= S_IRGRP;
	omask |= S_IROTH;
    }
    if (mode & W_OK)
    {
	umask |= S_IWUSR;
	gmask |= S_IWGRP;
	omask |= S_IWOTH;
    }
    if (mode & X_OK)
    {
	umask |= S_IXUSR;
	gmask |= S_IXGRP;
	omask |= S_IXOTH;
    }

    mask = sb.st_uid == uid ? umask : sb.st_gid == ingroup(sb.st_gid) ?
	gmask : omask;
    if ((sb.st_mode & mask) == mask)
	return true;
    errno = EACCES;
    return false;
#else /* !SETXID_SUPPORT */
    return access (file, mode) == 0;
#endif /* SETXID_SUPPORT */
}



/*
 * Make a directory and die if it fails
 */
void
make_directory (const char *name)
{
    struct stat sb;

    if (stat (name, &sb) == 0 && (!S_ISDIR (sb.st_mode)))
	    error (0, 0, "%s already exists but is not a directory", name);
    if (!noexec && mkdir (name, 0777) < 0)
	error (1, errno, "cannot make directory %s", name);
}

/*
 * Make a path to the argument directory, printing a message if something
 * goes wrong.
 */
void
make_directories (const char *name)
{
    char *cp;

    if (noexec)
	return;

    if (mkdir (name, 0777) == 0 || errno == EEXIST)
	return;
    if (! existence_error (errno))
    {
	error (0, errno, "cannot make path to %s", name);
	return;
    }
    if ((cp = strrchr (name, '/')) == NULL)
	return;
    *cp = '\0';
    make_directories (name);
    *cp++ = '/';
    if (*cp == '\0')
	return;
    (void) mkdir (name, 0777);
}

/* Create directory NAME if it does not already exist; fatal error for
   other errors.  Returns 0 if directory was created; 1 if it already
   existed.  */
int
mkdir_if_needed (const char *name)
{
    if (mkdir (name, 0777) < 0)
    {
	int save_errno = errno;
	if (save_errno != EEXIST && !isdir (name))
	    error (1, save_errno, "cannot make directory %s", name);
	return 1;
    }
    return 0;
}

/*
 * Change the mode of a file, either adding write permissions, or removing
 * all write permissions.  Either change honors the current umask setting.
 *
 * Don't do anything if PreservePermissions is set to `yes'.  This may
 * have unexpected consequences for some uses of xchmod.
 */
void
xchmod (const char *fname, int writable)
{
    struct stat sb;
    mode_t mode, oumask;

#ifdef PRESERVE_PERMISSIONS_SUPPORT
    if (config->preserve_perms)
	return;
#endif /* PRESERVE_PERMISSIONS_SUPPORT */

    if (stat (fname, &sb) < 0)
    {
	if (!noexec)
	    error (0, errno, "cannot stat %s", fname);
	return;
    }
    oumask = umask (0);
    (void) umask (oumask);
    if (writable)
    {
	mode = sb.st_mode | (~oumask
			     & (((sb.st_mode & S_IRUSR) ? S_IWUSR : 0)
				| ((sb.st_mode & S_IRGRP) ? S_IWGRP : 0)
				| ((sb.st_mode & S_IROTH) ? S_IWOTH : 0)));
    }
    else
    {
	mode = sb.st_mode & ~(S_IWRITE | S_IWGRP | S_IWOTH) & ~oumask;
    }

    TRACE (TRACE_FUNCTION, "chmod(%s,%o)", fname, (unsigned int) mode);

    if (noexec)
	return;

    if (chmod (fname, mode) < 0)
	error (0, errno, "cannot change mode of file %s", fname);
}

/*
 * Rename a file and die if it fails
 */
void
rename_file (const char *from, const char *to)
{
    TRACE (TRACE_FUNCTION, "rename(%s,%s)", from, to);

    if (noexec)
	return;

    if (rename (from, to) < 0)
	error (1, errno, "cannot rename file %s to %s", from, to);
}

/*
 * unlink a file, if possible.
 */
int
unlink_file (const char *f)
{
    TRACE (TRACE_FUNCTION, "unlink_file(%s)", f);

    if (noexec)
	return (0);

    return (CVS_UNLINK (f));
}



/*
 * Unlink a file or dir, if possible.  If it is a directory do a deep
 * removal of all of the files in the directory.  Return -1 on error
 * (in which case errno is set).
 */
int
unlink_file_dir (const char *f)
{
    struct stat sb;

    /* This is called by the server parent process in contexts where
       it is not OK to send output (e.g. after we sent "ok" to the
       client).  */
    if (!server_active)
	TRACE (TRACE_FUNCTION, "unlink_file_dir(%s)", f);

    if (noexec)
	return 0;

    /* For at least some unices, if root tries to unlink() a directory,
       instead of doing something rational like returning EISDIR,
       the system will gleefully go ahead and corrupt the filesystem.
       So we first call stat() to see if it is OK to call unlink().  This
       doesn't quite work--if someone creates a directory between the
       call to stat() and the call to unlink(), we'll still corrupt
       the filesystem.  Where is the Unix Haters Handbook when you need
       it?  */
    if (stat (f, &sb) < 0)
    {
	if (existence_error (errno))
	{
	    /* The file or directory doesn't exist anyhow.  */
	    return -1;
	}
    }
    else if (S_ISDIR (sb.st_mode))
	return deep_remove_dir (f);

    return CVS_UNLINK (f);
}



/* Remove a directory and everything it contains.  Returns 0 for
 * success, -1 for failure (in which case errno is set).
 */

static int
deep_remove_dir (const char *path)
{
    DIR		  *dirp;
    struct dirent *dp;

    if (rmdir (path) != 0)
    {
	if (errno == ENOTEMPTY
	    || errno == EEXIST
	    /* Ugly workaround for ugly AIX 4.1 (and 3.2) header bug
	       (it defines ENOTEMPTY and EEXIST to 17 but actually
	       returns 87).  */
	    || (ENOTEMPTY == 17 && EEXIST == 17 && errno == 87))
	{
	    if ((dirp = CVS_OPENDIR (path)) == NULL)
		/* If unable to open the directory return
		 * an error
		 */
		return -1;

	    errno = 0;
	    while ((dp = CVS_READDIR (dirp)) != NULL)
	    {
		char *buf;

		if (strcmp (dp->d_name, ".") == 0 ||
			    strcmp (dp->d_name, "..") == 0)
		    continue;

		buf = Xasprintf ("%s/%s", path, dp->d_name);

		/* See comment in unlink_file_dir explanation of why we use
		   isdir instead of just calling unlink and checking the
		   status.  */
		if (isdir (buf)) 
		{
		    if (deep_remove_dir (buf))
		    {
			CVS_CLOSEDIR (dirp);
			free (buf);
			return -1;
		    }
		}
		else
		{
		    if (CVS_UNLINK (buf) != 0)
		    {
			CVS_CLOSEDIR (dirp);
			free (buf);
			return -1;
		    }
		}
		free (buf);

		errno = 0;
	    }
	    if (errno != 0)
	    {
		int save_errno = errno;
		CVS_CLOSEDIR (dirp);
		errno = save_errno;
		return -1;
	    }
	    CVS_CLOSEDIR (dirp);
	    return rmdir (path);
	}
	else
	    return -1;
    }

    /* Was able to remove the directory return 0 */
    return 0;
}



/* Read NCHARS bytes from descriptor FD into BUF.
   Return the number of characters successfully read.
   The number returned is always NCHARS unless end-of-file or error.  */
static size_t
block_read (int fd, char *buf, size_t nchars)
{
    char *bp = buf;
    size_t nread;

    do 
    {
	nread = read (fd, bp, nchars);
	if (nread == (size_t)-1)
	{
#ifdef EINTR
	    if (errno == EINTR)
		continue;
#endif
	    return (size_t)-1;
	}

	if (nread == 0)
	    break; 

	bp += nread;
	nchars -= nread;
    } while (nchars != 0);

    return bp - buf;
} 

    
/*
 * Compare "file1" to "file2". Return non-zero if they don't compare exactly.
 * If FILE1 and FILE2 are special files, compare their salient characteristics
 * (i.e. major/minor device numbers, links, etc.
 */
int
xcmp (const char *file1, const char *file2)
{
    char *buf1, *buf2;
    struct stat sb1, sb2;
    int fd1, fd2;
    int ret;

    if (lstat (file1, &sb1) < 0)
	error (1, errno, "cannot lstat %s", file1);
    if (lstat (file2, &sb2) < 0)
	error (1, errno, "cannot lstat %s", file2);

    /* If FILE1 and FILE2 are not the same file type, they are unequal. */
    if ((sb1.st_mode & S_IFMT) != (sb2.st_mode & S_IFMT))
	return 1;

    /* If FILE1 and FILE2 are symlinks, they are equal if they point to
       the same thing. */
#ifdef S_ISLNK
    if (S_ISLNK (sb1.st_mode) && S_ISLNK (sb2.st_mode))
    {
	int result;
	buf1 = Xreadlink (file1, sb1.st_size);
	buf2 = Xreadlink (file2, sb2.st_size);
	result = (strcmp (buf1, buf2) == 0);
	free (buf1);
	free (buf2);
	return result;
    }
#endif

    /* If FILE1 and FILE2 are devices, they are equal if their device
       numbers match. */
    if (S_ISBLK (sb1.st_mode) || S_ISCHR (sb1.st_mode))
    {
#ifdef HAVE_STRUCT_STAT_ST_RDEV
	if (sb1.st_rdev == sb2.st_rdev)
	    return 0;
	else
	    return 1;
#else
	error (1, 0, "cannot compare device files on this system (%s and %s)",
	       file1, file2);
#endif
    }

    if ((fd1 = open (file1, O_RDONLY)) < 0)
	error (1, errno, "cannot open file %s for comparing", file1);
    if ((fd2 = open (file2, O_RDONLY)) < 0)
	error (1, errno, "cannot open file %s for comparing", file2);

    /* A generic file compare routine might compare st_dev & st_ino here 
       to see if the two files being compared are actually the same file.
       But that won't happen in CVS, so we won't bother. */

    if (sb1.st_size != sb2.st_size)
	ret = 1;
    else if (sb1.st_size == 0)
	ret = 0;
    else
    {
	/* FIXME: compute the optimal buffer size by computing the least
	   common multiple of the files st_blocks field */
	size_t buf_size = 8 * 1024;
	size_t read1;
	size_t read2;

	buf1 = xmalloc (buf_size);
	buf2 = xmalloc (buf_size);

	do 
	{
	    read1 = block_read (fd1, buf1, buf_size);
	    if (read1 == (size_t)-1)
		error (1, errno, "cannot read file %s for comparing", file1);

	    read2 = block_read (fd2, buf2, buf_size);
	    if (read2 == (size_t)-1)
		error (1, errno, "cannot read file %s for comparing", file2);

	    /* assert (read1 == read2); */

	    ret = memcmp(buf1, buf2, read1);
	} while (ret == 0 && read1 == buf_size);

	free (buf1);
	free (buf2);
    }
	
    (void) close (fd1);
    (void) close (fd2);
    return (ret);
}

/* Generate a unique temporary filename.  Returns a pointer to a newly
 * malloc'd string containing the name.  Returns successfully or not at
 * all.
 *
 *     THIS FUNCTION IS DEPRECATED!!!  USE cvs_temp_file INSTEAD!!!
 *
 * and yes, I know about the way the rcs commands use temp files.  I think
 * they should be converted too but I don't have time to look into it right
 * now.
 */
char *
cvs_temp_name (void)
{
    char *fn;
    FILE *fp;

    fp = cvs_temp_file (&fn);
    if (fp == NULL)
	error (1, errno, "Failed to create temporary file");
    if (fclose (fp) == EOF)
	error (0, errno, "Failed to close temporary file %s", fn);
    return fn;
}

/* Generate a unique temporary filename and return an open file stream
 * to the truncated file by that name
 *
 *  INPUTS
 *	filename	where to place the pointer to the newly allocated file
 *   			name string
 *
 *  OUTPUTS
 *	filename	dereferenced, will point to the newly allocated file
 *			name string.  This value is undefined if the function
 *			returns an error.
 *
 *  RETURNS
 *	An open file pointer to a read/write mode empty temporary file with the
 *	unique file name or NULL on failure.
 *
 *  ERRORS
 *	On error, errno will be set to some value either by CVS_FOPEN or
 *	whatever system function is called to generate the temporary file name.
 *	The value of filename is undefined on error.
 */
FILE *
cvs_temp_file (char **filename)
{
    char *fn;
    FILE *fp;
    int fd;

    /* FIXME - I'd like to be returning NULL here in noexec mode, but I think
     * some of the rcs & diff functions which rely on a temp file run in
     * noexec mode too.
     */

    assert (filename != NULL);

    fn = Xasprintf ("%s/%s", get_cvs_tmp_dir (), "cvsXXXXXX");
    fd = mkstemp (fn);

    /* a NULL return will be interpreted by callers as an error and
     * errno should still be set
     */
    if (fd == -1)
	fp = NULL;
    else if ((fp = CVS_FDOPEN (fd, "w+")) == NULL)
    {
	/* Attempt to close and unlink the file since mkstemp returned
	 * sucessfully and we believe it's been created and opened.
	 */
 	int save_errno = errno;
	if (close (fd))
	    error (0, errno, "Failed to close temporary file %s", fn);
	if (CVS_UNLINK (fn))
	    error (0, errno, "Failed to unlink temporary file %s", fn);
	errno = save_errno;
    }

    if (fp == NULL)
	free (fn);

    /* mkstemp is defined to open mode 0600 using glibc 2.0.7+.  There used
     * to be a complicated #ifdef checking the library versions here and then
     * a chmod 0600 on the temp file for versions of glibc less than 2.1.  This
     * is rather a special case, leaves a race condition open regardless, and
     * one could hope that sysadmins have read the relevant security
     * announcements and upgraded by now to a version with a fix committed in
     * January of 1999.
     *
     * If it is decided at some point that old, buggy versions of glibc should
     * still be catered to, a umask of 0600 should be set before file creation
     * instead then reset after file creation since this would avoid the race
     * condition that the chmod left open to exploitation.
     */

    *filename = fn;
    return fp;
}



/* Return a pointer into PATH's last component.  */
const char *
last_component (const char *path)
{
    const char *last = strrchr (path, '/');
    
    if (last && (last != path))
        return last + 1;
    else
        return path;
}



/* Return the home directory.  Returns a pointer to storage
   managed by this function or its callees (currently getenv).
   This function will return the same thing every time it is
   called.  Returns NULL if there is no home directory.

   Note that for a pserver server, this may return root's home
   directory.  What typically happens is that upon being started from
   inetd, before switching users, the code in cvsrc.c calls
   get_homedir which remembers root's home directory in the static
   variable.  Then the switch happens and get_homedir might return a
   directory that we don't even have read or execute permissions for
   (which is bad, when various parts of CVS try to read there).  One
   fix would be to make the value returned by get_homedir only good
   until the next call (which would free the old value).  Another fix
   would be to just always malloc our answer, and let the caller free
   it (that is best, because some day we may need to be reentrant).

   The workaround is to put -f in inetd.conf which means that
   get_homedir won't get called until after the switch in user ID.

   The whole concept of a "home directory" on the server is pretty
   iffy, although I suppose some people probably are relying on it for
   .cvsrc and such, in the cases where it works.  */
char *
get_homedir (void)
{
    static char *home = NULL;
    char *env;
    struct passwd *pw;

    if (home != NULL)
	return home;

    if (!server_active && (env = getenv ("HOME")) != NULL)
	home = env;
    else if ((pw = (struct passwd *) getpwuid (getuid ()))
	     && pw->pw_dir)
	home = xstrdup (pw->pw_dir);
    else
	return 0;

    return home;
}

/* Compose a path to a file in the home directory.  This is necessary because
 * of different behavior on UNIX and VMS.  See the notes in vms/filesubr.c.
 *
 * A more clean solution would be something more along the lines of a
 * "join a directory to a filename" kind of thing which was not specific to
 * the homedir.  This should aid portability between UNIX, Mac, Windows, VMS,
 * and possibly others.  This is already handled by Perl - it might be
 * interesting to see how much of the code was written in C since Perl is under
 * the GPL and the Artistic license - we might be able to use it.
 */
char *
strcat_filename_onto_homedir (const char *dir, const char *file)
{
    char *path = Xasprintf ("%s/%s", dir, file);
    return path;
}

/* See cvs.h for description.  On unix this does nothing, because the
   shell expands the wildcards.  */
void
expand_wild (int argc, char **argv, int *pargc, char ***pargv)
{
    int i;
    if (size_overflow_p (xtimes (argc, sizeof (char *)))) {
	*pargc = 0;
	*pargv = NULL;
	error (0, 0, "expand_wild: too many arguments");
	return;
    }
    *pargc = argc;
    *pargv = xnmalloc (argc, sizeof (char *));
    for (i = 0; i < argc; ++i)
	(*pargv)[i] = xstrdup (argv[i]);
}



static char *tmpdir_env;

/* Return path to temp directory.
 */
const char *
get_system_temp_dir (void)
{
    if (!tmpdir_env) tmpdir_env = getenv (TMPDIR_ENV);
    return tmpdir_env;
}



void
push_env_temp_dir (void)
{
    const char *tmpdir = get_cvs_tmp_dir ();
    if (tmpdir_env && strcmp (tmpdir_env, tmpdir))
	setenv (TMPDIR_ENV, tmpdir, 1);
}
