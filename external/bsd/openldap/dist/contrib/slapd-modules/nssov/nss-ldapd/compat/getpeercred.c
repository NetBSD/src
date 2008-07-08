/*
   getpeercred.h - function for determining information about the
                   other end of a unix socket
   This file is part of the nss-ldapd library.

   Copyright (C) 2008 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#ifdef HAVE_SYS_UCRED_H
#include <sys/ucred.h>
#endif /* HAVE SYS_UCRED_H */
#include <errno.h>
#ifdef HAVE_UCRED_H
#include <ucred.h>
#endif /* HAVE_UCRED_H */

#include "getpeercred.h"

/* Note: most of this code is untested, except for the first
         implementation (it may even fail to compile) */

int getpeercred(int sock,uid_t *uid,gid_t *gid,pid_t *pid)
{
#if defined(SO_PEERCRED)
  socklen_t l;
  struct ucred cred;
  /* initialize client information (in case getsockopt() breaks) */
  cred.pid=(pid_t)0;
  cred.uid=(uid_t)-1;
  cred.gid=(gid_t)-1;
  /* look up process information from peer */
  l=(socklen_t)sizeof(struct ucred);
  if (getsockopt(sock,SOL_SOCKET,SO_PEERCRED,&cred,&l) < 0)
    return -1; /* errno already set */
  /* return the data */
  if (uid!=NULL) *uid=cred.uid;
  if (gid!=NULL) *gid=cred.gid;
  if (pid!=NULL) *pid=cred.pid;
  return 0;
#elif defined(LOCAL_PEERCRED)
  socklen_t l;
  struct xucred cred;
  /* look up process information from peer */
  l=(socklen_t)sizeof(struct xucred);
  if (getsockopt(sock,0,LOCAL_PEERCRED,&cred,&l) < 0)
    return -1; /* errno already set */
  if (cred.cr_version!=XUCRED_VERSION)
  {
    errno=EINVAL;
    return -1;
  }
  /* return the data */
  if (uid!=NULL) *uid=cred.uid;
  if (gid!=NULL) *gid=cred.gid;
  if (pid!=NULL) *pid=(pid_t)-1;
  return 0;
#elif defined(HAVE_GETPEERUCRED)
  ucred_t *cred=NULL;
  if (getpeerucred(client,&cred))
    return -1;
  /* save the data */
  if (uid!=NULL) *uid=ucred_geteuid(&cred);
  if (gid!=NULL) *gid=ucred_getegid(&cred);
  if (pid!=NULL) *pid=ucred_getpid(&cred);
  /* free cred and return */
  ucred_free(&ucred);
  return 0;
#elif defined(HAVE_GETPEEREID)
  uid_t tuid;
  gid_t tgid;
  if (uid==NULL) uid=&tuid;
  if (gid==NULL) gid=&tguid;
  if (getpeereid(sock,uid,gid))
    return -1;
  /* return the data */
  if (uid!=NULL) *uid=cred.uid;
  if (gid!=NULL) *gid=cred.gid;
  if (pid!=NULL) *pid=-1; /* we return a -1 pid because we have no usable pid */
  return 0;
#else
  /* nothing found that is supported */
  errno=ENOSYS;
  return -1;
#endif
}
