/*
 * Copyright (c) 1997 Erez Zadok
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *      %W% (Berkeley) %G%
 *
 * $Id: xdr_mountres3.c,v 1.1.1.1 1997/07/24 21:20:09 christos Exp $
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amu.h>

/*
 * This ifdef is a hack: this whole file needs to be compiled
 * only if the system has NFS V3 and does not have the xdr_mountres3
 * function.  Autoconf should pick this source file to compile only
 * if these two conditions apply.
 */
#ifdef HAVE_FS_NFS3
bool_t
xdr_fhandle3(XDR *xdrs, fhandle3 *objp)
{
  long *buf;

  if (!xdr_bytes(xdrs,
		 (char **) &objp->fhandle3_val,
		 (u_int *) &objp->fhandle3_len,
		 FHSIZE3))
    return (FALSE);
  return (TRUE);
}


bool_t
xdr_mountstat3(XDR *xdrs, mountstat3 *objp)
{
  long *buf;

  if (!xdr_enum(xdrs, (enum_t *)objp))
    return (FALSE);
  return (TRUE);
}


bool_t
xdr_mountres3_ok(XDR *xdrs, mountres3_ok *objp)
{
  long *buf;

  if (!xdr_fhandle3(xdrs, &objp->fhandle))
    return (FALSE);
  if (!xdr_array(xdrs,
		 (char **)&objp->auth_flavors.auth_flavors_val,
		 (u_int *) &objp->auth_flavors.auth_flavors_len,
		 ~0,
		 sizeof (int),
		 (xdrproc_t) xdr_int))
    return (FALSE);
  return (TRUE);
}


bool_t
xdr_mountres3(XDR *xdrs, mountres3 *objp)
{
  long *buf;

  if (!xdr_mountstat3(xdrs, &objp->fhs_status))
    return (FALSE);
  switch (objp->fhs_status) {
  case 0:			/* 0 == MNT_OK or MNT3_OK */
    if (!xdr_mountres3_ok(xdrs, &objp->mountres3_u.mountinfo))
      return (FALSE);
    break;
  }
  return (TRUE);
}

#endif /* HAVE_FS_NFS3 */
