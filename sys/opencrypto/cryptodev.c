/*	$NetBSD: cryptodev.c,v 1.12.12.5 2008/02/04 09:24:47 yamt Exp $ */
/*	$FreeBSD: src/sys/opencrypto/cryptodev.c,v 1.4.2.4 2003/06/03 00:09:02 sam Exp $	*/
/*	$OpenBSD: cryptodev.c,v 1.53 2002/07/10 22:21:30 mickey Exp $	*/

/*
 * Copyright (c) 2001 Theo de Raadt
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
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
 * Effort sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and Air Force Research Laboratory, Air Force
 * Materiel Command, USAF, under agreement number F30602-01-2-0537.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cryptodev.c,v 1.12.12.5 2008/02/04 09:24:47 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/pool.h>
#include <sys/sysctl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/errno.h>
#include <sys/md5.h>
#include <sys/sha1.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kauth.h>

#include "opt_ocf.h"
#include <opencrypto/cryptodev.h>
#include <opencrypto/xform.h>

struct csession {
	TAILQ_ENTRY(csession) next;
	u_int64_t	sid;
	u_int32_t	ses;

	u_int32_t	cipher;
	struct enc_xform *txform;
	u_int32_t	mac;
	struct auth_hash *thash;

	void *		key;
	int		keylen;
	u_char		tmp_iv[EALG_MAX_BLOCK_LEN];

	void *		mackey;
	int		mackeylen;
	u_char		tmp_mac[CRYPTO_MAX_MAC_LEN];

	struct iovec	iovec[1];	/* user requests never have more */
	struct uio	uio;
	int		error;
};

struct fcrypt {
	TAILQ_HEAD(csessionlist, csession) csessions;
	int		sesn;
};

/* For our fixed-size allocations */
struct pool fcrpl;
struct pool csepl;

/* Declaration of master device (fd-cloning/ctxt-allocating) entrypoints */
static int	cryptoopen(dev_t dev, int flag, int mode, struct lwp *l);
static int	cryptoread(dev_t dev, struct uio *uio, int ioflag);
static int	cryptowrite(dev_t dev, struct uio *uio, int ioflag);
static int	cryptoselect(dev_t dev, int rw, struct lwp *l);

