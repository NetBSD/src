/*	$NetBSD: transp_tli.c,v 1.1.1.6 2003/03/09 01:13:33 christos Exp $	*/

/*
 * Copyright (c) 1997-2003 Erez Zadok
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
 *    must display the following acknowledgment:
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
 *
 * Id: transp_tli.c,v 1.14 2002/12/27 22:44:03 ezk Exp
 *
 * TLI specific utilities.
 *      -Erez Zadok <ezk@cs.columbia.edu>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amu.h>

struct netconfig *nfsncp;


/*
 * find the IP address that can be used to connect to the local host
 */
void
amu_get_myaddress(struct in_addr *iap)
{
  int ret;
  voidp handlep;
  struct netconfig *ncp;
  struct nd_addrlist *addrs = (struct nd_addrlist *) NULL;
  struct nd_hostserv service;

  handlep = setnetconfig();
  ncp = getnetconfig(handlep);
  service.h_host = HOST_SELF_CONNECT;
  service.h_serv = (char *) NULL;

  ret = netdir_getbyname(ncp, &service, &addrs);

  if (ret || !addrs || addrs->n_cnt < 1) {
    plog(XLOG_FATAL, "cannot get local host address. using 127.0.0.1");
    iap->s_addr = 0x7f000001;
  } else {
    /*
     * XXX: there may be more more than one address for this local
     * host.  Maybe something can be done with those.
     */
    struct sockaddr_in *sinp = (struct sockaddr_in *) addrs->n_addrs[0].buf;
    iap->s_addr = htonl(sinp->sin_addr.s_addr);
  }

  endnetconfig(handlep);	/* free's up internal resources too */
  netdir_free((voidp) addrs, ND_ADDRLIST);
}


/*
 * How to bind to reserved ports.
 * TLI handle (socket) and port version.
 */
int
bind_resv_port(int td, u_short *pp)
{
  int rc = -1, port;
  struct t_bind *treq, *tret;
  struct sockaddr_in *sin;

  treq = (struct t_bind *) t_alloc(td, T_BIND, T_ADDR);
  if (!treq) {
    plog(XLOG_ERROR, "t_alloc req");
    return -1;
  }
  tret = (struct t_bind *) t_alloc(td, T_BIND, T_ADDR);
  if (!tret) {
    t_free((char *) treq, T_BIND);
    plog(XLOG_ERROR, "t_alloc ret");
    return -1;
  }
  memset((char *) treq->addr.buf, 0, treq->addr.len);
  sin = (struct sockaddr_in *) treq->addr.buf;
  sin->sin_family = AF_INET;
  treq->qlen = 0;
  treq->addr.len = treq->addr.maxlen;
  errno = EADDRINUSE;
  port = IPPORT_RESERVED;

  do {
    --port;
    sin->sin_port = htons(port);
    rc = t_bind(td, treq, tret);
    if (rc < 0) {
      plog(XLOG_ERROR, "t_bind");
    } else {
      if (memcmp(treq->addr.buf, tret->addr.buf, tret->addr.len) == 0)
	break;
      else
	t_unbind(td);
    }
  } while ((rc < 0 || errno == EADDRINUSE) && (int) port > IPPORT_RESERVED / 2);

  if (pp) {
    if (rc == 0)
      *pp = port;
    else
      plog(XLOG_ERROR, "could not t_bind to any reserved port");
  }
  t_free((char *) tret, T_BIND);
  t_free((char *) treq, T_BIND);
  return rc;
}


/*
 * How to bind to reserved ports.
 * (port-only) version.
 */
