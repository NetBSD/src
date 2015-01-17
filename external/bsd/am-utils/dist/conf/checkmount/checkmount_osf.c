/*	$NetBSD: checkmount_osf.c,v 1.1.1.3 2015/01/17 16:34:16 christos Exp $	*/

/*
 * Copyright (c) 1997-2014 Erez Zadok
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
 *
 *
 * File: am-utils/conf/checkmount/checkmount_osf.c
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>

extern int is_same_host(char *name1, char *name2, struct in_addr addr2);


int
fixmount_check_mount(char *host, struct in_addr hostaddr, char *path)
{
  int nentries, i;
  struct statfs *fslist;
  int found = 0;

  nentries = getmntinfo(&fslist, MNT_NOWAIT);
  if (nentries <= 0) {
    perror("getmntinfo");
    exit(1);
  }

  for (i = 0; !found && (i < nentries); i++) {
    char *delim;

    /*
     * Apparently two forms of nfs mount syntax are
     * accepted: host:/path or /path@host
     */
    if ((delim = strchr(fslist[i].f_mntfromname, ':'))) {
      *delim = '\0';
      if ((STREQ(delim + 1, path) ||
	   STREQ(fslist[i].f_mntonname, path)) &&
	  is_same_host(fslist[i].f_mntfromname,
		       host, hostaddr))
	  found = 1;
    } else if ((delim = strchr(fslist[i].f_mntfromname, '@'))) {
      *delim = '\0';
      if ((STREQ(fslist[i].f_mntfromname, path) ||
	   STREQ(fslist[i].f_mntonname, path)) &&
	  is_same_host(delim + 1, host, hostaddr))
	  found = 1;
    }
  }

  return found;
}
