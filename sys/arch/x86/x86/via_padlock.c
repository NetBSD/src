/*	$OpenBSD: via.c,v 1.8 2006/11/17 07:47:56 tom Exp $	*/
/*	$NetBSD: via_padlock.c,v 1.21.6.2 2017/12/03 11:36:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2003 Jason Wright
 * Copyright (c) 2003, 2004 Theo de Raadt
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: via_padlock.c,v 1.21.6.2 2017/12/03 11:36:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/cpu.h>

#include <x86/specialreg.h>

#include <machine/cpufunc.h>
#include <machine/cpuvar.h>

#include <opencrypto/cryptodev.h>
#include <opencrypto/cryptosoft.h>
#include <opencrypto/xform.h>
#include <crypto/rijndael/rijndael.h>

#include <opencrypto/cryptosoft_xform.c>

#include <x86/via_padlock.h>

static int	via_padlock_match(device_t, cfdata_t, void *);
static void	via_padlock_attach(device_t, device_t, void *);
static int	via_padlock_detach(device_t, int);
static void	via_padlock_attach_intr(device_t);

CFATTACH_DECL_NEW(
    padlock,
    sizeof(struct via_padlock_softc),
    via_padlock_match,
    via_padlock_attach,
    via_padlock_detach,
    NULL
);

int	via_padlock_crypto_newsession(void *, uint32_t *, struct cryptoini *);
int	via_padlock_crypto_process(void *, struct cryptop *, int);
int	via_padlock_crypto_swauth(struct cryptop *, struct cryptodesc *,
	    struct swcr_data *, void *);
int	via_padlock_crypto_encdec(struct cryptop *, struct cryptodesc *,
	    struct via_padlock_session *, struct via_padlock_softc *, void *);
int	via_padlock_crypto_freesession(void *, uint64_t);
static	__inline void via_padlock_cbc(void *, void *, void *, void *, int,
	    void *);

static void
via_c3_ace_init(struct via_padlock_softc *sc)
{
	/*
	 * There is no reason to call into the kernel to use this
	 * driver from userspace, because the crypto instructions can
	 * be directly accessed there.  Setting CRYPTOCAP_F_SOFTWARE
	 * has approximately the right semantics though the name is
	 * confusing (however, consider that crypto via unprivileged
	 * instructions _is_ "just software" in some sense).
	 */
	sc->sc_cid = crypto_get_driverid(CRYPTOCAP_F_SOFTWARE);
	if (sc->sc_cid < 0) {
		aprint_error_dev(sc->sc_dev,
		    "could not get a crypto driver ID\n");
		return;
	}

	sc->sc_cid_attached = true;

	/*
	 * Ask the opencrypto subsystem to register ourselves. Although
	 * we don't support hardware offloading for various HMAC algorithms,
	 * we will handle them, because opencrypto prefers drivers that
	 * support all requested algorithms.
	 *
	 *
	 * XXX We should actually implement the HMAC modes this hardware
	 * XXX can accellerate (wrap its plain SHA1/SHA2 as HMAC) and
	 * XXX strongly consider removing those passed through to cryptosoft.
	 * XXX As it stands, we can "steal" sessions from drivers which could
	 * XXX better accellerate them.
	 *
	 * XXX Note the ordering dependency between when this (or any
	 * XXX crypto driver) attaches and when cryptosoft does.  We are
	 * XXX basically counting on the swcrypto pseudo-device to just
	 * XXX happen to attach last, or _it_ will steal every session
	 * XXX from _us_!
	 */
#define REGISTER(alg) \
	crypto_register(sc->sc_cid, alg, 0, 0, \
	    via_padlock_crypto_newsession, via_padlock_crypto_freesession, \
	    via_padlock_crypto_process, sc);

	REGISTER(CRYPTO_AES_CBC);
	REGISTER(CRYPTO_MD5_HMAC_96);
	REGISTER(CRYPTO_MD5_HMAC);
	REGISTER(CRYPTO_SHA1_HMAC_96);
	REGISTER(CRYPTO_SHA1_HMAC);
	REGISTER(CRYPTO_RIPEMD160_HMAC_96);
	REGISTER(CRYPTO_RIPEMD160_HMAC);
	REGISTER(CRYPTO_SHA2_HMAC);
}