int
bind_resv_port2(u_short *pp)
{
  int td, rc = -1, port;
  struct t_bind *treq, *tret;
  struct sockaddr_in *sin;
  extern char *t_errlist[];
  extern int t_errno;
  struct netconfig *nc = (struct netconfig *) NULL;
  voidp nc_handle;

  if ((nc_handle = setnetconfig()) == (voidp) NULL) {
    plog(XLOG_ERROR, "Cannot rewind netconfig: %s", nc_sperror());
    return -1;
  }
  /*
   * Search the netconfig table for INET/UDP.
   * This loop will terminate if there was an error in the /etc/netconfig
   * file or if you reached the end of the file without finding the udp
   * device.  Either way your machine has probably far more problems (for
   * example, you cannot have nfs v2 w/o UDP).
   */
  while (1) {
    if ((nc = getnetconfig(nc_handle)) == (struct netconfig *) NULL) {
      plog(XLOG_ERROR, "Error accessing getnetconfig: %s", nc_sperror());
      endnetconfig(nc_handle);
      return -1;
    }
    if (STREQ(nc->nc_protofmly, NC_INET) &&
	STREQ(nc->nc_proto, NC_UDP))
      break;
  }

  /*
   * This is the primary reason for the getnetconfig code above: to get the
   * correct device name to udp, and t_open a descriptor to be used in
   * t_bind below.
   */
  td = t_open(nc->nc_device, O_RDWR, (struct t_info *) 0);
  endnetconfig(nc_handle);

  if (td < 0) {
    plog(XLOG_ERROR, "t_open failed: %d: %s", t_errno, t_errlist[t_errno]);
    return -1;
  }
  treq = (struct t_bind *) t_alloc(td, T_BIND, T_ADDR);
  if (!treq) {
    plog(XLOG_ERROR, "t_alloc req");
    return -1;
  }
  tret = (struct t_bind *) t_alloc(td, T_BIND, T_ADDR);
  if (!tret) {
    t_free((char *) treq, T_BIND);
    plog(XLOG_ERROR, "t_alloc ret");
    return -1;
  }
  memset((char *) treq->addr.buf, 0, treq->addr.len);
  sin = (struct sockaddr_in *) treq->addr.buf;
  sin->sin_family = AF_INET;
  treq->qlen = 0;
  treq->addr.len = treq->addr.maxlen;
  errno = EADDRINUSE;
  port = IPPORT_RESERVED;

  do {
    --port;
    sin->sin_port = htons(port);
    rc = t_bind(td, treq, tret);
    if (rc < 0) {
      plog(XLOG_ERROR, "t_bind for port %d: %s", port, t_errlist[t_errno]);
    } else {
      if (memcmp(treq->addr.buf, tret->addr.buf, tret->addr.len) == 0)
	break;
      else
	t_unbind(td);
    }
  } while ((rc < 0 || errno == EADDRINUSE) && (int) port > IPPORT_RESERVED / 2);

  if (pp && rc == 0)
    *pp = port;
  t_free((char *) tret, T_BIND);
  t_free((char *) treq, T_BIND);
  return rc;
}


/*
 * close a descriptor, TLI style
 */
int
amu_close(int fd)
{
  return t_close(fd);
}


/*
 * Create an rpc client attached to the mount daemon.
 */
