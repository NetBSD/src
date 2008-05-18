/*	$NetBSD: dirent_private.h,v 1.1.18.1 2008/05/18 12:30:15 yamt Exp $	*/

/*
 * One struct _dirpos is malloced to describe the current directory
 * position each time telldir is called. It records the current magic 
 * cookie returned by getdirentries and the offset within the buffer
 * associated with that return value.
 */
struct dirpos {
	struct dirpos *dp_next;	/* next structure in list */
	off_t	dp_seek;	/* magic cookie returned by getdirentries */
	long	dp_loc;		/* offset of entry in buffer */
};

struct _dirdesc;
void _seekdir_unlocked(struct _dirdesc *, long);
long _telldir_unlocked(struct _dirdesc *);
#ifndef __LIBC12_SOURCE__
struct dirent;
struct dirent *_readdir_unlocked(struct _dirdesc *, int)
    __RENAME(___readdir_unlocked50);
#endif
