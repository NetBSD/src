/*	$NetBSD: yppush.h,v 1.2 2002/07/06 00:46:12 wiz Exp $	*/

/*
 * Copyright (c) 1996 Mats O Jansson <moj@stacken.kth.se>
 * All rights reserved.
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
 *	This product includes software developed by Mats O Jansson
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This file was originally generated with rpcgen, then cleaned up
 * by Jason R. Thorpe <thorpej@NetBSD.ORG>.
 */

#define YPPUSH_XFRRESPPROG ((unsigned long)0x40000000)
#define YPPUSH_XFRRESPVERS ((unsigned long)1)

#if 0	/* defined in <rpcsvc/yp_prot.h> */
#define YPPUSHPROC_NULL ((unsigned long)0)
#define YPPUSHPROC_XFRRESP ((unsigned long)1)
#endif

void	*yppushproc_null_1(void *, CLIENT *);
void	*yppushproc_null_1_svc(void *, struct svc_req *);
void	*yppushproc_xfrresp_1(struct yppushresp_xfr *, CLIENT *);
void	*yppushproc_xfrresp_1_svc(void *, struct svc_req *);

void	yppush_xfrrespprog_1(struct svc_req *, SVCXPRT *);

char	*yppush_err_string(int);