CLIENT *
get_mount_client(char *host, struct sockaddr_in *unused_sin, struct timeval *tv, int *sock, u_long mnt_version)
{
  CLIENT *client;
  struct netbuf nb;
  struct netconfig *nc = NULL;
  struct sockaddr_in sin;

  nb.maxlen = sizeof(sin);
  nb.buf = (char *) &sin;

  /*
   * First try a TCP handler
   */

  /*
   * Find mountd address on TCP
   */
  if ((nc = getnetconfigent(NC_TCP)) == NULL) {
    plog(XLOG_ERROR, "getnetconfig for tcp failed: %s", nc_sperror());
    goto tryudp;
  }
  if (!rpcb_getaddr(MOUNTPROG, mnt_version, nc, &nb, host)) {
    /*
     * don't print error messages here, since mountd might legitimately
     * serve udp only
     */
    goto tryudp;
  }
  /*
   * Create privileged TCP socket
   */
  *sock = t_open(nc->nc_device, O_RDWR, 0);

  if (*sock < 0) {
    plog(XLOG_ERROR, "t_open %s: %m", nc->nc_device);
    goto tryudp;
  }
  if (bind_resv_port(*sock, (u_short *) 0) < 0)
    plog(XLOG_ERROR, "couldn't bind mountd socket to privileged port");

  if ((client = clnt_vc_create(*sock, &nb, MOUNTPROG, mnt_version, 0, 0))
      == (CLIENT *) NULL) {
    plog(XLOG_ERROR, "clnt_vc_create failed");
    t_close(*sock);
    goto tryudp;
  }
  /* tcp succeeded */
  dlog("get_mount_client: using tcp, port %d", sin.sin_port);
  if (nc)
    freenetconfigent(nc);
  return client;

tryudp:
  /* first free possibly previously allocated netconfig entry */
  if (nc)
    freenetconfigent(nc);

  /*
   * TCP failed so try UDP
   */

  /*
   * Find mountd address on UDP
   */
  if ((nc = getnetconfigent(NC_UDP)) == NULL) {
    plog(XLOG_ERROR, "getnetconfig for udp failed: %s", nc_sperror());
    goto badout;
  }
  if (!rpcb_getaddr(MOUNTPROG, mnt_version, nc, &nb, host)) {
    plog(XLOG_ERROR, "%s",
	 clnt_spcreateerror("couldn't get mountd address on udp"));
    goto badout;
  }
  /*
   * Create privileged UDP socket
   */
  *sock = t_open(nc->nc_device, O_RDWR, 0);

  if (*sock < 0) {
    plog(XLOG_ERROR, "t_open %s: %m", nc->nc_device);
    goto badout;		/* neither tcp not udp succeeded */
  }
  if (bind_resv_port(*sock, (u_short *) 0) < 0)
    plog(XLOG_ERROR, "couldn't bind mountd socket to privileged port");

  if ((client = clnt_dg_create(*sock, &nb, MOUNTPROG, mnt_version, 0, 0))
      == (CLIENT *) NULL) {
    plog(XLOG_ERROR, "clnt_dg_create failed");
    t_close(*sock);
    goto badout;		/* neither tcp not udp succeeded */
  }
  if (clnt_control(client, CLSET_RETRY_TIMEOUT, (char *) tv) == FALSE) {
    plog(XLOG_ERROR, "clnt_control CLSET_RETRY_TIMEOUT for udp failed");
    clnt_destroy(client);
    goto badout;		/* neither tcp not udp succeeded */
  }
  /* udp succeeded */
  dlog("get_mount_client: using udp, port %d", sin.sin_port);
  return client;

badout:
  /* failed */
  if (nc)
    freenetconfigent(nc);
  return NULL;
}


/*
 * find the address of the caller of an RPC procedure.
 */
struct sockaddr_in *
amu_svc_getcaller(SVCXPRT *xprt)
{
  struct netbuf *nbp = (struct netbuf *) NULL;

  if ((nbp = svc_getrpccaller(xprt)) != NULL)
    return (struct sockaddr_in *) nbp->buf; /* all OK */

  return NULL;			/* failed */
}


/*
 * register an RPC server
 */
int
amu_svc_register(SVCXPRT *xprt, u_long prognum, u_long versnum, void (*dispatch)(), u_long protocol, struct netconfig *ncp)
{
  return svc_reg(xprt, prognum, versnum, dispatch, ncp);
}


/*
 * Bind NFS to a reserved port.
 */
static int
bindnfs_port(int unused_so, u_short *nfs_portp)
{
  u_short port;
  int error = bind_resv_port2(&port);

  if (error == 0)
    *nfs_portp = port;
  return error;
}


/*
 * Create the nfs service for amd
 * return 0 (TRUE) if OK, 1 (FALSE) if failed.
 */
int
create_nfs_service(int *soNFSp, u_short *nfs_portp, SVCXPRT **nfs_xprtp, void (*dispatch_fxn)(struct svc_req *rqstp, SVCXPRT *transp))
{
  char *nettype = "ticlts";

  nfsncp = getnetconfigent(nettype);
  if (nfsncp == NULL) {
    plog(XLOG_ERROR, "cannot getnetconfigent for %s", nettype);
    /* failed with ticlts, try plain udp (hpux11) */
    nettype = "udp";
    nfsncp = getnetconfigent(nettype);
    if (nfsncp == NULL) {
      plog(XLOG_ERROR, "cannot getnetconfigent for %s", nettype);
      return 1;
    }
  }
  *nfs_xprtp = svc_tli_create(RPC_ANYFD, nfsncp, NULL, 0, 0);
  if (*nfs_xprtp == NULL) {
    plog(XLOG_ERROR, "cannot create nfs tli service for amd");
    return 1;
  }

  /*
   * Get the service file descriptor and check its number to see if
   * the t_open failed.  If it succeeded, then go on to binding to a
   * reserved nfs port.
   */
  *soNFSp = (*nfs_xprtp)->xp_fd;
  if (*soNFSp < 0 || bindnfs_port(*soNFSp, nfs_portp) < 0) {
    plog(XLOG_ERROR, "Can't create privileged nfs port (TLI)");
    return 1;
  }
  if (svc_reg(*nfs_xprtp, NFS_PROGRAM, NFS_VERSION, dispatch_fxn, NULL) != 1) {
    plog(XLOG_ERROR, "could not register amd NFS service");
    return 1;
  }

  return 0;			/* all is well */
}