/* Declaration of cloned-device (per-ctxt) entrypoints */
static int	cryptof_read(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int	cryptof_write(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int	cryptof_ioctl(struct file *, u_long, void*, struct lwp *l);
static int	cryptof_close(struct file *, struct lwp *);

static const struct fileops cryptofops = {
    cryptof_read,
    cryptof_write,
    cryptof_ioctl,
    fnullop_fcntl,
    fnullop_poll,
    fbadop_stat,
    cryptof_close,
    fnullop_kqfilter
};

static struct	csession *csefind(struct fcrypt *, u_int);
static int	csedelete(struct fcrypt *, struct csession *);
static struct	csession *cseadd(struct fcrypt *, struct csession *);
static struct	csession *csecreate(struct fcrypt *, u_int64_t, void *, u_int64_t,
    void *, u_int64_t, u_int32_t, u_int32_t, struct enc_xform *,
    struct auth_hash *);
static int	csefree(struct csession *);

static int	cryptodev_op(struct csession *, struct crypt_op *, struct lwp *);
static int	cryptodev_key(struct crypt_kop *);
int	cryptodev_dokey(struct crypt_kop *kop, struct crparam kvp[]);

static int	cryptodev_cb(void *);
static int	cryptodevkey_cb(void *);

/*
 * sysctl-able control variables for /dev/crypto now defined in crypto.c:
 * crypto_usercrypto, crypto_userasmcrypto, crypto_devallowsoft.
 */

/* ARGSUSED */
int
cryptof_read(struct file *fp, off_t *poff,
    struct uio *uio, kauth_cred_t cred, int flags)
{
	return (EIO);
}

/* ARGSUSED */
int
cryptof_write(struct file *fp, off_t *poff,
    struct uio *uio, kauth_cred_t cred, int flags)
{
	return (EIO);
}

/* ARGSUSED */
int
cryptof_ioctl(struct file *fp, u_long cmd, void* data, struct lwp *l)
{
	struct cryptoini cria, crie;
	struct fcrypt *fcr = (struct fcrypt *)fp->f_data;
	struct csession *cse;
	struct session_op *sop;
	struct crypt_op *cop;
	struct enc_xform *txform = NULL;
	struct auth_hash *thash = NULL;
	u_int64_t sid;
	u_int32_t ses;
	int error = 0;

	/* backwards compatibility */
        struct file *criofp;
	struct fcrypt *criofcr;
	int criofd;

        switch (cmd) {
        case CRIOGET:   /* XXX deprecated, remove after 5.0 */
                if ((error = falloc(l, &criofp, &criofd)) != 0)
                        return error;
                criofcr = pool_get(&fcrpl, PR_WAITOK);
		mutex_spin_enter(&crypto_mtx);
                TAILQ_INIT(&criofcr->csessions);
                /*
                 * Don't ever return session 0, to allow detection of
                 * failed creation attempts with multi-create ioctl.
                 */
                criofcr->sesn = 1;
		mutex_spin_exit(&crypto_mtx);
                (void)fdclone(l, criofp, criofd, (FREAD|FWRITE),
			      &cryptofops, criofcr);
                *(u_int32_t *)data = criofd;
		return error;
                break;
	case CIOCGSESSION:
		sop = (struct session_op *)data;
		switch (sop->cipher) {
		case 0:
			break;
		case CRYPTO_DES_CBC:
			txform = &enc_xform_des;
			break;
		case CRYPTO_3DES_CBC:
			txform = &enc_xform_3des;
			break;
		case CRYPTO_BLF_CBC:
			txform = &enc_xform_blf;
			break;
		case CRYPTO_CAST_CBC:
			txform = &enc_xform_cast5;
			break;
		case CRYPTO_SKIPJACK_CBC:
			txform = &enc_xform_skipjack;
			break;
		case CRYPTO_AES_CBC:
			txform = &enc_xform_rijndael128;
			break;
		case CRYPTO_NULL_CBC:
			txform = &enc_xform_null;
			break;
		case CRYPTO_ARC4:
			txform = &enc_xform_arc4;
			break;
		default:
			DPRINTF(("Invalid cipher %d\n", sop->cipher));
			return (EINVAL);
		}

		switch (sop->mac) {
		case 0:
			break;
		case CRYPTO_MD5_HMAC:
			thash = &auth_hash_hmac_md5;
			break;
		case CRYPTO_SHA1_HMAC:
			thash = &auth_hash_hmac_sha1;
			break;
		case CRYPTO_MD5_HMAC_96:
			thash = &auth_hash_hmac_md5_96;
			break;
		case CRYPTO_SHA1_HMAC_96:
			thash = &auth_hash_hmac_sha1_96;
			break;
		case CRYPTO_SHA2_HMAC:
			if (sop->mackeylen == auth_hash_hmac_sha2_256.keysize)
				thash = &auth_hash_hmac_sha2_256;
			else if (sop->mackeylen == auth_hash_hmac_sha2_384.keysize)
				thash = &auth_hash_hmac_sha2_384;
			else if (sop->mackeylen == auth_hash_hmac_sha2_512.keysize)
				thash = &auth_hash_hmac_sha2_512;
			else {
				DPRINTF(("Invalid mackeylen %d\n",
				    sop->mackeylen));
				return (EINVAL);
			}
			break;
		case CRYPTO_RIPEMD160_HMAC:
			thash = &auth_hash_hmac_ripemd_160_96;
			break;
		case CRYPTO_MD5:
			thash = &auth_hash_md5;
			break;
		case CRYPTO_SHA1:
			thash = &auth_hash_sha1;
			break;
		case CRYPTO_NULL_HMAC:
			thash = &auth_hash_null;
			break;
		default:
			DPRINTF(("Invalid mac %d\n", sop->mac));
			return (EINVAL);
		}

		bzero(&crie, sizeof(crie));
		bzero(&cria, sizeof(cria));

		if (txform) {
			crie.cri_alg = txform->type;
			crie.cri_klen = sop->keylen * 8;
			if (sop->keylen > txform->maxkey ||
			    sop->keylen < txform->minkey) {
				DPRINTF(("keylen %d not in [%d,%d]\n",
				    sop->keylen, txform->minkey,
				    txform->maxkey));
				error = EINVAL;
				goto bail;
			}

			crie.cri_key = malloc(crie.cri_klen / 8, M_XDATA,
			    M_WAITOK);
			if ((error = copyin(sop->key, crie.cri_key,
			    crie.cri_klen / 8)))
				goto bail;
			if (thash)
				crie.cri_next = &cria;
		}

		if (thash) {
			cria.cri_alg = thash->type;
			cria.cri_klen = sop->mackeylen * 8;
			if (sop->mackeylen != thash->keysize) {
				DPRINTF(("mackeylen %d != keysize %d\n",
				    sop->mackeylen, thash->keysize));
				error = EINVAL;
				goto bail;
			}

			if (cria.cri_klen) {
				cria.cri_key = malloc(cria.cri_klen / 8,
				    M_XDATA, M_WAITOK);
				if ((error = copyin(sop->mackey, cria.cri_key,
				    cria.cri_klen / 8)))
					goto bail;
			}
		}
		/* crypto_newsession requires that we hold the mutex. */
		mutex_spin_enter(&crypto_mtx);
		error = crypto_newsession(&sid, (txform ? &crie : &cria),
			    crypto_devallowsoft);
		if (error) {
		  	DPRINTF(("SIOCSESSION violates kernel parameters %d\n",
			    error));
			goto bail;
		}

		cse = csecreate(fcr, sid, crie.cri_key, crie.cri_klen,
		    cria.cri_key, cria.cri_klen, sop->cipher, sop->mac, txform,
		    thash);

		if (cse == NULL) {
			DPRINTF(("csecreate failed\n"));
			crypto_freesession(sid);
			error = EINVAL;
			goto bail;
		}
		sop->ses = cse->ses;

bail:
		mutex_spin_exit(&crypto_mtx);
		if (error) {
			if (crie.cri_key)
				FREE(crie.cri_key, M_XDATA);
			if (cria.cri_key)
				FREE(cria.cri_key, M_XDATA);
		}
		break;
	case CIOCFSESSION:
		mutex_spin_enter(&crypto_mtx);
		ses = *(u_int32_t *)data;
		cse = csefind(fcr, ses);
		if (cse == NULL)
			return (EINVAL);
		csedelete(fcr, cse);
		error = csefree(cse);
		mutex_spin_exit(&crypto_mtx);
		break;
	case CIOCCRYPT:
		mutex_spin_enter(&crypto_mtx);
		cop = (struct crypt_op *)data;
		cse = csefind(fcr, cop->ses);
		mutex_spin_exit(&crypto_mtx);
		if (cse == NULL) {
			DPRINTF(("csefind failed\n"));
			return (EINVAL);
		}
		error = cryptodev_op(cse, cop, l);
		break;
	case CIOCKEY:
		error = cryptodev_key((struct crypt_kop *)data);
		break;
	case CIOCASYMFEAT:
		error = crypto_getfeat((int *)data);
		break;
	default:
		DPRINTF(("invalid ioctl cmd %ld\n", cmd));
		error = EINVAL;
	}
	return (error);
}

static int
cryptodev_op(struct csession *cse, struct crypt_op *cop, struct lwp *l)
{
	struct cryptop *crp = NULL;
	struct cryptodesc *crde = NULL, *crda = NULL;
	int error;

	if (cop->len > 256*1024-4)
		return (E2BIG);

	if (cse->txform) {
		if (cop->len == 0 || (cop->len % cse->txform->blocksize) != 0)
			return (EINVAL);
	}

	bzero(&cse->uio, sizeof(cse->uio));
	cse->uio.uio_iovcnt = 1;
	cse->uio.uio_resid = 0;
	cse->uio.uio_rw = UIO_WRITE;
	cse->uio.uio_iov = cse->iovec;
	UIO_SETUP_SYSSPACE(&cse->uio);
	memset(&cse->iovec, 0, sizeof(cse->iovec));
	cse->uio.uio_iov[0].iov_len = cop->len;
	cse->uio.uio_iov[0].iov_base = malloc(cop->len, M_XDATA, M_WAITOK);
	cse->uio.uio_resid = cse->uio.uio_iov[0].iov_len;

	crp = crypto_getreq((cse->txform != NULL) + (cse->thash != NULL));
	if (crp == NULL) {
		error = ENOMEM;
		goto bail;
	}

	if (cse->thash) {
		crda = crp->crp_desc;
		if (cse->txform)
			crde = crda->crd_next;
	} else {
		if (cse->txform)
			crde = crp->crp_desc;
		else {
			error = EINVAL;
			goto bail;
		}
	}

	if ((error = copyin(cop->src, cse->uio.uio_iov[0].iov_base, cop->len)))
		goto bail;

	if (crda) {
		crda->crd_skip = 0;
		crda->crd_len = cop->len;
		crda->crd_inject = 0;	/* ??? */

		crda->crd_alg = cse->mac;
		crda->crd_key = cse->mackey;
		crda->crd_klen = cse->mackeylen * 8;
	}

	if (crde) {
		if (cop->op == COP_ENCRYPT)
			crde->crd_flags |= CRD_F_ENCRYPT;
		else
			crde->crd_flags &= ~CRD_F_ENCRYPT;
		crde->crd_len = cop->len;
		crde->crd_inject = 0;

		crde->crd_alg = cse->cipher;
		crde->crd_key = cse->key;
		crde->crd_klen = cse->keylen * 8;
	}

	crp->crp_ilen = cop->len;
	crp->crp_flags = CRYPTO_F_IOV | CRYPTO_F_CBIMM
		       | (cop->flags & COP_F_BATCH);
	crp->crp_buf = (void *)&cse->uio;
	crp->crp_callback = (int (*) (struct cryptop *)) cryptodev_cb;
	crp->crp_sid = cse->sid;
	crp->crp_opaque = (void *)cse;

	if (cop->iv) {
		if (crde == NULL) {
			error = EINVAL;
			goto bail;
		}
		if (cse->cipher == CRYPTO_ARC4) { /* XXX use flag? */
			error = EINVAL;
			goto bail;
		}
		if ((error = copyin(cop->iv, cse->tmp_iv, cse->txform->blocksize)))
			goto bail;
		bcopy(cse->tmp_iv, crde->crd_iv, cse->txform->blocksize);
		crde->crd_flags |= CRD_F_IV_EXPLICIT | CRD_F_IV_PRESENT;
		crde->crd_skip = 0;
	} else if (crde) {
		if (cse->cipher == CRYPTO_ARC4) { /* XXX use flag? */
			crde->crd_skip = 0;
		} else {
			crde->crd_flags |= CRD_F_IV_PRESENT;
			crde->crd_skip = cse->txform->blocksize;
			crde->crd_len -= cse->txform->blocksize;
		}
	}

	if (cop->mac) {
		if (crda == NULL) {
			error = EINVAL;
			goto bail;
		}
		crp->crp_mac=cse->tmp_mac;
	}

	/*
	 * XXX there was a comment here which said that we went to
	 * XXX splcrypto() but needed to only if CRYPTO_F_CBIMM,
	 * XXX disabled on NetBSD since 1.6O due to a race condition.
	 * XXX But crypto_dispatch went to splcrypto() itself!  (And
	 * XXX now takes the crypto_mtx mutex itself).  We do, however,
	 * XXX need to hold the mutex across the call to cv_wait().
	 * XXX     (should we arrange for crypto_dispatch to return to
	 * XXX      us with it held?  it seems quite ugly to do so.)
	 */
	error = crypto_dispatch(crp);
	mutex_spin_enter(&crypto_mtx);
	if (error != 0) {
		DPRINTF(("cryptodev_op: not waiting, error.\n"));
		mutex_spin_exit(&crypto_mtx);
		goto bail;
	}
	while (!(crp->crp_flags & CRYPTO_F_DONE)) {
		DPRINTF(("cryptodev_op: sleeping on cv %08x for crp %08x\n", \
			(uint32_t)&crp->crp_cv, (uint32_t)crp));
		cv_wait(&crp->crp_cv, &crypto_mtx);	/* XXX cv_wait_sig? */
	}
	if (crp->crp_flags & CRYPTO_F_ONRETQ) {
		DPRINTF(("cryptodev_op: DONE, not woken by cryptoret.\n"));
		(void)crypto_ret_q_remove(crp);
	}
	mutex_spin_exit(&crypto_mtx);

	if (crp->crp_etype != 0) {
		error = crp->crp_etype;
		goto bail;
	}

	if (cse->error) {
		error = cse->error;
		goto bail;
	}

	if (cop->dst &&
	    (error = copyout(cse->uio.uio_iov[0].iov_base, cop->dst, cop->len)))
		goto bail;

	if (cop->mac &&
	    (error = copyout(crp->crp_mac, cop->mac, cse->thash->authsize)))
		goto bail;

bail:
	if (crp)
		crypto_freereq(crp);
	if (cse->uio.uio_iov[0].iov_base)
		free(cse->uio.uio_iov[0].iov_base, M_XDATA);

	return (error);
}

static int
cryptodev_cb(void *op)
{
	struct cryptop *crp = (struct cryptop *) op;
	struct csession *cse = (struct csession *)crp->crp_opaque;
	int error = 0;

	mutex_spin_enter(&crypto_mtx);
	cse->error = crp->crp_etype;
	if (crp->crp_etype == EAGAIN) {
		/* always drop mutex to call dispatch routine */
		mutex_spin_exit(&crypto_mtx);
		error = crypto_dispatch(crp);
		mutex_spin_enter(&crypto_mtx);
	}
	if (error != 0 || (crp->crp_flags & CRYPTO_F_DONE)) {
		cv_signal(&crp->crp_cv);
	}
	mutex_spin_exit(&crypto_mtx);
	return (0);
}

static int
cryptodevkey_cb(void *op)
{
	struct cryptkop *krp = (struct cryptkop *) op;

	mutex_spin_enter(&crypto_mtx);
	cv_signal(&krp->krp_cv);
	mutex_spin_exit(&crypto_mtx);
	return (0);
}

static int
cryptodev_key(struct crypt_kop *kop)
{
	struct cryptkop *krp = NULL;
	int error = EINVAL;
	int in, out, size, i;

	if (kop->crk_iparams + kop->crk_oparams > CRK_MAXPARAM) {
		return (EFBIG);
	}

	in = kop->crk_iparams;
	out = kop->crk_oparams;
	switch (kop->crk_op) {
	case CRK_MOD_EXP:
		if (in == 3 && out == 1)
			break;
		return (EINVAL);
	case CRK_MOD_EXP_CRT:
		if (in == 6 && out == 1)
			break;
		return (EINVAL);
	case CRK_DSA_SIGN:
		if (in == 5 && out == 2)
			break;
		return (EINVAL);
	case CRK_DSA_VERIFY:
		if (in == 7 && out == 0)
			break;
		return (EINVAL);
	case CRK_DH_COMPUTE_KEY:
		if (in == 3 && out == 1)
			break;
		return (EINVAL);
	case CRK_MOD_ADD:
		if (in == 3 && out == 1)
			break;
		return (EINVAL);
	case CRK_MOD_ADDINV:
		if (in == 2 && out == 1)
			break;
		return (EINVAL);
	case CRK_MOD_SUB:
		if (in == 3 && out == 1)
			break;
		return (EINVAL);
	case CRK_MOD_MULT:
		if (in == 3 && out == 1)
			break;
		return (EINVAL);
	case CRK_MOD_MULTINV:
		if (in == 2 && out == 1)
			break;
		return (EINVAL);
	case CRK_MOD:
		if (in == 2 && out == 1)
			break;
		return (EINVAL);
	default:
		return (EINVAL);
	}

	krp = pool_get(&cryptkop_pool, PR_WAITOK);
	if (!krp)
		return (ENOMEM);
	bzero(krp, sizeof *krp);
	cv_init(&krp->krp_cv, "crykdev");
	krp->krp_op = kop->crk_op;
	krp->krp_status = kop->crk_status;
	krp->krp_iparams = kop->crk_iparams;
	krp->krp_oparams = kop->crk_oparams;
	krp->krp_status = 0;
	krp->krp_callback = (int (*) (struct cryptkop *)) cryptodevkey_cb;

	for (i = 0; i < CRK_MAXPARAM; i++)
		krp->krp_param[i].crp_nbits = kop->crk_param[i].crp_nbits;
	for (i = 0; i < krp->krp_iparams + krp->krp_oparams; i++) {
		size = (krp->krp_param[i].crp_nbits + 7) / 8;
		if (size == 0)
			continue;
		krp->krp_param[i].crp_p = malloc(size, M_XDATA, M_WAITOK);
		if (i >= krp->krp_iparams)
			continue;
		error = copyin(kop->crk_param[i].crp_p, krp->krp_param[i].crp_p, size);
		if (error)
			goto fail;
	}

	error = crypto_kdispatch(krp);
	if (error != 0) {
		goto fail;
	}

	mutex_spin_enter(&crypto_mtx);
	while (!(krp->krp_flags & CRYPTO_F_DONE)) {
		cv_wait(&krp->krp_cv, &crypto_mtx);	/* XXX cv_wait_sig? */
	}
	if (krp->krp_flags & CRYPTO_F_ONRETQ) {
		DPRINTF(("cryptodev_key: DONE early, not via cryptoret.\n"));
		(void)crypto_ret_kq_remove(krp);
	}
	mutex_spin_exit(&crypto_mtx);

	if (krp->krp_status != 0) {
		error = krp->krp_status;
		goto fail;
	}

	for (i = krp->krp_iparams; i < krp->krp_iparams + krp->krp_oparams; i++) {
		size = (krp->krp_param[i].crp_nbits + 7) / 8;
		if (size == 0)
			continue;
		error = copyout(krp->krp_param[i].crp_p, kop->crk_param[i].crp_p, size);
		if (error)
			goto fail;
	}

fail:
	if (krp) {
		kop->crk_status = krp->krp_status;
		for (i = 0; i < CRK_MAXPARAM; i++) {
			if (krp->krp_param[i].crp_p)
				FREE(krp->krp_param[i].crp_p, M_XDATA);
		}
		pool_put(&cryptkop_pool, krp);
	}
	return (error);
}

/* ARGSUSED */
static int
cryptof_close(struct file *fp, struct lwp *l)
{
	struct fcrypt *fcr = (struct fcrypt *)fp->f_data;
	struct csession *cse;

	mutex_spin_enter(&crypto_mtx);
	while ((cse = TAILQ_FIRST(&fcr->csessions))) {
		TAILQ_REMOVE(&fcr->csessions, cse, next);
		(void)csefree(cse);
	}
	pool_put(&fcrpl, fcr);

	fp->f_data = NULL;
	mutex_spin_exit(&crypto_mtx);

	return 0;
}

/* csefind: call with crypto_mtx held. */
static struct csession *
csefind(struct fcrypt *fcr, u_int ses)
{
	struct csession *cse, *ret = NULL;

	KASSERT(mutex_owned(&crypto_mtx));
	TAILQ_FOREACH(cse, &fcr->csessions, next)
		if (cse->ses == ses)
			ret = cse;
	return (ret);
}

/* csedelete: call with crypto_mtx held. */
static int
csedelete(struct fcrypt *fcr, struct csession *cse_del)
{
	struct csession *cse;
	int ret = 0;

	KASSERT(mutex_owned(&crypto_mtx));
	TAILQ_FOREACH(cse, &fcr->csessions, next) {
		if (cse == cse_del) {
			TAILQ_REMOVE(&fcr->csessions, cse, next);
			ret = 1;
		}
	}
	return (ret);
}

/* cseadd: call with crypto_mtx held. */
static struct csession *
cseadd(struct fcrypt *fcr, struct csession *cse)
{
	KASSERT(mutex_owned(&crypto_mtx));
	/* don't let session ID wrap! */
	if (fcr->sesn + 1 == 0) return NULL;
	TAILQ_INSERT_TAIL(&fcr->csessions, cse, next);
	cse->ses = fcr->sesn++;
	return (cse);
}

/* csecreate: call with crypto_mtx held. */
static struct csession *
csecreate(struct fcrypt *fcr, u_int64_t sid, void *key, u_int64_t keylen,
    void *mackey, u_int64_t mackeylen, u_int32_t cipher, u_int32_t mac,
    struct enc_xform *txform, struct auth_hash *thash)
{
	struct csession *cse;

	KASSERT(mutex_owned(&crypto_mtx));
	cse = pool_get(&csepl, PR_NOWAIT);
	if (cse == NULL)
		return NULL;
	cse->key = key;
	cse->keylen = keylen/8;
	cse->mackey = mackey;
	cse->mackeylen = mackeylen/8;
	cse->sid = sid;
	cse->cipher = cipher;
	cse->mac = mac;
	cse->txform = txform;
	cse->thash = thash;
	if (cseadd(fcr, cse))
		return (cse);
	else {
		pool_put(&csepl, cse);
		return NULL;
	}
}

/* csefree: call with crypto_mtx held. */
static int
csefree(struct csession *cse)
{
	int error;

	KASSERT(mutex_owned(&crypto_mtx));
	error = crypto_freesession(cse->sid);
	if (cse->key)
		FREE(cse->key, M_XDATA);
	if (cse->mackey)
		FREE(cse->mackey, M_XDATA);
	pool_put(&csepl, cse);
	return (error);
}

static int
cryptoopen(dev_t dev, int flag, int mode,
    struct lwp *l)
{
	struct file *fp;
        struct fcrypt *fcr;
        int fd, error;

	if (crypto_usercrypto == 0)
		return (ENXIO);

	if ((error = falloc(l, &fp, &fd)) != 0)
		return error;

	fcr = pool_get(&fcrpl, PR_WAITOK);
	mutex_spin_enter(&crypto_mtx);
	TAILQ_INIT(&fcr->csessions);
	/*
	 * Don't ever return session 0, to allow detection of
	 * failed creation attempts with multi-create ioctl.
	 */
	fcr->sesn = 1;
	mutex_spin_exit(&crypto_mtx);
	return fdclone(l, fp, fd, flag, &cryptofops, fcr);
}

static int
cryptoread(dev_t dev, struct uio *uio, int ioflag)
{
	return (EIO);
}

static int
cryptowrite(dev_t dev, struct uio *uio, int ioflag)
{
	return (EIO);
}

int
cryptoselect(dev_t dev, int rw, struct lwp *l)
{
	return (0);
}

/*static*/
struct cdevsw crypto_cdevsw = {
	/* open */	cryptoopen,
	/* close */	noclose,
	/* read */	cryptoread,
	/* write */	cryptowrite,
	/* ioctl */	noioctl,
	/* ttstop?*/	nostop,
	/* ??*/		notty,
	/* poll */	cryptoselect /*nopoll*/,
	/* mmap */	nommap,
	/* kqfilter */	nokqfilter,
	/* type */	D_OTHER,
};

/*
 * Pseudo-device initialization routine for /dev/crypto
 */
void	cryptoattach(int);

void
cryptoattach(int num)
{
	pool_init(&fcrpl, sizeof(struct fcrypt), 0, 0, 0, "fcrpl",
		  NULL, IPL_NET);	/* XXX IPL_NET ("splcrypto") */
	pool_init(&csepl, sizeof(struct csession), 0, 0, 0, "csepl",
		  NULL, IPL_NET);	/* XXX IPL_NET ("splcrypto") */

	/*
	 * Preallocate space for 64 users, with 5 sessions each.
	 * (consider that a TLS protocol session requires at least
	 * 3DES, MD5, and SHA1 (both hashes are used in the PRF) for
	 * the negotiation, plus HMAC_SHA1 for the actual SSL records,
	 * consuming one session here for each algorithm.
	 */
	pool_prime(&fcrpl, 64);
	pool_prime(&csepl, 64 * 5);
}
