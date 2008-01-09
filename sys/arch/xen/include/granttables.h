/* $NetBSD: granttables.h,v 1.1.52.2 2008/01/09 01:50:05 matt Exp $ */
/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *      This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Interface to the Xen Grant tables */
#include <xen/xen3-public/grant_table.h>

void xengnt_init(void);

/*
 * grant access to a remote domain. Returns a handle on the allocated grant
 * entry in table in grant_ref_t *.
 */
int xengnt_grant_access(domid_t, paddr_t, int, grant_ref_t *);

/*
 * Revoke access. Caller is responsible to ensure that the grant entry is
 * not referenced any more.
 */
void xengnt_revoke_access(grant_ref_t);

/* allow a page transfer from a remote domain */
int xengnt_grant_transfer(domid_t, grant_ref_t *);

/* end transfer, return the new page address or 0 */
paddr_t xengnt_revoke_transfer(grant_ref_t);

/*
 * Query grant status (i.e. if remote has a valid mapping to this grant).
 * Returns GTF_reading | GTF_writing.
 */
int xengnt_status(grant_ref_t);