/*
 * Create the amq service for amd (both TCP and UDP)
 */
int
create_amq_service(int *udp_soAMQp, SVCXPRT **udp_amqpp, struct netconfig **udp_amqncpp, int *tcp_soAMQp, SVCXPRT **tcp_amqpp, struct netconfig **tcp_amqncpp)
{
  /*
   * (partially) create the amq service for amd
   * to be completed further in by caller.
   */

  /* first create the TCP service */
  if (tcp_amqncpp)
    if ((*tcp_amqncpp = getnetconfigent(NC_TCP)) == NULL) {
      plog(XLOG_ERROR, "cannot getnetconfigent for %s", NC_TCP);
      return 1;
    }
  if (tcp_amqpp) {
    *tcp_amqpp = svc_tli_create(RPC_ANYFD, *tcp_amqncpp, NULL, 0, 0);
    if (*tcp_amqpp == NULL) {
      plog(XLOG_ERROR, "cannot create (tcp) tli service for amq");
      return 1;
    }
  }
  if (tcp_soAMQp && tcp_amqpp)
    *tcp_soAMQp = (*tcp_amqpp)->xp_fd;

  /* next create the UDP service */
  if (udp_amqncpp)
    if ((*udp_amqncpp = getnetconfigent(NC_UDP)) == NULL) {
      plog(XLOG_ERROR, "cannot getnetconfigent for %s", NC_UDP);
      return 1;
    }
  if (udp_amqpp) {
    *udp_amqpp = svc_tli_create(RPC_ANYFD, *udp_amqncpp, NULL, 0, 0);
    if (*udp_amqpp == NULL) {
      plog(XLOG_ERROR, "cannot create (udp) tli service for amq");
      return 1;
    }
  }
  if (udp_soAMQp && udp_amqpp)
    *udp_soAMQp = (*udp_amqpp)->xp_fd;

  return 0;			/* all is well */
}


/*
 * Find netconfig info for TCP/UDP device, and fill in the knetconfig
 * structure.  If in_ncp is not NULL, use that instead of defaulting
 * to a TCP/UDP service.  If in_ncp is NULL, then use the service type
 * specified in nc_protoname (which may be either "tcp" or "udp").  If
 * nc_protoname is NULL, default to UDP.
 */
int
get_knetconfig(struct knetconfig **kncpp, struct netconfig *in_ncp, char *nc_protoname)
{
  struct netconfig *ncp = NULL;
  struct stat statbuf;

  if (in_ncp)
    ncp = in_ncp;
  else {
    if (nc_protoname)
      ncp = getnetconfigent(nc_protoname);
    else
      ncp = getnetconfigent(NC_UDP);
  }
  if (!ncp)
    return -2;

  *kncpp = (struct knetconfig *) xzalloc(sizeof(struct knetconfig));
  if (*kncpp == (struct knetconfig *) NULL) {
    if (!in_ncp)
      freenetconfigent(ncp);
    return -3;
  }
  (*kncpp)->knc_semantics = ncp->nc_semantics;
  (*kncpp)->knc_protofmly = strdup(ncp->nc_protofmly);
  (*kncpp)->knc_proto = strdup(ncp->nc_proto);

  if (stat(ncp->nc_device, &statbuf) < 0) {
    plog(XLOG_ERROR, "could not stat() %s: %m", ncp->nc_device);
    XFREE(*kncpp);
    *kncpp = NULL;
    if (!in_ncp)
      freenetconfigent(ncp);
    return -3;			/* amd will end (free not needed) */
  }
  (*kncpp)->knc_rdev = (dev_t) statbuf.st_rdev;
  if (!in_ncp) {		/* free only if argument not passed */
    freenetconfigent(ncp);
    ncp = NULL;
  }
  return 0;
}


/*
 * Free a previously allocated knetconfig structure.
 */
void
free_knetconfig(struct knetconfig *kncp)
{
  if (kncp) {
    if (kncp->knc_protofmly)
      XFREE(kncp->knc_protofmly);
    if (kncp->knc_proto)
      XFREE(kncp->knc_proto);
    XFREE(kncp);
    kncp = (struct knetconfig *) NULL;
  }
}


