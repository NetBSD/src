/*	$KAME: qop_jobs.c,v 1.2 2002/10/26 07:09:22 kjc Exp $	*/
/*
 * Copyright (c) 2001-2002, by the Rector and Board of Visitors of 
 * the University of Virginia.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, 
 * with or without modification, are permitted provided 
 * that the following conditions are met:
 *
 * Redistributions of source code must retain the above 
 * copyright notice, this list of conditions and the following 
 * disclaimer. 
 *
 * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following 
 * disclaimer in the documentation and/or other materials provided 
 * with the distribution. 
 *
 * Neither the name of the University of Virginia nor the names 
 * of its contributors may be used to endorse or promote products 
 * derived from this software without specific prior written 
 * permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (C) 1999-2000
 *	Sony Computer Science Laboratories, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SONY CSL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL SONY CSL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*                                                                     
 * JoBS - altq prototype implementation                                
 *                                                                     
 * Author: Nicolas Christin <nicolas@cs.virginia.edu>
 *
 * JoBS algorithms originally devised and proposed by		       
 * Nicolas Christin and Jorg Liebeherr.
 * Grateful Acknowledgments to Tarek Abdelzaher for his help and       
 * comments, and to Kenjiro Cho for some helpful advice.
 * Contributed by the Multimedia Networks Group at the University
 * of Virginia. 
 *
 * http://qosbox.cs.virginia.edu
 *                                                                      
 */ 							               

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <syslog.h>
#include <netdb.h>

#include <altq/altq.h>
#include <altq/altq_var.h>
#include <altq/altq_jobs.h>
#include "altq_qop.h"
#include "qop_jobs.h"
#define	SCALE_LOSS 32

#if 0
static int qop_jobs_enable_hook(struct ifinfo *);
#endif
static int qop_jobs_delete_class_hook(struct classinfo *);

static int jobs_attach(struct ifinfo *);
static int jobs_detach(struct ifinfo *);
static int jobs_clear(struct ifinfo *);
static int jobs_enable(struct ifinfo *);
static int jobs_disable(struct ifinfo *);
static int jobs_add_class(struct classinfo *);
static int jobs_modify_class(struct classinfo *, void *);
static int jobs_delete_class(struct classinfo *);
static int jobs_add_filter(struct fltrinfo *);
static int jobs_delete_filter(struct fltrinfo *);

#define JOBS_DEVICE	"/dev/altq/jobs"

static int jobs_fd = -1;
static int jobs_refcount = 0;

static struct qdisc_ops jobs_qdisc = {
	ALTQT_JOBS,
	"jobs",
	jobs_attach,
	jobs_detach,
	jobs_clear,
	jobs_enable,
	jobs_disable,
	jobs_add_class,
	jobs_modify_class,
	jobs_delete_class,
	jobs_add_filter,
	jobs_delete_filter
};

/*
 * parser interface
 */
#define EQUAL(s1, s2)	(strcmp((s1), (s2)) == 0)

int
jobs_interface_parser(const char *ifname, int argc, char **argv)
{
	u_int	bandwidth = 100000000;	/* 100 Mbps */
	u_int	tbrsize = 0;
	int	qlimit = 200; /* 200 packets */
	int separate = 0; /* by default: shared buffer */

	/*
	 * process options
	 */
	while (argc > 0) {
		if (EQUAL(*argv, "bandwidth")) {
			argc--; argv++;
			if (argc > 0)
				bandwidth = atobps(*argv);
		} else if (EQUAL(*argv, "tbrsize")) {
			argc--; argv++;
			if (argc > 0)
				tbrsize = atobytes(*argv);
		} else if (EQUAL(*argv, "qlimit")) {
			argc--; argv++;
			if (argc > 0)
				qlimit = (int)strtol(*argv, NULL, 0);
		} else if (EQUAL(*argv, "separate")) {
			argc--; argv++;
			separate = 1;
		} else if (EQUAL(*argv, "jobs")) {
			/* just skip */
		} else {
			LOG(LOG_ERR, 0, "Unknown keyword '%s'", *argv);
			return (0);
		}
		argc--; argv++;
	}
  
	if (qcmd_tbr_register(ifname, bandwidth, tbrsize) != 0)
		return (0);
  
	if (qcmd_jobs_add_if(ifname, bandwidth, qlimit, separate) != 0)
		return (0);
	return (1);
}

