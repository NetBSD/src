/*	$NetBSD: getpeereid.h,v 1.3.2.2 2017/04/26 02:52:14 pgoyette Exp $	*/
/* Id: bsd-getpeereid.h,v 1.1 2002/09/12 00:33:02 djm Exp */

#ifndef _BSD_GETPEEREID_H
#define _BSD_GETPEEREID_H

#include <sys/types.h> /* For uid_t, gid_t */

int	 getpeereid(int , uid_t *, gid_t *);

#endif /* _BSD_GETPEEREID_H */
