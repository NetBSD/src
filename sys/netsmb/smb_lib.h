/*	$NetBSD: smb_lib.h,v 1.1 2000/12/07 03:48:10 deberg Exp $	*/

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

#ifndef _NETSMB_SMB_LIB_H_
#define _NETSMB_SMB_LIB_H_

#include <netsmb/smb.h>
#include <netsmb/smb_dev.h>

#define	SMB_CFG_FILE	"/usr/local/etc/nsmb.conf"

#define	STDPARAM_ARGS	'A':case 'B':case 'C':case 'E':case 'I': \
		   case 'L':case 'M': \
		   case 'N':case 'U':case 'R':case 'S':case 'T': \
		   case 'W':case 'O':case 'P'

#define STDPARAM_OPT	"A:BCE:I:L:M:NO:P:U:R:S:T:W:"

/*
 * bits to indicate the source of error
 */
#define SMB_RAP_ERROR	0x10000

#ifndef min
#define	min(a,b)	(((a)<(b)) ? (a) : (b))
#endif

/*
 * SMB work context. Used to store all values which is necessary
 * to establish connection to an SMB server.
 */
struct smb_ctx {
	int		ct_flags;	/* SMBCF_ */
	int		ct_fd;		/* handle of connection */
	int		ct_parsedlevel;
	int		ct_minlevel;
	int		ct_maxlevel;
	char *		ct_srvaddr;	/* hostname or IP address of server */
	char *		ct_scope;	/* NetBIOS scope */
	char		ct_locname[SMB_MAXUSERNAMELEN + 1];
	const char *	ct_uncnext;
	struct smbioc_oconn	ct_conn;
	struct smbioc_ossn	ct_ssn;
	struct smbioc_oshare	ct_sh;
};

#define	SMBCF_NOPWD		0x0001	/* don't ask for a password */
#define	SMBCF_SRIGHTS		0x0002	/* share access rights was supplied */
#define	SMBCF_LOCALE		0x0004	/* use current locale */
#define	SMBCF_RESOLVED		0x8000	/* structure has been verified */

/*
 * request handling structures
 */
struct mbuf {
	int		m_len;
	int		m_maxlen;
	char *		m_data;
	struct mbuf *	m_next;
};

struct mbdata {
	struct mbuf *	mb_top;
	struct mbuf * 	mb_cur;
	char *		mb_pos;
	int		mb_count;
};

#define	M_ALIGNFACTOR	(sizeof(long))
#define M_ALIGN(len)	(((len) + M_ALIGNFACTOR - 1) & ~(M_ALIGNFACTOR - 1))
#define	M_BASESIZE	(sizeof(struct mbuf))
#define	M_MINSIZE	(256 - M_BASESIZE)
#define M_TOP(m)	((char*)(m) + M_BASESIZE)
#define mtod(m,t)	((t)(m)->m_data)
#define M_TRAILINGSPACE(m) ((m)->m_maxlen - (m)->m_len)

struct smb_rq {
	u_char		rq_cmd;
	struct mbdata	rq_rq;
	struct mbdata	rq_rp;
	struct smb_ctx *rq_ctx;
	int		rq_wcount;
	int		rq_bcount;
};

struct smb_bitname {
	u_int	bn_bit;
	char	*bn_name;
};

extern struct rcfile *smb_rc;

__BEGIN_DECLS

struct sockaddr;

int  smb_lib_init(void);
int  smb_open_rcfile(void);
void smb_error(const char *, int,...);
char *smb_printb(char *, int, const struct smb_bitname *);
void *smb_dumptree(void);

/*
 * Context management
 */