int
via_padlock_crypto_newsession(void *arg, uint32_t *sidp, struct cryptoini *cri)
{
	struct cryptoini *c;
	struct via_padlock_softc *sc = arg;
	struct via_padlock_session *ses = NULL;
	const struct swcr_auth_hash *axf;
	struct swcr_data *swd;
	int sesn, i, cw0;

	KASSERT(sc != NULL /*, ("via_padlock_crypto_freesession: null softc")*/);
	if (sc == NULL || sidp == NULL || cri == NULL)
		return (EINVAL);

	if (sc->sc_sessions == NULL) {
		ses = sc->sc_sessions = malloc(sizeof(*ses), M_DEVBUF,
		    M_NOWAIT);
		if (ses == NULL)
			return (ENOMEM);
		sesn = 0;
		sc->sc_nsessions = 1;
	} else {
		for (sesn = 0; sesn < sc->sc_nsessions; sesn++) {
			if (sc->sc_sessions[sesn].ses_used == 0) {
				ses = &sc->sc_sessions[sesn];
				break;
			}
		}

		if (ses == NULL) {
			sesn = sc->sc_nsessions;
			ses = malloc((sesn + 1) * sizeof(*ses), M_DEVBUF,
			    M_NOWAIT);
			if (ses == NULL)
				return (ENOMEM);
			memcpy(ses, sc->sc_sessions, sesn * sizeof(*ses));
			memset(sc->sc_sessions, 0, sesn * sizeof(*ses));
			free(sc->sc_sessions, M_DEVBUF);
			sc->sc_sessions = ses;
			ses = &sc->sc_sessions[sesn];
			sc->sc_nsessions++;
		}
	}

	memset(ses, 0, sizeof(*ses));
	ses->ses_used = 1;

	for (c = cri; c != NULL; c = c->cri_next) {
		switch (c->cri_alg) {
		case CRYPTO_AES_CBC:
			switch (c->cri_klen) {
			case 128:
				cw0 = C3_CRYPT_CWLO_KEY128;
				break;
			case 192:
				cw0 = C3_CRYPT_CWLO_KEY192;
				break;
			case 256:
				cw0 = C3_CRYPT_CWLO_KEY256;
				break;
			default:
				return (EINVAL);
			}
			cw0 |= C3_CRYPT_CWLO_ALG_AES |
				C3_CRYPT_CWLO_KEYGEN_SW |
				C3_CRYPT_CWLO_NORMAL;

			cprng_fast(ses->ses_iv, sizeof(ses->ses_iv));
			ses->ses_klen = c->cri_klen;
			ses->ses_cw0 = cw0;

			/* Build expanded keys for both directions */
			rijndaelKeySetupEnc(ses->ses_ekey, c->cri_key,
			    c->cri_klen);
			rijndaelKeySetupDec(ses->ses_dkey, c->cri_key,
			    c->cri_klen);
			for (i = 0; i < 4 * (RIJNDAEL_MAXNR + 1); i++) {
				ses->ses_ekey[i] = ntohl(ses->ses_ekey[i]);
				ses->ses_dkey[i] = ntohl(ses->ses_dkey[i]);
			}

			break;

		/* Use hashing implementations from the cryptosoft code. */
		case CRYPTO_MD5_HMAC:
			axf = &swcr_auth_hash_hmac_md5;
			goto authcommon;
		case CRYPTO_MD5_HMAC_96:
			axf = &swcr_auth_hash_hmac_md5_96;
			goto authcommon;
		case CRYPTO_SHA1_HMAC:
			axf = &swcr_auth_hash_hmac_sha1;
			goto authcommon;
		case CRYPTO_SHA1_HMAC_96:
			axf = &swcr_auth_hash_hmac_sha1_96;
			goto authcommon;
		case CRYPTO_RIPEMD160_HMAC:
			axf = &swcr_auth_hash_hmac_ripemd_160;
			goto authcommon;
		case CRYPTO_RIPEMD160_HMAC_96:
			axf = &swcr_auth_hash_hmac_ripemd_160_96;
			goto authcommon;
		case CRYPTO_SHA2_HMAC:
			if (cri->cri_klen == 256)
				axf = &swcr_auth_hash_hmac_sha2_256;
			else if (cri->cri_klen == 384)
				axf = &swcr_auth_hash_hmac_sha2_384;
			else if (cri->cri_klen == 512)
				axf = &swcr_auth_hash_hmac_sha2_512;
			else {
				return EINVAL;
			}
		authcommon:
			swd = malloc(sizeof(struct swcr_data), M_CRYPTO_DATA,
			    M_NOWAIT|M_ZERO);
			if (swd == NULL) {
				via_padlock_crypto_freesession(sc, sesn);
				return (ENOMEM);
			}
			ses->swd = swd;

			swd->sw_ictx = malloc(axf->ctxsize,
			    M_CRYPTO_DATA, M_NOWAIT);
			if (swd->sw_ictx == NULL) {
				via_padlock_crypto_freesession(sc, sesn);
				return (ENOMEM);
			}

			swd->sw_octx = malloc(axf->ctxsize,
			    M_CRYPTO_DATA, M_NOWAIT);
			if (swd->sw_octx == NULL) {
				via_padlock_crypto_freesession(sc, sesn);
				return (ENOMEM);
			}

			for (i = 0; i < c->cri_klen / 8; i++)
				c->cri_key[i] ^= HMAC_IPAD_VAL;

			axf->Init(swd->sw_ictx);
			axf->Update(swd->sw_ictx, c->cri_key, c->cri_klen / 8);
			axf->Update(swd->sw_ictx, hmac_ipad_buffer,
			    HMAC_BLOCK_LEN - (c->cri_klen / 8));

			for (i = 0; i < c->cri_klen / 8; i++)
				c->cri_key[i] ^= (HMAC_IPAD_VAL ^
				    HMAC_OPAD_VAL);

			axf->Init(swd->sw_octx);
			axf->Update(swd->sw_octx, c->cri_key, c->cri_klen / 8);
			axf->Update(swd->sw_octx, hmac_opad_buffer,
			    HMAC_BLOCK_LEN - (c->cri_klen / 8));

			for (i = 0; i < c->cri_klen / 8; i++)
				c->cri_key[i] ^= HMAC_OPAD_VAL;

			swd->sw_axf = axf;
			swd->sw_alg = c->cri_alg;

			break;
		default:
			return (EINVAL);
		}
	}

	*sidp = VIAC3_SID(0, sesn);
	return (0);
}

