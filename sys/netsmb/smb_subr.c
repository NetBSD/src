/*	$NetBSD: smb_subr.c,v 1.1 2000/12/07 03:48:10 deberg Exp $	*/

/*
 * Copyright (c) 2000, Boris Popov
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
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/signalvar.h>
#include <sys/mbuf.h>

#include <sys/tree.h>

#include "iconv.h"

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_rq.h>
#include <netsmb/smb_subr.h>
#include <netsmb/netbios.h>

#ifndef NetBSD
MALLOC_DEFINE(M_SMBDATA, "SMBDATA", "Misc netsmb data");
MALLOC_DEFINE(M_SMBSTR, "SMBSTR", "netsmb string data");
MALLOC_DEFINE(M_SMBTEMP, "SMBTEMP", "Temp netsmb data");
#endif

smb_unichar smb_unieol = 0;

void
smb_makescred(struct smb_cred *scred, struct proc *p, struct ucred *cred)
{
	if (p) {
		scred->scr_p = p;
		scred->scr_cred = cred ? cred : p->p_ucred;
	} else {
		scred->scr_p = NULL;
		scred->scr_cred = cred ? cred : NULL;
	}
}

int
smb_proc_intr(struct proc *p)
{
#ifndef NetBSD
#if __FreeBSD_version < 400009

	if (p && p->p_siglist &&
	    (((p->p_siglist & ~p->p_sigmask) & ~p->p_sigignore) & SMB_SIGMASK))
		return EINTR;
	return 0;
#else
	sigset_t tmpset;

	if (p == NULL)
		return 0;
	tmpset = p->p_siglist;
	SIGSETNAND(tmpset, p->p_sigmask);
	SIGSETNAND(tmpset, p->p_sigignore);
	if (SIGNOTEMPTY(p->p_siglist) && SMB_SIGMASK(tmpset))
                return EINTR;
	return 0;
#endif
#else
	sigset_t tmpset;

	if (p == NULL)
		return 0;
/*  	tmpsetp = p->p_siglist; */
	sigemptyset (&tmpset);
	sigplusset (&p->p_siglist, &tmpset);
	sigminusset(&p->p_sigmask, &tmpset);
	sigminusset(&p->p_sigignore, &tmpset);
	if (SMB_SIGMASK(&tmpset))
		return EINTR;
	return 0;
#endif
}

char *
smb_strdup(const char *s)
{
	char *p;
	int len;

	len = s ? strlen(s) + 1 : 1;
	MALLOC(p, char *, len, M_SMBSTR, M_WAITOK);
	if (p == NULL)
		return NULL;
	if (s)
		bcopy(s, p, len);
	else
		*p = 0;
	return p;
}

/*
 * duplicate string from a user space.
 */
char *
smb_strdupin(char *s, int maxlen)
{
	char *p, bt;
	int len = 0;

	for (p = s; ;p++) {
		if (copyin(p, &bt, 1))
			return NULL;
		len++;
		if (maxlen && len > maxlen)
			return NULL;
		if (bt == 0)
			break;
	}
	MALLOC(p, char *, len, M_SMBSTR, M_WAITOK);
	if (p == NULL)
		return NULL;
	copyin(s, p, len);
	return p;
}

/*
 * duplicate memory block from a user space.
 */
void *
smb_memdupin(void *umem, int len)
{
	char *p;

	if (len > 8 * 1024)
		return NULL;
	MALLOC(p, char *, len, M_SMBSTR, M_WAITOK);
	if (p == NULL)
		return NULL;
	if (copyin(umem, p, len) == 0)
		return p;
	free(p, M_SMBSTR);
	return NULL;
}

/*
 * duplicate memory block in the kernel space.
 */
void *
smb_memdup(const void *umem, int len)
{
	char *p;

	if (len > 8 * 1024)
		return NULL;
	MALLOC(p, char *, len, M_SMBSTR, M_WAITOK);
	if (p == NULL)
		return NULL;
	bcopy(umem, p, len);
	return p;
}

void
smb_strfree(char *s)
{
	free(s, M_SMBSTR);
}

void
smb_memfree(void *s)
{
	free(s, M_SMBSTR);
}

void
smb_strtouni(u_int16_t *dst, const char *src)
{
	while (*src) {
		*dst++ = htoles(*src++);
	}
	*dst = 0;
}

#ifdef SMB_SOCKETDATA_DEBUG
void
m_dumpm(struct mbuf *m) {
	char *p;
	int len;
	printf("d=");
	while(m) {
		p=mtod(m,char *);
		len=m->m_len;
		printf("(%d)",len);
		while(len--){
			printf("%02x ",((int)*(p++)) & 0xff);
		}
		m=m->m_next;
	};
	printf("\n");
}
#endif