int
jobs_class_parser(const char *ifname, const char *class_name,
		  const char *parent_name, int argc, char **argv)
{
	int64_t adc, rdc, alc, rlc, arc;
	int	pri = 0;
	int	flags = 0;
	int	error;

	/* disable everything by default */
	adc = -1;
	rdc = -1;
	arc = -1;
	alc = -1;
	rlc = -1;

	while (argc > 0) {
		if (EQUAL(*argv, "priority")) {
			argc--; argv++;
			if (argc > 0) {
				if (strtol(*argv, NULL, 0) < 0)
					pri = 0;
				else
					pri = strtol(*argv, NULL, 0);
			}	
		} else if (EQUAL(*argv, "adc")) {
			argc--; argv++;
			if (argc > 0) {
				if (strtol(*argv, NULL, 0) < 0)
					adc = -1;
				else
					adc = strtol(*argv, NULL, 0);
			}      
		} else if (EQUAL(*argv, "rdc")) {
			argc--; argv++;
			if (argc > 0) {
				if (strtol(*argv, NULL, 0) < 0)
					rdc = -1;
				else
					rdc = strtol(*argv, NULL, 0);
			}
		} else if (EQUAL(*argv, "alc")) {
			argc--; argv++;
			if (argc > 0) {
				if (strtol(*argv, NULL, 0) < 0)
					alc = -1;
				else
					alc = (int64_t)(strtod(*argv, NULL)
					    * ((int64_t)1 << SCALE_LOSS));
				/*
				 * alc is given in fraction of 1, convert it
				 * to a fraction of 2^(SCALE_LOSS)
				 */
			}
		} else if (EQUAL(*argv, "rlc")) {
			argc--; argv++;
			if (argc > 0) {
				if (strtol(*argv, NULL, 0) < 0)
					rlc = -1;
				else
					rlc = strtol(*argv, NULL, 0);
			}
		} else if (EQUAL(*argv, "arc")) {
			argc--; argv++;
			if (argc > 0) {
				if (EQUAL(*argv,"-1"))
					arc = -1;
				else
					arc = atobps(*argv);
			}
		} else if (EQUAL(*argv, "default")) {
			flags |= JOCF_DEFAULTCLASS;
		} else {
			LOG(LOG_ERR, 0,
			    "Unknown keyword '%s' in %s, line %d",
			    *argv, altqconfigfile, line_no);
			return (0);
		}
		argc--; argv++;
	}
  
	error = qcmd_jobs_add_class(ifname, class_name, pri,
	    adc, rdc, alc, rlc, arc, flags);

	if (error) {
		LOG(LOG_ERR, errno, "jobs_class_parser: %s",
		    qoperror(error));
		return (0);
	}
	return (1);
}

/*
 * qcmd api
 */
int
qcmd_jobs_add_if(const char *ifname, u_int bandwidth, int qlimit, int separate)
{
	int error;
  
	error = qop_jobs_add_if(NULL, ifname, bandwidth, qlimit, separate);
	if (error != 0)
		LOG(LOG_ERR, errno, "%s: can't add jobs on interface '%s'",
		    qoperror(error), ifname);
	return (error);
}

int
qcmd_jobs_add_class(const char *ifname, const char *class_name, int pri,
    int64_t adc, int64_t rdc, int64_t alc, int64_t rlc, int64_t arc,
    int flags)
{
	struct ifinfo *ifinfo;
	int error = 0;
	char name_adc[20],name_alc[20],name_rdc[20],name_rlc[20],name_arc[20];

	if ((ifinfo = ifname2ifinfo(ifname)) == NULL)
		error = QOPERR_BADIF;
	if (error == 0)
		error = qop_jobs_add_class(NULL, class_name, ifinfo, 
		    pri, adc, rdc, alc, rlc, arc, flags);
	if (error != 0)
		LOG(LOG_ERR, errno,
		    "jobs: %s: can't add class '%s' on interface '%s'",
		    qoperror(error), class_name, ifname);
	else {
		if (adc > 0)
			sprintf(name_adc,"%.3f ms",(double)adc/1000.);
		else
			sprintf(name_adc,"N/A");
		if (alc > 0)
			sprintf(name_alc,"%.2f%%",
			    (double)100*alc/((u_int64_t)1 << SCALE_LOSS));
		else
			sprintf(name_alc,"N/A");
		if (rdc > 0)
			sprintf(name_rdc,"%d",(int)rdc);
		else
			sprintf(name_rdc,"N/A");
		if (rlc > 0)
			sprintf(name_rlc,"%d",(int)rlc);
		else
			sprintf(name_rlc,"N/A");
		if (arc > 0)
			sprintf(name_arc,"%.2f Mbps",(double)arc/1000000.);
		else
			sprintf(name_arc,"N/A");
	      
		LOG(LOG_INFO, 0, 
		    "added '%s' (pri=%d,adc=%s,rdc=%s,alc=%s,rlc=%s,arc=%s) on interface '%s'\n",
		    class_name,pri,name_adc,name_rdc,name_alc,name_rlc,name_arc,ifname);
	}
	return (error);
}