int
via_padlock_crypto_freesession(void *arg, uint64_t tid)
{
	struct via_padlock_softc *sc = arg;
	struct swcr_data *swd;
	const struct swcr_auth_hash *axf;
	int sesn;
	uint32_t sid = ((uint32_t)tid) & 0xffffffff;

	KASSERT(sc != NULL /*, ("via_padlock_crypto_freesession: null softc")*/);
	if (sc == NULL)
		return (EINVAL);

	sesn = VIAC3_SESSION(sid);
	if (sesn >= sc->sc_nsessions)
		return (EINVAL);

	if (sc->sc_sessions[sesn].swd) {
		swd = sc->sc_sessions[sesn].swd;
		axf = swd->sw_axf;

		if (swd->sw_ictx) {
			memset(swd->sw_ictx, 0, axf->ctxsize);
			free(swd->sw_ictx, M_CRYPTO_DATA);
		}
		if (swd->sw_octx) {
			memset(swd->sw_octx, 0, axf->ctxsize);
			free(swd->sw_octx, M_CRYPTO_DATA);
		}
		free(swd, M_CRYPTO_DATA);
	}

	memset(&sc->sc_sessions[sesn], 0, sizeof(sc->sc_sessions[sesn]));
	return (0);
}

static __inline void
via_padlock_cbc(void *cw, void *src, void *dst, void *key, int rep,
    void *iv)
{
	unsigned int creg0;

	creg0 = rcr0();		/* Permit access to SIMD/FPU path */
	lcr0(creg0 & ~(CR0_EM|CR0_TS));

	/* Do the deed */
	__asm __volatile("pushfl; popfl");	/* force key reload */
	__asm __volatile(".byte 0xf3, 0x0f, 0xa7, 0xd0" : /* rep xcrypt-cbc */
			: "a" (iv), "b" (key), "c" (rep), "d" (cw), "S" (src), "D" (dst)
			: "memory", "cc");

	lcr0(creg0);
}

int
via_padlock_crypto_swauth(struct cryptop *crp, struct cryptodesc *crd,
    struct swcr_data *sw, void *buf)
{
	int	type;

	if (crp->crp_flags & CRYPTO_F_IMBUF)
		type = CRYPTO_BUF_MBUF;
	else
		type= CRYPTO_BUF_IOV;

