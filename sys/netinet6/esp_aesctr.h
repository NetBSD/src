/*	$NetBSD: esp_aesctr.h,v 1.2.90.1 2009/05/13 17:22:28 jym Exp $	*/
/*	$KAME: esp_aesctr.h,v 1.2 2003/07/20 00:29:38 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, 1998 and 2003 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NETINET6_ESP_AESCTR_H_
#define _NETINET6_ESP_AESCTR_H_

extern int esp_aesctr_mature(struct secasvar *);
extern size_t esp_aesctr_schedlen(const struct esp_algorithm *);
extern int esp_aesctr_schedule(const struct esp_algorithm *,
	struct secasvar *);
extern int esp_aesctr_decrypt(struct mbuf *, size_t,
	struct secasvar *, const struct esp_algorithm *, int);
extern int esp_aesctr_encrypt(struct mbuf *, size_t, size_t,
	struct secasvar *, const struct esp_algorithm *, int);

#endif /* !_NETINET6_ESP_AESCTR_H_ */