int
qcmd_jobs_modify_class(const char *ifname, const char *class_name, int pri, 
    int64_t adc, int64_t rdc, int64_t alc, int64_t rlc, int64_t arc)
{
	struct ifinfo *ifinfo;
	struct classinfo *clinfo;
  
	if ((ifinfo = ifname2ifinfo(ifname)) == NULL)
		return (QOPERR_BADIF);

	if ((clinfo = clname2clinfo(ifinfo, class_name)) == NULL)
		return (QOPERR_BADCLASS);

	return qop_jobs_modify_class(clinfo, pri, adc, rdc, alc, rlc, arc);
}

/*
 * qop api
 */
int
qop_jobs_add_if(struct ifinfo **rp, const char *ifname,
    u_int bandwidth, int qlimit, int separate)
{
	struct ifinfo *ifinfo = NULL;
	struct jobs_ifinfo *jobs_ifinfo;
	int error;
  
	if ((jobs_ifinfo = calloc(1, sizeof(*jobs_ifinfo))) == NULL)
		return (QOPERR_NOMEM);
	jobs_ifinfo->qlimit   = qlimit;
	jobs_ifinfo->separate = separate;

	error = qop_add_if(&ifinfo, ifname, bandwidth,
	    &jobs_qdisc, jobs_ifinfo);
	if (error != 0) {
		free(jobs_ifinfo);
		return (error);
	}

	if (rp != NULL)
		*rp = ifinfo;
	return (0);
}

int 
qop_jobs_add_class(struct classinfo **rp, const char *class_name,
    struct ifinfo *ifinfo, int pri,
    int64_t adc, int64_t rdc, int64_t alc, int64_t rlc, int64_t arc,
    int flags)
{
	struct classinfo *clinfo;
	struct jobs_ifinfo *jobs_ifinfo;
	struct jobs_classinfo *jobs_clinfo = NULL;
	int error;

	jobs_ifinfo = ifinfo->private;
	if ((flags & JOCF_DEFAULTCLASS) && jobs_ifinfo->default_class != NULL)
		return (QOPERR_CLASS_INVAL);

	if ((jobs_clinfo = calloc(1, sizeof(*jobs_clinfo))) == NULL) {
		error = QOPERR_NOMEM;
		goto err_ret;
	}

	jobs_clinfo->pri = pri;
	jobs_clinfo->adc = adc;
	jobs_clinfo->rdc = rdc;
	jobs_clinfo->alc = alc;
	jobs_clinfo->rlc = rlc;
	jobs_clinfo->arc = arc;
	jobs_clinfo->flags = flags;

	if ((error = qop_add_class(&clinfo, class_name, ifinfo, NULL,
	    jobs_clinfo)) != 0)
		goto err_ret;

	/* set delete hook */
	clinfo->delete_hook = qop_jobs_delete_class_hook;
  
	if (flags & JOCF_DEFAULTCLASS)
		jobs_ifinfo->default_class = clinfo;

	if (rp != NULL)
		*rp = clinfo;
	return (0);

err_ret:
	if (jobs_clinfo != NULL) {
		free(jobs_clinfo);
		clinfo->private = NULL;
	}

	return (error);
}

/*
 * this is called from qop_delete_class() before a class is destroyed
 * for discipline specific cleanup.
 */
static int
qop_jobs_delete_class_hook(struct classinfo *clinfo)
{
	/* in fact this function doesn't do anything
	 * i'm not sure how/when it's used, so I just
	 * leave it here
	 */
	struct jobs_classinfo *jobs_clinfo;

	jobs_clinfo = clinfo->private;
	return (0);
}