	return (swcr_authcompute(crp, crd, sw, buf, type));
}

int
via_padlock_crypto_encdec(struct cryptop *crp, struct cryptodesc *crd,
    struct via_padlock_session *ses, struct via_padlock_softc *sc, void *buf)
{
	uint32_t *key;
	int err = 0;

	if ((crd->crd_len % 16) != 0) {
		err = EINVAL;
		return (err);
	}

	sc->op_buf = malloc(crd->crd_len, M_DEVBUF, M_NOWAIT);
	if (sc->op_buf == NULL) {
		err = ENOMEM;
		return (err);
	}

	if (crd->crd_flags & CRD_F_ENCRYPT) {
		sc->op_cw[0] = ses->ses_cw0 | C3_CRYPT_CWLO_ENCRYPT;
		key = ses->ses_ekey;
		if (crd->crd_flags & CRD_F_IV_EXPLICIT)
			memcpy(sc->op_iv, crd->crd_iv, 16);
		else
			memcpy(sc->op_iv, ses->ses_iv, 16);

		if ((crd->crd_flags & CRD_F_IV_PRESENT) == 0) {
			if (crp->crp_flags & CRYPTO_F_IMBUF)
				m_copyback((struct mbuf *)crp->crp_buf,
				    crd->crd_inject, 16, sc->op_iv);
			else if (crp->crp_flags & CRYPTO_F_IOV)
				cuio_copyback((struct uio *)crp->crp_buf,
				    crd->crd_inject, 16, sc->op_iv);
			else
				memcpy((char *)crp->crp_buf + crd->crd_inject,
				    sc->op_iv, 16);
		}
	} else {
		sc->op_cw[0] = ses->ses_cw0 | C3_CRYPT_CWLO_DECRYPT;
		key = ses->ses_dkey;
		if (crd->crd_flags & CRD_F_IV_EXPLICIT)
			memcpy(sc->op_iv, crd->crd_iv, 16);
		else {
			if (crp->crp_flags & CRYPTO_F_IMBUF)
				m_copydata((struct mbuf *)crp->crp_buf,
				    crd->crd_inject, 16, sc->op_iv);
			else if (crp->crp_flags & CRYPTO_F_IOV)
				cuio_copydata((struct uio *)crp->crp_buf,
				    crd->crd_inject, 16, sc->op_iv);
			else
				memcpy(sc->op_iv, (char *)crp->crp_buf +
				    crd->crd_inject, 16);
		}
	}

	if (crp->crp_flags & CRYPTO_F_IMBUF)
		m_copydata((struct mbuf *)crp->crp_buf,
		    crd->crd_skip, crd->crd_len, sc->op_buf);
	else if (crp->crp_flags & CRYPTO_F_IOV)
		cuio_copydata((struct uio *)crp->crp_buf,
		    crd->crd_skip, crd->crd_len, sc->op_buf);
	else
		memcpy(sc->op_buf, (char *)crp->crp_buf + crd->crd_skip,
		    crd->crd_len);

	sc->op_cw[1] = sc->op_cw[2] = sc->op_cw[3] = 0;
	via_padlock_cbc(&sc->op_cw, sc->op_buf, sc->op_buf, key,
	    crd->crd_len / 16, sc->op_iv);

	if (crp->crp_flags & CRYPTO_F_IMBUF)
		m_copyback((struct mbuf *)crp->crp_buf,
		    crd->crd_skip, crd->crd_len, sc->op_buf);
	else if (crp->crp_flags & CRYPTO_F_IOV)
		cuio_copyback((struct uio *)crp->crp_buf,
		    crd->crd_skip, crd->crd_len, sc->op_buf);
	else
		memcpy((char *)crp->crp_buf + crd->crd_skip, sc->op_buf,
		    crd->crd_len);

	/* copy out last block for use as next session IV */
	if (crd->crd_flags & CRD_F_ENCRYPT) {
		if (crp->crp_flags & CRYPTO_F_IMBUF)
			m_copydata((struct mbuf *)crp->crp_buf,
			    crd->crd_skip + crd->crd_len - 16, 16,
			    ses->ses_iv);
		else if (crp->crp_flags & CRYPTO_F_IOV)
			cuio_copydata((struct uio *)crp->crp_buf,
			    crd->crd_skip + crd->crd_len - 16, 16,
			    ses->ses_iv);
		else
			memcpy(ses->ses_iv, (char *)crp->crp_buf +
			    crd->crd_skip + crd->crd_len - 16, 16);
	}