/* get the best possible NFS version for a host and transport */
static CLIENT *
amu_clnt_create_best_vers(const char *hostname, u_long program, u_long *out_version, u_long low_version, u_long high_version, const char *nettype)
{
  CLIENT *clnt;
  enum clnt_stat rpc_stat;
  struct rpc_err rpcerr;
  struct timeval tv;
  u_long lo, hi;

  /* 3 seconds is more than enough for a LAN */
  tv.tv_sec = 3;
  tv.tv_usec = 0;

#ifdef HAVE_CLNT_CREATE_TIMED
  clnt = clnt_create_timed(hostname, program, high_version, nettype, &tv);
  if (!clnt) {
    plog(XLOG_INFO, "failed to create RPC client to \"%s\" after %d seconds",
	 hostname, (int) tv.tv_sec);
    return NULL;
  }
#else /* not HAVE_CLNT_CREATE_TIMED */
  /* Solaris 2.3 and earlier didn't have clnt_create_timed() */
  clnt = clnt_create(hostname, program, high_version, nettype);
  if (!clnt) {
    plog(XLOG_INFO, "failed to create RPC client to \"%s\"", hostname);
    return NULL;
  }
#endif /* not HAVE_CLNT_CREATE_TIMED */

  rpc_stat = clnt_call(clnt,
		       NULLPROC,
		       (XDRPROC_T_TYPE) xdr_void,
		       NULL,
		       (XDRPROC_T_TYPE) xdr_void,
		       NULL,
		       tv);
  if (rpc_stat == RPC_SUCCESS) {
    *out_version = high_version;
    return clnt;
  }
  while (low_version < high_version) {
    if (rpc_stat != RPC_PROGVERSMISMATCH)
      break;
    clnt_geterr(clnt, &rpcerr);
    lo = rpcerr.re_vers.low;
    hi = rpcerr.re_vers.high;
    if (hi < high_version)
      high_version = hi;
    else
      high_version--;
    if (lo > low_version)
      low_version = lo;
    if (low_version > high_version)
      goto out;

    CLNT_CONTROL(clnt, CLSET_VERS, (char *)&high_version);
    rpc_stat = clnt_call(clnt,
			 NULLPROC,
			 (XDRPROC_T_TYPE) xdr_void,
			 NULL,
			 (XDRPROC_T_TYPE) xdr_void,
			 NULL,
			 tv);
    if (rpc_stat == RPC_SUCCESS) {
      *out_version = high_version;
      return clnt;
    }
  }
  clnt_geterr(clnt, &rpcerr);

out:
  rpc_createerr.cf_stat = rpc_stat;
  rpc_createerr.cf_error = rpcerr;
  clnt_destroy(clnt);
  return NULL;
}


/*
 * Find the best NFS version for a host.
 */
u_long
get_nfs_version(char *host, struct sockaddr_in *sin, u_long nfs_version, const char *proto)
{
  CLIENT *clnt = NULL;
  u_long versout;

  /*
   * If not set or set wrong, then try from NFS_VERS_MAX on down. If
   * set, then try from nfs_version on down.
   */
  if (nfs_version <= 0 || nfs_version > NFS_VERS_MAX) {
    nfs_version = NFS_VERS_MAX;
  }

  if (nfs_version == NFS_VERSION) {
    dlog("get_nfs_version trying NFS(%d,%s) for %s",
	 (int) nfs_version, proto, host);
  } else {
    dlog("get_nfs_version trying NFS(%d-%d,%s) for %s",
	 (int) NFS_VERSION, (int) nfs_version, proto, host);
  }

  /* get the best NFS version, and timeout quickly if remote host is down */
  clnt = amu_clnt_create_best_vers(host, NFS_PROGRAM, &versout,
				   NFS_VERSION, nfs_version, proto);

  if (clnt == NULL) {
    if (nfs_version == NFS_VERSION)
      plog(XLOG_INFO, "get_nfs_version NFS(%d,%s) failed for %s: %s",
	   (int) nfs_version, proto, host, clnt_spcreateerror(""));
    else
      plog(XLOG_INFO, "get_nfs_version NFS(%d-%d,%s) failed for %s: %s",
	   (int) NFS_VERSION, (int) nfs_version, proto, host, clnt_spcreateerror(""));
    return 0;
  }
  clnt_destroy(clnt);

  return versout;
}


