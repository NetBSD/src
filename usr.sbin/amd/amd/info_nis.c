/*
 * Copyright (c) 1997 Erez Zadok
 * Copyright (c) 1989 Jan-Simon Pendry
 * Copyright (c) 1989 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1989 The Regents of the University of California.
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
 * $Id: info_nis.c,v 1.6 1997/07/24 23:16:36 christos Exp $
 *
 */

/*
 * Get info from NIS map
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amd.h>

struct nis_callback_data {
  mnt_map *ncd_m;
  char *ncd_map;
  void (*ncd_fn) ();
};


/*
 * Figure out the nis domain name
 */
static int
determine_nis_domain(void)
{
  static int nis_not_running = 0;

  char default_domain[YPMAXDOMAIN];

  if (nis_not_running)
    return ENOENT;

  if (getdomainname(default_domain, sizeof(default_domain)) < 0) {
    nis_not_running = 1;
    plog(XLOG_ERROR, "getdomainname: %m");
    return EIO;
  }
  if (!*default_domain) {
    nis_not_running = 1;
    plog(XLOG_WARNING, "NIS domain name is not set.  NIS ignored.");
    return ENOENT;
  }
  gopt.nis_domain = strdup(default_domain);

  return 0;
}


/*
 * Callback from yp_all
 */
static int
callback(int status, char *key, int kl, char *val, int vl, char *data)
{
  struct nis_callback_data *ncdp = (struct nis_callback_data *) data;

  if (status == YP_TRUE) {

    /*
     * Add to list of maps
     */
    char *kp = strnsave(key, kl);
    char *vp = strnsave(val, vl);
    (*ncdp->ncd_fn) (ncdp->ncd_m, kp, vp);

    /*
     * We want more ...
     */
    return FALSE;

  } else {

    /*
     * NOMORE means end of map - otherwise log error
     */
    if (status != YP_NOMORE) {
      /*
       * Check what went wrong
       */
      int e = ypprot_err(status);

#ifdef DEBUG
      plog(XLOG_ERROR, "yp enumeration of %s: %s, status=%d, e=%d",
	   ncdp->ncd_map, yperr_string(e), status, e);
#else /* not DEBUG */
      plog(XLOG_ERROR, "yp enumeration of %s: %s", ncdp->ncd_map, yperr_string(e));
#endif /* not DEBUG */
    }
    return TRUE;
  }
}


int
nis_reload(mnt_map *m, char *map, void (*fn) (int, char *, int, char *, int, char *))
{
  struct ypall_callback cbinfo;
  int error;
  struct nis_callback_data data;

  if (!gopt.nis_domain) {
    error = determine_nis_domain();
    if (error)
      return error;
  }
  data.ncd_m = m;
  data.ncd_map = map;
  data.ncd_fn = fn;
  cbinfo.data = (voidp) &data;
  cbinfo.foreach = (int (*)()) callback;


  /*
   * If you are using NIS and your yp_all function is "broken", you have to
   * get it fixed.  The bug in yp_all() is that it does not close a TCP
   * connection to ypserv, and this ypserv runs out of open filedescriptors,
   * getting into an infinite loop, thus all YP clients enevtually unbind
   * and hang too.
   */
  error = yp_all(gopt.nis_domain, map, &cbinfo);

  if (error)
    plog(XLOG_ERROR, "error grabbing nis map of %s: %s", map, yperr_string(ypprot_err(error)));

  return error;
}


/*
 * Try to locate a key using NIS.
 */
int
nis_search(mnt_map *m, char *map, char *key, char **val, time_t *tp)
{
  int outlen;
  int res;
  YP_ORDER_OUTORDER_TYPE order;

  /*
   * Make sure domain initialised
   */
  if (!gopt.nis_domain) {
    int error = determine_nis_domain();
    if (error)
      return error;
  }

  /*
   * Check if map has changed
   */
  if (yp_order(gopt.nis_domain, map, &order))
    return EIO;
  if ((time_t) order > *tp) {
    *tp = (time_t) order;
    return -1;
  }

  /*
   * Lookup key
   */
  res = yp_match(gopt.nis_domain, map, key, strlen(key), val, &outlen);

  /*
   * Do something interesting with the return code
   */
  switch (res) {
  case 0:
    return 0;

  case YPERR_KEY:
    return ENOENT;

  default:
    plog(XLOG_ERROR, "%s: %s", map, yperr_string(res));
    return EIO;
  }
}


int
nis_init(mnt_map *m, char *map, time_t *tp)
{
  YP_ORDER_OUTORDER_TYPE order;

  if (!gopt.nis_domain) {
    int error = determine_nis_domain();
    if (error)
      return error;
  }
  /*
   * To see if the map exists, try to find
   * a master for it.
   */
  if (yp_order(gopt.nis_domain, map, &order))
    return ENOENT;
  *tp = (time_t) order;
#ifdef DEBUG
  dlog("NIS master for %s@%s has order %d", map, gopt.nis_domain, order);
#endif /* DEBUG */
  return 0;
}


int
nis_mtime(mnt_map *m, char *map, time_t *tp)
{
  return nis_init(m,map, tp);
}