	if (sc->op_buf != NULL) {
		memset(sc->op_buf, 0, crd->crd_len);
		free(sc->op_buf, M_DEVBUF);
		sc->op_buf = NULL;
	}

	return (err);
}

int
via_padlock_crypto_process(void *arg, struct cryptop *crp, int hint)
{
	struct via_padlock_softc *sc = arg;
	struct via_padlock_session *ses;
	struct cryptodesc *crd;
	int sesn, err = 0;

	KASSERT(sc != NULL /*, ("via_padlock_crypto_process: null softc")*/);
	if (crp == NULL || crp->crp_callback == NULL) {
		err = EINVAL;
		goto out;
	}

	sesn = VIAC3_SESSION(crp->crp_sid);
	if (sesn >= sc->sc_nsessions) {
		err = EINVAL;
		goto out;
	}
	ses = &sc->sc_sessions[sesn];

	for (crd = crp->crp_desc; crd; crd = crd->crd_next) {
		switch (crd->crd_alg) {
		case CRYPTO_AES_CBC:
			if ((err = via_padlock_crypto_encdec(crp, crd, ses,
			    sc, crp->crp_buf)) != 0)
				goto out;
			break;

		case CRYPTO_MD5_HMAC:
		case CRYPTO_SHA1_HMAC:
		case CRYPTO_RIPEMD160_HMAC:
		case CRYPTO_SHA2_HMAC:
			if ((err = via_padlock_crypto_swauth(crp, crd,
			    ses->swd, crp->crp_buf)) != 0)
				goto out;
			break;

		default:
			err = EINVAL;
			goto out;
		}
	}
out:
	crp->crp_etype = err;
	crypto_done(crp);
	return (err);
}

static int
via_padlock_match(device_t parent, cfdata_t cf, void *opaque)
{
	struct cpufeature_attach_args *cfaa = opaque;
	struct cpu_info *ci = cfaa->ci;

	if (strcmp(cfaa->name, "padlock") != 0)
		return 0;
	if ((cpu_feature[4] & (CPUID_VIA_HAS_ACE|CPUID_VIA_HAS_RNG)) == 0)
		return 0;
	if ((ci->ci_flags & (CPUF_BSP|CPUF_SP|CPUF_PRIMARY)) == 0)
		return 0;
	return 1;
}

static void
via_padlock_attach(device_t parent, device_t self, void *opaque)
{
	struct via_padlock_softc *sc = device_private(self);

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal(": VIA PadLock\n");

	pmf_device_register(self, NULL, NULL);

	config_interrupts(self, via_padlock_attach_intr);
}

static void
via_padlock_attach_intr(device_t self)
{
	struct via_padlock_softc *sc = device_private(self);

	aprint_normal("%s:", device_xname(self));
	if (cpu_feature[4] & CPUID_VIA_HAS_RNG) {
		aprint_normal(" RNG");
	}
	if (cpu_feature[4] & CPUID_VIA_HAS_ACE) {
		via_c3_ace_init(sc);
		aprint_normal(" ACE");
	}
	aprint_normal("\n");
}

static int
via_padlock_detach(device_t self, int flags)
{
	struct via_padlock_softc *sc = device_private(self);

	if (sc->sc_cid_attached) {
		crypto_unregister(sc->sc_cid, CRYPTO_AES_CBC);
		crypto_unregister(sc->sc_cid, CRYPTO_MD5_HMAC_96);
		crypto_unregister(sc->sc_cid, CRYPTO_MD5_HMAC);
		crypto_unregister(sc->sc_cid, CRYPTO_SHA1_HMAC_96);
		crypto_unregister(sc->sc_cid, CRYPTO_SHA1_HMAC);
		crypto_unregister(sc->sc_cid, CRYPTO_RIPEMD160_HMAC_96);
		crypto_unregister(sc->sc_cid, CRYPTO_RIPEMD160_HMAC);
		crypto_unregister(sc->sc_cid, CRYPTO_SHA2_HMAC);
		sc->sc_cid_attached = false;
	}

	pmf_device_deregister(self);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, padlock, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
padlock_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_padlock,
		    cfattach_ioconf_padlock, cfdata_ioconf_padlock);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_padlock,
		    cfattach_ioconf_padlock, cfdata_ioconf_padlock);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
