/*	$NetBSD: namespace.h,v 1.4 1999/11/20 19:47:16 veego Exp $

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthias Scheler.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _NAMESPACE_H_
#define _NAMESPACE_H_

#define dn_expand		bind_dn_expand
#define inet_addr		bind_inet_addr
#define inet_aton		bind_inet_aton
#define inet_pton		bind_inet_pton
#define fp_nquery		bind_fp_nquery
#define fp_resstat		bind_fp_resstat
#define hostalias		bind_hostalias
#define p_class			bind_p_class
#define p_type			bind_p_type
#define putlong			bind_putlong
#define _getlong		bind_getlong
#define putshort		bind_putshort
#define res_dnok		bind_res_dnok
#define _getshort		bind__getshort
#define res_hnok		bind_res_hnok	
#define res_send_setqhook	bind_res_send_setqhook
#define res_send_setrhook	bind_res_send_setrhook
#define __p_type_syms		bind_p_type_syms
#define __p_class_syms		bind_p_class_syms
#define __putshort		bind_putshort
#define __putlong		bind_putlong
#define res_querydomain		bind_res_querydomain
#define res_query		bind_res_query
#define res_search		bind_res_search
#define res_state		bind_res_state
#define _res			bind__res
#define _res_opcodes		bind__res_opcodes

#endif /* _NAMESPACE_H_ */