int
qop_jobs_modify_class(struct classinfo *clinfo, int pri,
    int64_t adc, int64_t rdc, int64_t alc, int64_t rlc, int64_t arc)
{
	struct jobs_classinfo *jobs_clinfo = clinfo->private;
	int error;
	int pri_old;
	int64_t adc_old, rdc_old, alc_old, rlc_old, arc_old;
 
	pri_old = jobs_clinfo->pri;
	adc_old = jobs_clinfo->adc;
	rdc_old = jobs_clinfo->rdc;
	alc_old = jobs_clinfo->alc;
	rlc_old = jobs_clinfo->rlc;
	arc_old = jobs_clinfo->arc;

	jobs_clinfo = clinfo->private;
	jobs_clinfo->adc = adc;
	jobs_clinfo->rdc = rdc;
	jobs_clinfo->alc = alc;
	jobs_clinfo->rlc = rlc;
	jobs_clinfo->arc = arc;
	jobs_clinfo->pri = pri;

	error = qop_modify_class(clinfo, NULL);
	if (error == 0)
		return (0);
  
	/* modify failed!, restore the old service guarantees */
	jobs_clinfo->adc = adc_old;
	jobs_clinfo->rdc = rdc_old;
	jobs_clinfo->alc = alc_old;
	jobs_clinfo->rlc = rlc_old;
	jobs_clinfo->arc = arc_old;
	jobs_clinfo->pri = pri_old;
	return (error);
}

#if 0
/*
 * sanity check at enabling jobs:
 *  there must one default class for an interface
 */
static int
qop_jobs_enable_hook(struct ifinfo *ifinfo)
{
	struct jobs_ifinfo *jobs_ifinfo;
	
	jobs_ifinfo = ifinfo->private;
	if (jobs_ifinfo->default_class == NULL) {
		LOG(LOG_ERR, 0, "jobs: no default class on interface %s!",
		    ifinfo->ifname);
		return (QOPERR_CLASS);
	}
	return (0);
}
#endif

/*
 *  system call interfaces for qdisc_ops
 */
static int
jobs_attach(struct ifinfo *ifinfo)
{
	struct jobs_attach attach;
  
	if (jobs_fd < 0 &&
	    (jobs_fd = open(JOBS_DEVICE, O_RDWR)) < 0 &&
	    (jobs_fd = open_module(JOBS_DEVICE, O_RDWR)) < 0) {
		LOG(LOG_ERR, errno, "JOBS open");
		return (QOPERR_SYSCALL);
	}

	jobs_refcount++;
	memset(&attach, 0, sizeof(attach));
	strncpy(attach.iface.jobs_ifname, ifinfo->ifname, IFNAMSIZ);
	attach.bandwidth = ifinfo->bandwidth;
	attach.qlimit = (u_int)((struct jobs_ifinfo*)ifinfo->private)->qlimit;
	attach.separate = (u_int)((struct jobs_ifinfo*)ifinfo->private)->separate;

	if (ioctl(jobs_fd, JOBS_IF_ATTACH, (struct jobs_attach*) &attach) < 0)
		return (QOPERR_SYSCALL);

#if 1
	LOG(LOG_INFO, 0,
	    "jobs attached to %s (b/w = %d bps, buff = %d pkts [%s])\n",
	    attach.iface.jobs_ifname,
	    (int) attach.bandwidth, (int) attach.qlimit,
	    attach.separate?"separate buffers":"shared buffer");
#endif
	return (0);
}

static int
jobs_detach(struct ifinfo *ifinfo)
{
	struct jobs_interface iface;

	memset(&iface, 0, sizeof(iface));
	strncpy(iface.jobs_ifname, ifinfo->ifname, IFNAMSIZ);

	if (ioctl(jobs_fd, JOBS_IF_DETACH, &iface) < 0)
		return (QOPERR_SYSCALL);

	if (--jobs_refcount == 0) {
		close(jobs_fd);
		jobs_fd = -1;
	}
	return (0);
}

static int
jobs_enable(struct ifinfo *ifinfo)
{
	struct jobs_interface iface;
  
	memset(&iface, 0, sizeof(iface));
	strncpy(iface.jobs_ifname, ifinfo->ifname, IFNAMSIZ);

	if (ioctl(jobs_fd, JOBS_ENABLE, &iface) < 0)
		return (QOPERR_SYSCALL);
	return (0);
}

static int
jobs_disable(struct ifinfo *ifinfo)
{
	struct jobs_interface iface;

	memset(&iface, 0, sizeof(iface));
	strncpy(iface.jobs_ifname, ifinfo->ifname, IFNAMSIZ);

	if (ioctl(jobs_fd, JOBS_DISABLE, &iface) < 0)
		return (QOPERR_SYSCALL);
	return (0);
}