int  smb_ctx_init(struct smb_ctx *, int, char *[], int, int, int);
void smb_ctx_done(struct smb_ctx *);
int  smb_ctx_parseunc(struct smb_ctx *, const char *, int, const char **);
int  smb_ctx_setcharset(struct smb_ctx *, const char *);
int  smb_ctx_setserver(struct smb_ctx *, const char *);
int  smb_ctx_setuser(struct smb_ctx *, const char *);
int  smb_ctx_setshare(struct smb_ctx *, const char *, int);
int  smb_ctx_setscope(struct smb_ctx *, const char *);
int  smb_ctx_setworkgroup(struct smb_ctx *, const char *);
int  smb_ctx_setpassword(struct smb_ctx *, const char *);
int  smb_ctx_setsrvaddr(struct smb_ctx *, const char *);
int  smb_ctx_opt(struct smb_ctx *, int, const char *);
int  smb_ctx_lookup(struct smb_ctx *, int, int);
int  smb_ctx_login(struct smb_ctx *);
int  smb_ctx_readrc(struct smb_ctx *);
int  smb_ctx_resolve(struct smb_ctx *);
int  smb_ctx_setflags(struct smb_ctx *, int, int, int);

int  smb_smb_open_print_file(struct smb_ctx *, int, int, const char *, smbfh*);
int  smb_smb_close_print_file(struct smb_ctx *, smbfh);

int  smb_read(struct smb_ctx *, smbfh, off_t, size_t, char *);
int  smb_write(struct smb_ctx *, smbfh, off_t, size_t, const char *);

#define smb_rq_getrequest(rqp)	(&(rqp)->rq_rq)
#define smb_rq_getreply(rqp)	(&(rqp)->rq_rp)

int  smb_rq_init(struct smb_ctx *, u_char, size_t, struct smb_rq **);
void smb_rq_done(struct smb_rq *);
void smb_rq_wend(struct smb_rq *);
int  smb_rq_simple(struct smb_rq *);
int  smb_rq_dmem(struct mbdata *, const char *, size_t);
int  smb_rq_dstring(struct mbdata *, const char *);

int  smb_t2_request(struct smb_ctx *, int, int, const char *,
	int, void *, int, void *, int *, void *, int *, void *);

void smb_simplecrypt(char *dst, const char *src);
int  smb_simpledecrypt(char *dst, const char *src);

int  m_getm(struct mbuf *, size_t, struct mbuf **);
int  m_lineup(struct mbuf *, struct mbuf **);
int  mb_init(struct mbdata *, size_t);
int  mb_initm(struct mbdata *, struct mbuf *);
int  mb_done(struct mbdata *);
int  mb_put_byte(struct mbdata *, u_int8_t);
int  mb_put_wordbe(struct mbdata *, u_int16_t);
int  mb_put_wordle(struct mbdata *, u_int16_t);
int  mb_put_dwordbe(struct mbdata *, u_int32_t);
int  mb_put_dwordle(struct mbdata *, u_int32_t);
int  mb_put_qwordbe(struct mbdata *, int64_t);
int  mb_put_qwordle(struct mbdata *, int64_t);
int  mb_put_mem(struct mbdata *, const char *, size_t);
int  mb_put_pstring(struct mbdata *mbp, const char *s);
int  mb_put_mbuf(struct mbdata *, struct mbuf *);

int  mb_get_byte(struct mbdata *, u_int8_t *);
int  mb_get_word(struct mbdata *, u_int16_t *);
int  mb_get_wordle(struct mbdata *, u_int16_t *);
int  mb_get_wordbe(struct mbdata *, u_int16_t *);
int  mb_get_dword(struct mbdata *, u_int32_t *);
int  mb_get_dwordbe(struct mbdata *, u_int32_t *);
int  mb_get_dwordle(struct mbdata *, u_int32_t *);
int  mb_get_qword(struct mbdata *, int64_t *);
int  mb_get_qwordbe(struct mbdata *, int64_t *);
int  mb_get_qwordle(struct mbdata *, int64_t *);
int  mb_get_mem(struct mbdata *, char *, size_t);

__END_DECLS

#endif /* _NETSMB_SMB_LIB_H_ */