int
smb_maperror(int eclass, int eno)
{
	if (eclass == 0 && eno == 0)
		return 0;
	switch (eclass) {
	    case ERRDOS:
		switch (eno) {
		    case ERRbadfunc:
		    case ERRbadmcb:
		    case ERRbadenv:
		    case ERRbadformat:
		    case ERRrmuns:
			return EINVAL;
		    case ERRbadfile:
		    case ERRbadpath:
		    case ERRremcd:
		    case 66:		/* nt returns it when share not available */
			return ENOENT;
		    case ERRnofids:
			return EMFILE;
		    case ERRnoaccess:
		    case ERRbadshare:
			return EACCES;
		    case ERRbadfid:
			return EBADF;
		    case ERRnomem:
			return ENOMEM;	/* actually remote no mem... */
		    case ERRbadmem:
			return EFAULT;
		    case ERRbadaccess:
			return EACCES;
		    case ERRbaddata:
			return E2BIG;
		    case ERRbaddrive:
			return ENXIO;
		    case ERRdiffdevice:
			return EXDEV;
		    case ERRnofiles:
			return 0;	/* eeof ? */
			return ETXTBSY;
		    case ERRlock:
			return EDEADLK;
		    case ERRfilexists:
			return EEXIST;
		    case 123:		/* dunno what is it, but samba maps as noent */
			return ENOENT;
		    case 145:		/* samba */
			return ENOTEMPTY;
		    case 183:
			return EEXIST;
		}
		break;
	    case ERRSRV:
		switch (eno) {
		    case ERRerror:
			return EINVAL;
		    case ERRbadpw:
			return EAUTH;
		    case ERRaccess:
			return EACCES;
		    case ERRinvnid:
			return ENETRESET;
		    case ERRinvnetname:
			SMBERROR("NetBIOS name is invalid\n");
			return EAUTH;
		    case 3:		/* reserved and returned */
			return EIO;
		    case 2239:		/* NT: account exists but disabled */
			return EPERM;
		}
		break;
	    case ERRHRD:
		switch (eno) {
		    case ERRnowrite:
			return EROFS;
		    case ERRbadunit:
			return ENODEV;
		    case ERRnotready:
		    case ERRbadcmd:
		    case ERRdata:
			return EIO;
		    case ERRbadreq:
			return EBADRPC;
		    case ERRbadshare:
			return ETXTBSY;
		    case ERRlock:
			return EDEADLK;
		}
		break;
	}
	SMBERROR("Unmapped error %d:%d\n", eclass, eno);
	return EBADRPC;
}

int
smb_put_dmem(struct mbdata *mbp, struct smb_conn *scp, const char *src,
	int size, int caseopt)
{
	struct iconv_drv *dp = scp->sc_toserver;
	struct mbuf *m;
	char *dst;
	int error = 0, s, cplen, outlen;

	if (size == 0)
		return 0;
	if (dp == NULL) {
		return mb_put_mem(mbp, src, size, MB_MSYSTEM);
	}
	m = mbp->mb_cur;
	if (m_getm(m, size) == NULL)
		return ENOBUFS;
	while (size > 0) {
		cplen = M_TRAILINGSPACE(m);
		if (cplen == 0) {
			m = m->m_next;
			continue;
		}
		if (cplen > size)
			cplen = size;
		s = cplen;
		dst = mtod(m, caddr_t) + m->m_len;
		error = iconv_conv(dp, &src, &cplen, &dst, &outlen);
		if (error)
			return error;
		size -= s;
		m->m_len += s;
		mbp->mb_count += s;
	}
	mbp->mb_pos = mtod(m, caddr_t) + m->m_len;
	mbp->mb_cur = m;
	return 0;
}

int
smb_put_dstring(struct mbdata *mbp, struct smb_conn *scp, const char *src,
	int caseopt)
{
	int error;

	error = smb_put_dmem(mbp, scp, src, strlen(src), caseopt);
	if (error)
		return error;
	return mb_put_byte(mbp, 0);
}

int
smb_put_asunistring(struct smb_rq *rqp, const char *src)
{
	struct mbdata *mbp = &rqp->sr_rq;
	struct iconv_drv *dp = rqp->sr_conn->sc_toserver;
	u_char c;
	int error;

	while (*src) {
		iconv_convmem(dp, &c, src++, 1);
		error = mb_put_wordle(mbp, c);
		if (error)
			return error;
	}
	return mb_put_wordle(mbp, 0);
}

#define M_NBDATA	M_PCB
struct sockaddr *
dup_sockaddr(struct sockaddr *sa, int i)
{
	struct sockaddr *p;
	register int len = sizeof(struct sockaddr_nb);

	MALLOC(p, struct sockaddr *, len, M_NBDATA, M_WAITOK);
	if (p == NULL)
		return NULL;
	bcopy(sa, p, len);
	return p;
}