#if defined(HAVE_FS_AUTOFS) && defined(AUTOFS_PROG)
/*
 * find the IP address that can be used to connect autofs service to.
 */
static int
get_autofs_address(struct netconfig *ncp, struct t_bind *tbp)
{
  int ret;
  struct nd_addrlist *addrs = (struct nd_addrlist *) NULL;
  struct nd_hostserv service;

  service.h_host = HOST_SELF_CONNECT;
  service.h_serv = "autofs";

  ret = netdir_getbyname(ncp, &service, &addrs);

  if (ret) {
    plog(XLOG_FATAL, "get_autofs_address: cannot get local host address: %s", netdir_sperror());
    goto out;
  }

  /*
   * XXX: there may be more more than one address for this local
   * host.  Maybe something can be done with those.
   */
  tbp->addr.len = addrs->n_addrs->len;
  tbp->addr.maxlen = addrs->n_addrs->len;
  memcpy(tbp->addr.buf, addrs->n_addrs->buf, addrs->n_addrs->len);
  tbp->qlen = 8;		/* arbitrary? who cares really */

  /* all OK */
  netdir_free((voidp) addrs, ND_ADDRLIST);

out:
  return ret;
}


/*
 * Register the autofs service for amd
 */
int
register_autofs_service(char *autofs_conftype, void (*autofs_dispatch)())
{
  struct t_bind *tbp = 0;
  struct netconfig *autofs_ncp;
  SVCXPRT *autofs_xprt = NULL;
  int fd = -1, err = 1;		/* assume failed */

  plog(XLOG_INFO, "registering autofs service: %s", autofs_conftype);
  autofs_ncp = getnetconfigent(autofs_conftype);
  if (autofs_ncp == NULL) {
    plog(XLOG_ERROR, "register_autofs_service: cannot getnetconfigent for %s", autofs_conftype);
    goto out;
  }

  fd = t_open(autofs_ncp->nc_device, O_RDWR, NULL);
  if (fd < 0) {
    plog(XLOG_ERROR, "register_autofs_service: t_open failed (%s)",
	 t_errlist[t_errno]);
    goto out;
  }

  tbp = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR);
  if (!tbp) {
    plog(XLOG_ERROR, "register_autofs_service: t_alloca failed");
    goto out;
  }

  if (get_autofs_address(autofs_ncp, tbp) != 0) {
    plog(XLOG_ERROR, "register_autofs_service: get_autofs_address failed");
    goto out;
  }

  autofs_xprt = svc_tli_create(fd, autofs_ncp, tbp, 0, 0);
  if (autofs_xprt == NULL) {
    plog(XLOG_ERROR, "cannot create autofs tli service for amd");
    goto out;
  }

  rpcb_unset(AUTOFS_PROG, AUTOFS_VERS, autofs_ncp);
  if (svc_reg(autofs_xprt, AUTOFS_PROG, AUTOFS_VERS, autofs_dispatch, autofs_ncp) == FALSE) {
    plog(XLOG_ERROR, "could not register amd AUTOFS service");
    goto out;
  }
  err = 0;
  goto really_out;

out:
  if (autofs_ncp)
    freenetconfigent(autofs_ncp);
  if (autofs_xprt)
    SVC_DESTROY(autofs_xprt);
  else {
    if (fd > 0)
      t_close(fd);
  }

really_out:
  if (tbp)
    t_free((char *) tbp, T_BIND);

  dlog("register_autofs_service: returning %d\n", err);
  return err;
}


int
unregister_autofs_service(char *autofs_conftype)
{
  struct netconfig *autofs_ncp;
  int err = 1;

  plog(XLOG_INFO, "unregistering autofs service listener: %s", autofs_conftype);

  autofs_ncp = getnetconfigent(autofs_conftype);
  if (autofs_ncp == NULL) {
    plog(XLOG_ERROR, "destroy_autofs_service: cannot getnetconfigent for %s", autofs_conftype);
    goto out;
  }

out:
  rpcb_unset(AUTOFS_PROG, AUTOFS_VERS, autofs_ncp);
  return err;
}
#endif /* HAVE_FS_AUTOFS && AUTOFS_PROG */
