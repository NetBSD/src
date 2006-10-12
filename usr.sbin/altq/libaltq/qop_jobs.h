/*	$KAME: qop_jobs.h,v 1.1 2002/10/26 06:59:53 kjc Exp $	*/
/*	$Id: qop_jobs.h,v 1.2 2006/10/12 19:59:13 peter Exp $	*/
/*
 * Copyright (c) 2001-2002, by the Rector and Board of Visitors of the 
 * University of Virginia.
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

#include <altq/altq_jobs.h>

/*
 * jobs private ifinfo structure
 */
struct jobs_ifinfo {
	int qlimit;		/* max queue length */
	int separate;
	struct classinfo *default_class;
};

/*
 * jobs private classinfo structure
 */
struct jobs_classinfo {
	int		pri;
	int64_t		adc;
	int64_t		rdc;
	int64_t		alc;
	int64_t		rlc;
	int64_t		arc;
	int		flags;
};


int jobs_interface_parser(const char *ifname, int argc, char **argv);
int jobs_class_parser(const char *ifname, const char *class_name,
    const char *parent_name, int argc, char **argv);
int qcmd_jobs_add_if(const char *ifname, u_int bandwidth, int qlimit, int separate);
int qcmd_jobs_add_class(const char *ifname, const char *class_name, int pri,
    int64_t adc, int64_t rdc, int64_t alc, int64_t rlc, int64_t arc,
    int flags);
int qcmd_jobs_modify_class(const char *ifname, const char *class_name, int pri,
    int64_t adc, int64_t rdc, int64_t alc, int64_t rlc, int64_t arc);
int qop_jobs_add_if(struct ifinfo **rp, const char *ifname, 
    u_int bandwidth, int qlimit, int separate);
int qop_jobs_add_class(struct classinfo **rp, const char *class_name,
    struct ifinfo *ifinfo, int pri,
    int64_t adc, int64_t rdc, int64_t alc, int64_t rlc, int64_t arc,
    int flags);
int qop_jobs_modify_class(struct classinfo *clinfo, int pri,
    int64_t adc, int64_t rdc, int64_t alc, int64_t rlc, int64_t arc);