static int
jobs_clear(struct ifinfo *ifinfo)
{
	struct jobs_interface iface;

	memset(&iface, 0, sizeof(iface));
	strncpy(iface.jobs_ifname, ifinfo->ifname, IFNAMSIZ);

	if (ioctl(jobs_fd, JOBS_CLEAR, &iface) < 0)
		return (QOPERR_SYSCALL);
	return (0);
}

static int
jobs_add_class(struct classinfo *clinfo)
{
	struct jobs_add_class class_add;
	struct jobs_classinfo *jobs_clinfo;
	struct jobs_ifinfo *jobs_ifinfo;

	jobs_ifinfo = clinfo->ifinfo->private;
	jobs_clinfo = clinfo->private;

	memset(&class_add, 0, sizeof(class_add));
	strncpy(class_add.iface.jobs_ifname, clinfo->ifinfo->ifname, IFNAMSIZ);
	class_add.pri = jobs_clinfo->pri;
	class_add.cl_adc = jobs_clinfo->adc;
	class_add.cl_rdc = jobs_clinfo->rdc;
	class_add.cl_alc = jobs_clinfo->alc;
	class_add.cl_rlc = jobs_clinfo->rlc;
	class_add.cl_arc = jobs_clinfo->arc;
	class_add.flags = jobs_clinfo->flags;
	if (ioctl(jobs_fd, JOBS_ADD_CLASS, &class_add) < 0) {
		clinfo->handle = JOBS_NULLCLASS_HANDLE;
		return (QOPERR_SYSCALL);
	}
	clinfo->handle = class_add.class_handle;
	return (0);
}

static int
jobs_modify_class(struct classinfo *clinfo, void *arg)
{
	struct jobs_modify_class class_mod;
	struct jobs_classinfo *jobs_clinfo;

	jobs_clinfo = clinfo->private;

	memset(&class_mod, 0, sizeof(class_mod));
	strncpy(class_mod.iface.jobs_ifname, clinfo->ifinfo->ifname, IFNAMSIZ);
	class_mod.class_handle = clinfo->handle;

	class_mod.pri = jobs_clinfo->pri;
	class_mod.cl_adc = jobs_clinfo->adc;
	class_mod.cl_rdc = jobs_clinfo->rdc;
	class_mod.cl_alc = jobs_clinfo->alc;
	class_mod.cl_rlc = jobs_clinfo->rlc;
	class_mod.cl_arc = jobs_clinfo->arc;
	class_mod.flags = jobs_clinfo->flags;

	if (ioctl(jobs_fd, JOBS_MOD_CLASS, &class_mod) < 0)
		return (QOPERR_SYSCALL);
	return (0);
}

static int
jobs_delete_class(struct classinfo *clinfo)
{
	struct jobs_delete_class class_delete;
  
	if (clinfo->handle == JOBS_NULLCLASS_HANDLE)
		return (0);

	memset(&class_delete, 0, sizeof(class_delete));
	strncpy(class_delete.iface.jobs_ifname, clinfo->ifinfo->ifname,
	    IFNAMSIZ);
	class_delete.class_handle = clinfo->handle;

	if (ioctl(jobs_fd, JOBS_DEL_CLASS, &class_delete) < 0)
		return (QOPERR_SYSCALL);
	return (0);
}

static int
jobs_add_filter(struct fltrinfo *fltrinfo)
{
	struct jobs_add_filter fltr_add;

	memset(&fltr_add, 0, sizeof(fltr_add));
	strncpy(fltr_add.iface.jobs_ifname, fltrinfo->clinfo->ifinfo->ifname,
	    IFNAMSIZ);
	fltr_add.class_handle = fltrinfo->clinfo->handle;
	fltr_add.filter = fltrinfo->fltr;

	if (ioctl(jobs_fd, JOBS_ADD_FILTER, &fltr_add) < 0)
		return (QOPERR_SYSCALL);
	fltrinfo->handle = fltr_add.filter_handle;
	return (0);
}

static int
jobs_delete_filter(struct fltrinfo *fltrinfo)
{
	struct jobs_delete_filter fltr_del;

	memset(&fltr_del, 0, sizeof(fltr_del));
	strncpy(fltr_del.iface.jobs_ifname, fltrinfo->clinfo->ifinfo->ifname,
	    IFNAMSIZ);
	fltr_del.filter_handle = fltrinfo->handle;

	if (ioctl(jobs_fd, JOBS_DEL_FILTER, &fltr_del) < 0)
		return (QOPERR_SYSCALL);
	return (0);
}
