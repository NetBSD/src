/*	$NetBSD: glxsb.c,v 1.12.4.2 2016/10/05 20:55:28 skrll Exp $	*/
/* $OpenBSD: glxsb.c,v 1.7 2007/02/12 14:31:45 tom Exp $ */

/*
 * Copyright (c) 2006 Tom Cosgrove <tom@openbsd.org>
 * Copyright (c) 2003, 2004 Theo de Raadt
 * Copyright (c) 2003 Jason Wright
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

/*
 * Driver for the security block on the AMD Geode LX processors
 * http://www.amd.com/files/connectivitysolutions/geode/geode_lx/33234d_lx_ds.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: glxsb.c,v 1.12.4.2 2016/10/05 20:55:28 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/types.h>
#include <sys/callout.h>
#include <sys/bus.h>
#include <sys/cprng.h>
#include <sys/rndsource.h>

#include <machine/cpufunc.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <opencrypto/cryptodev.h>
#include <crypto/rijndael/rijndael.h>

#define SB_GLD_MSR_CAP		0x58002000	/* RO - Capabilities */
#define SB_GLD_MSR_CONFIG	0x58002001	/* RW - Master Config */
#define SB_GLD_MSR_SMI		0x58002002	/* RW - SMI */
#define SB_GLD_MSR_ERROR	0x58002003	/* RW - Error */
#define SB_GLD_MSR_PM		0x58002004	/* RW - Power Mgmt */
#define SB_GLD_MSR_DIAG		0x58002005	/* RW - Diagnostic */
#define SB_GLD_MSR_CTRL		0x58002006	/* RW - Security Block Cntrl */

						/* For GLD_MSR_CTRL: */
#define SB_GMC_DIV0		0x0000		/* AES update divisor values */
#define SB_GMC_DIV1		0x0001
#define SB_GMC_DIV2		0x0002
#define SB_GMC_DIV3		0x0003
#define SB_GMC_DIV_MASK		0x0003
#define SB_GMC_SBI		0x0004		/* AES swap bits */
#define SB_GMC_SBY		0x0008		/* AES swap bytes */
#define SB_GMC_TW		0x0010		/* Time write (EEPROM) */
#define SB_GMC_T_SEL0		0x0000		/* RNG post-proc: none */
#define SB_GMC_T_SEL1		0x0100		/* RNG post-proc: LFSR */
#define SB_GMC_T_SEL2		0x0200		/* RNG post-proc: whitener */
#define SB_GMC_T_SEL3		0x0300		/* RNG LFSR+whitener */
#define SB_GMC_T_SEL_MASK	0x0300
#define SB_GMC_T_NE		0x0400		/* Noise (generator) Enable */
#define SB_GMC_T_TM		0x0800		/* RNG test mode */
						/*     (deterministic) */

/* Security Block configuration/control registers (offsets from base) */

#define SB_CTL_A		0x0000		/* RW - SB Control A */
#define SB_CTL_B		0x0004		/* RW - SB Control B */
#define SB_AES_INT		0x0008		/* RW - SB AES Interrupt */
#define SB_SOURCE_A		0x0010		/* RW - Source A */
#define SB_DEST_A		0x0014		/* RW - Destination A */
#define SB_LENGTH_A		0x0018		/* RW - Length A */
#define SB_SOURCE_B		0x0020		/* RW - Source B */
#define SB_DEST_B		0x0024		/* RW - Destination B */
#define SB_LENGTH_B		0x0028		/* RW - Length B */
#define SB_WKEY			0x0030		/* WO - Writable Key 0-3 */
#define SB_WKEY_0		0x0030		/* WO - Writable Key 0 */
#define SB_WKEY_1		0x0034		/* WO - Writable Key 1 */
#define SB_WKEY_2		0x0038		/* WO - Writable Key 2 */
#define SB_WKEY_3		0x003C		/* WO - Writable Key 3 */
#define SB_CBC_IV		0x0040		/* RW - CBC IV 0-3 */
#define SB_CBC_IV_0		0x0040		/* RW - CBC IV 0 */
#define SB_CBC_IV_1		0x0044		/* RW - CBC IV 1 */
#define SB_CBC_IV_2		0x0048		/* RW - CBC IV 2 */
#define SB_CBC_IV_3		0x004C		/* RW - CBC IV 3 */
#define SB_RANDOM_NUM		0x0050		/* RW - Random Number */
#define SB_RANDOM_NUM_STATUS	0x0054		/* RW - Random Number Status */
#define SB_EEPROM_COMM		0x0800		/* RW - EEPROM Command */
#define SB_EEPROM_ADDR		0x0804		/* RW - EEPROM Address */
#define SB_EEPROM_DATA		0x0808		/* RW - EEPROM Data */
#define SB_EEPROM_SEC_STATE	0x080C		/* RW - EEPROM Security State */

						/* For SB_CTL_A and _B */
#define SB_CTL_ST		0x0001		/* Start operation (enc/dec) */
#define SB_CTL_ENC		0x0002		/* Encrypt (0 is decrypt) */
#define SB_CTL_DEC		0x0000		/* Decrypt */
#define SB_CTL_WK		0x0004		/* Use writable key (we set) */
#define SB_CTL_DC		0x0008		/* Destination coherent */
#define SB_CTL_SC		0x0010		/* Source coherent */
#define SB_CTL_CBC		0x0020		/* CBC (0 is ECB) */

						/* For SB_AES_INT */
#define SB_AI_DISABLE_AES_A	0x0001		/* Disable AES A compl int */
#define SB_AI_ENABLE_AES_A	0x0000		/* Enable AES A compl int */
#define SB_AI_DISABLE_AES_B	0x0002		/* Disable AES B compl int */
#define SB_AI_ENABLE_AES_B	0x0000		/* Enable AES B compl int */
#define SB_AI_DISABLE_EEPROM	0x0004		/* Disable EEPROM op comp int */
#define SB_AI_ENABLE_EEPROM	0x0000		/* Enable EEPROM op compl int */
#define SB_AI_AES_A_COMPLETE	0x0100		/* AES A operation complete */
#define SB_AI_AES_B_COMPLETE	0x0200		/* AES B operation complete */
#define SB_AI_EEPROM_COMPLETE	0x0400		/* EEPROM operation complete */

#define SB_RNS_TRNG_VALID	0x0001		/* in SB_RANDOM_NUM_STATUS */

#define SB_MEM_SIZE		0x0810		/* Size of memory block */

#define SB_AES_ALIGN		0x0010		/* Source and dest buffers */
						/* must be 16-byte aligned */
#define SB_AES_BLOCK_SIZE	0x0010

/*
 * The Geode LX security block AES acceleration doesn't perform scatter-
 * gather: it just takes source and destination addresses.  Therefore the
 * plain- and ciphertexts need to be contiguous.  To this end, we allocate
 * a buffer for both, and accept the overhead of copying in and out.  If
 * the number of bytes in one operation is bigger than allowed for by the
 * buffer (buffer is twice the size of the max length, as it has both input
 * and output) then we have to perform multiple encryptions/decryptions.
 */
#define GLXSB_MAX_AES_LEN	16384

struct glxsb_dma_map {
	bus_dmamap_t		dma_map;
	bus_dma_segment_t	dma_seg;
	int			dma_nsegs;
	int			dma_size;
	void *			dma_vaddr;
	uint32_t		dma_paddr;
};
struct glxsb_session {
	uint32_t	ses_key[4];
	uint8_t		ses_iv[SB_AES_BLOCK_SIZE];
	int		ses_klen;
	int		ses_used;
};

struct glxsb_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct callout		sc_co;

	bus_dma_tag_t		sc_dmat;
	struct glxsb_dma_map	sc_dma;
	int32_t			sc_cid;
	int			sc_nsessions;
	struct glxsb_session	*sc_sessions;

	krndsource_t	sc_rnd_source;
};

int	glxsb_match(device_t, cfdata_t, void *);
void	glxsb_attach(device_t, device_t, void *);
void	glxsb_rnd(void *);

CFATTACH_DECL_NEW(glxsb, sizeof(struct glxsb_softc),
    glxsb_match, glxsb_attach, NULL, NULL);

#define GLXSB_SESSION(sid)		((sid) & 0x0fffffff)
#define	GLXSB_SID(crd,ses)		(((crd) << 28) | ((ses) & 0x0fffffff))

int glxsb_crypto_setup(struct glxsb_softc *);
int glxsb_crypto_newsession(void *, uint32_t *, struct cryptoini *);
int glxsb_crypto_process(void *, struct cryptop *, int);
int glxsb_crypto_freesession(void *, uint64_t);
static __inline void glxsb_aes(struct glxsb_softc *, uint32_t, uint32_t,
    uint32_t, void *, int, void *);

int glxsb_dma_alloc(struct glxsb_softc *, int, struct glxsb_dma_map *);
void glxsb_dma_pre_op(struct glxsb_softc *, struct glxsb_dma_map *);
void glxsb_dma_post_op(struct glxsb_softc *, struct glxsb_dma_map *);
void glxsb_dma_free(struct glxsb_softc *, struct glxsb_dma_map *);

int
glxsb_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_AMD &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_AMD_GEODELX_AES)
		return (1);

	return (0);
}

void
glxsb_attach(device_t parent, device_t self, void *aux)
{
	struct glxsb_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	bus_addr_t membase;
	bus_size_t memsize;
	uint64_t msr;
	uint32_t intr;

	msr = rdmsr(SB_GLD_MSR_CAP);
	if ((msr & 0xFFFF00) != 0x130400) {
		aprint_error(": unknown ID 0x%x\n",
		    (int)((msr & 0xFFFF00) >> 16));
		return;
	}

	/* printf(": revision %d", (int) (msr & 0xFF)); */

	/* Map in the security block configuration/control registers */
	if (pci_mapreg_map(pa, PCI_MAPREG_START,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &sc->sc_iot, &sc->sc_ioh, &membase, &memsize)) {
		aprint_error(": can't find mem space\n");
		return;
	}

	sc->sc_dev = self;

	/*
	 * Configure the Security Block.
	 *
	 * We want to enable the noise generator (T_NE), and enable the
	 * linear feedback shift register and whitener post-processing
	 * (T_SEL = 3).  Also ensure that test mode (deterministic values)
	 * is disabled.
	 */
	msr = rdmsr(SB_GLD_MSR_CTRL);
	msr &= ~(SB_GMC_T_TM | SB_GMC_T_SEL_MASK);
	msr |= SB_GMC_T_NE | SB_GMC_T_SEL3;
#if 0
	msr |= SB_GMC_SBI | SB_GMC_SBY;		/* for AES, if necessary */
#endif
	wrmsr(SB_GLD_MSR_CTRL, msr);

	rnd_attach_source(&sc->sc_rnd_source, device_xname(self),
			  RND_TYPE_RNG, RND_FLAG_COLLECT_VALUE);

	/* Install a periodic collector for the "true" (AMD's word) RNG */
	callout_init(&sc->sc_co, 0);
	callout_setfunc(&sc->sc_co, glxsb_rnd, sc);
	glxsb_rnd(sc);
	aprint_normal(": RNG");

	/* We don't have an interrupt handler, so disable completion INTs */
	intr = SB_AI_DISABLE_AES_A | SB_AI_DISABLE_AES_B |
	    SB_AI_DISABLE_EEPROM | SB_AI_AES_A_COMPLETE |
	    SB_AI_AES_B_COMPLETE | SB_AI_EEPROM_COMPLETE;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SB_AES_INT, intr);

	sc->sc_dmat = pa->pa_dmat;

	if (glxsb_crypto_setup(sc))
		aprint_normal(" AES");

	aprint_normal("\n");
}

void
glxsb_rnd(void *v)
{
	struct glxsb_softc *sc = v;
	uint32_t status, value;
	extern int hz;

	status = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SB_RANDOM_NUM_STATUS);
	if (status & SB_RNS_TRNG_VALID) {
		value = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SB_RANDOM_NUM);
		rnd_add_data(&sc->sc_rnd_source, &value, sizeof(value),
			     sizeof(value) * NBBY);
	}

	callout_schedule(&sc->sc_co, (hz > 100) ? (hz / 100) : 1);
}

int
glxsb_crypto_setup(struct glxsb_softc *sc)
{

	/* Allocate a contiguous DMA-able buffer to work in */
	if (glxsb_dma_alloc(sc, GLXSB_MAX_AES_LEN * 2, &sc->sc_dma) != 0)
		return 0;

	sc->sc_cid = crypto_get_driverid(0);
	if (sc->sc_cid < 0)
		return 0;

	crypto_register(sc->sc_cid, CRYPTO_AES_CBC, 0, 0,
	    glxsb_crypto_newsession, glxsb_crypto_freesession,
	    glxsb_crypto_process, sc);

	sc->sc_nsessions = 0;

	return 1;
}

int
glxsb_crypto_newsession(void *aux, uint32_t *sidp, struct cryptoini *cri)
{
	struct glxsb_softc *sc = aux;
	struct glxsb_session *ses = NULL;
	int sesn;

	if (sc == NULL || sidp == NULL || cri == NULL ||
	    cri->cri_next != NULL || cri->cri_alg != CRYPTO_AES_CBC ||
	    cri->cri_klen != 128)
		return (EINVAL);

	for (sesn = 0; sesn < sc->sc_nsessions; sesn++) {
		if (sc->sc_sessions[sesn].ses_used == 0) {
			ses = &sc->sc_sessions[sesn];
			break;
		}
	}

	if (ses == NULL) {
		sesn = sc->sc_nsessions;
		ses = malloc((sesn + 1) * sizeof(*ses), M_DEVBUF, M_NOWAIT);
		if (ses == NULL)
			return (ENOMEM);
		if (sesn != 0) {
			memcpy(ses, sc->sc_sessions, sesn * sizeof(*ses));
			memset(sc->sc_sessions, 0, sesn * sizeof(*ses));
			free(sc->sc_sessions, M_DEVBUF);
		}
		sc->sc_sessions = ses;
		ses = &sc->sc_sessions[sesn];
		sc->sc_nsessions++;
	}

	memset(ses, 0, sizeof(*ses));
	ses->ses_used = 1;

	cprng_fast(ses->ses_iv, sizeof(ses->ses_iv));
	ses->ses_klen = cri->cri_klen;

	/* Copy the key (Geode LX wants the primary key only) */
	memcpy(ses->ses_key, cri->cri_key, sizeof(ses->ses_key));

	*sidp = GLXSB_SID(0, sesn);
	return (0);
}

int
glxsb_crypto_freesession(void *aux, uint64_t tid)
{
	struct glxsb_softc *sc = aux;
	int sesn;
	uint32_t sid = ((uint32_t)tid) & 0xffffffff;

	if (sc == NULL)
		return (EINVAL);
	sesn = GLXSB_SESSION(sid);
	if (sesn >= sc->sc_nsessions)
		return (EINVAL);
	memset(&sc->sc_sessions[sesn], 0, sizeof(sc->sc_sessions[sesn]));
	return (0);
}

/*
 * Must be called at splnet() or higher
 */
static __inline void
glxsb_aes(struct glxsb_softc *sc, uint32_t control, uint32_t psrc,
    uint32_t pdst, void *key, int len, void *iv)
{
	uint32_t status;
	int i;

	if (len & 0xF) {
		printf("%s: len must be a multiple of 16 (not %d)\n",
		    device_xname(sc->sc_dev), len);
		return;
	}

	/* Set the source */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SB_SOURCE_A, psrc);

	/* Set the destination address */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SB_DEST_A, pdst);

	/* Set the data length */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SB_LENGTH_A, len);

	/* Set the IV */
	if (iv != NULL) {
		bus_space_write_region_4(sc->sc_iot, sc->sc_ioh,
		    SB_CBC_IV, iv, 4);
		control |= SB_CTL_CBC;
	}

	/* Set the key */
	bus_space_write_region_4(sc->sc_iot, sc->sc_ioh, SB_WKEY, key, 4);

	/* Ask the security block to do it */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SB_CTL_A,
	    control | SB_CTL_WK | SB_CTL_DC | SB_CTL_SC | SB_CTL_ST);

	/*
	 * Now wait until it is done.
	 *
	 * We do a busy wait.  Obviously the number of iterations of
	 * the loop required to perform the AES operation depends upon
	 * the number of bytes to process.
	 *
	 * On a 500 MHz Geode LX we see
	 *
	 *	length (bytes)	typical max iterations
	 *	    16		   12
	 *	    64		   22
	 *	   256		   59
	 *	  1024		  212
	 *	  8192		1,537
	 *
	 * Since we have a maximum size of operation defined in
	 * GLXSB_MAX_AES_LEN, we use this constant to decide how long
	 * to wait.  Allow an order of magnitude longer than it should
	 * really take, just in case.
	 */
	for (i = 0; i < GLXSB_MAX_AES_LEN * 10; i++) {
		status = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SB_CTL_A);

		if ((status & SB_CTL_ST) == 0)		/* Done */
			return;
	}

	aprint_error_dev(sc->sc_dev, "operation failed to complete\n");
}

int
glxsb_crypto_process(void *aux, struct cryptop *crp, int hint)
{
	struct glxsb_softc *sc = aux;
	struct glxsb_session *ses;
	struct cryptodesc *crd;
	char *op_src, *op_dst;
	uint32_t op_psrc, op_pdst;
	uint8_t op_iv[SB_AES_BLOCK_SIZE], *piv;
	int sesn, err = 0;
	int len, tlen, xlen;
	int offset;
	uint32_t control;
	int s;

	s = splnet();

	if (crp == NULL || crp->crp_callback == NULL) {
		err = EINVAL;
		goto out;
	}
	crd = crp->crp_desc;
	if (crd == NULL || crd->crd_next != NULL ||
	    crd->crd_alg != CRYPTO_AES_CBC ||
	    (crd->crd_len % SB_AES_BLOCK_SIZE) != 0) {
		err = EINVAL;
		goto out;
	}

	sesn = GLXSB_SESSION(crp->crp_sid);
	if (sesn >= sc->sc_nsessions) {
		err = EINVAL;
		goto out;
	}
	ses = &sc->sc_sessions[sesn];

	/* How much of our buffer will we need to use? */
	xlen = crd->crd_len > GLXSB_MAX_AES_LEN ?
	    GLXSB_MAX_AES_LEN : crd->crd_len;

	/*
	 * XXX Check if we can have input == output on Geode LX.
	 * XXX In the meantime, use two separate (adjacent) buffers.
	 */
	op_src = sc->sc_dma.dma_vaddr;
	op_dst = (char *)sc->sc_dma.dma_vaddr + xlen;

	op_psrc = sc->sc_dma.dma_paddr;
	op_pdst = sc->sc_dma.dma_paddr + xlen;

	if (crd->crd_flags & CRD_F_ENCRYPT) {
		control = SB_CTL_ENC;
		if (crd->crd_flags & CRD_F_IV_EXPLICIT)
			memcpy(op_iv, crd->crd_iv, sizeof(op_iv));
		else
			memcpy(op_iv, ses->ses_iv, sizeof(op_iv));

		if ((crd->crd_flags & CRD_F_IV_PRESENT) == 0) {
			if (crp->crp_flags & CRYPTO_F_IMBUF)
				m_copyback((struct mbuf *)crp->crp_buf,
				    crd->crd_inject, sizeof(op_iv), op_iv);
			else if (crp->crp_flags & CRYPTO_F_IOV)
				cuio_copyback((struct uio *)crp->crp_buf,
				    crd->crd_inject, sizeof(op_iv), op_iv);
			else
				bcopy(op_iv,
				    (char *)crp->crp_buf + crd->crd_inject,
				    sizeof(op_iv));
		}
	} else {
		control = SB_CTL_DEC;
		if (crd->crd_flags & CRD_F_IV_EXPLICIT)
			memcpy(op_iv, crd->crd_iv, sizeof(op_iv));
		else {
			if (crp->crp_flags & CRYPTO_F_IMBUF)
				m_copydata((struct mbuf *)crp->crp_buf,
				    crd->crd_inject, sizeof(op_iv), op_iv);
			else if (crp->crp_flags & CRYPTO_F_IOV)
				cuio_copydata((struct uio *)crp->crp_buf,
				    crd->crd_inject, sizeof(op_iv), op_iv);
			else
				bcopy((char *)crp->crp_buf + crd->crd_inject,
				    op_iv, sizeof(op_iv));
		}
	}

	offset = 0;
	tlen = crd->crd_len;
	piv = op_iv;

	/* Process the data in GLXSB_MAX_AES_LEN chunks */
	while (tlen > 0) {
		len = (tlen > GLXSB_MAX_AES_LEN) ? GLXSB_MAX_AES_LEN : tlen;

		if (crp->crp_flags & CRYPTO_F_IMBUF)
			m_copydata((struct mbuf *)crp->crp_buf,
			    crd->crd_skip + offset, len, op_src);
		else if (crp->crp_flags & CRYPTO_F_IOV)
			cuio_copydata((struct uio *)crp->crp_buf,
			    crd->crd_skip + offset, len, op_src);
		else
			bcopy((char *)crp->crp_buf + crd->crd_skip + offset,
			    op_src, len);

		glxsb_dma_pre_op(sc, &sc->sc_dma);

		glxsb_aes(sc, control, op_psrc, op_pdst, ses->ses_key,
		    len, op_iv);

		glxsb_dma_post_op(sc, &sc->sc_dma);

		if (crp->crp_flags & CRYPTO_F_IMBUF)
			m_copyback((struct mbuf *)crp->crp_buf,
			    crd->crd_skip + offset, len, op_dst);
		else if (crp->crp_flags & CRYPTO_F_IOV)
			cuio_copyback((struct uio *)crp->crp_buf,
			    crd->crd_skip + offset, len, op_dst);
		else
			memcpy((char *)crp->crp_buf + crd->crd_skip + offset,
			    op_dst, len);

		offset += len;
		tlen -= len;

		if (tlen <= 0) {	/* Ideally, just == 0 */
			/* Finished - put the IV in session IV */
			piv = ses->ses_iv;
		}

		/*
		 * Copy out last block for use as next iteration/session IV.
		 *
		 * piv is set to op_iv[] before the loop starts, but is
		 * set to ses->ses_iv if we're going to exit the loop this
		 * time.
		 */
		if (crd->crd_flags & CRD_F_ENCRYPT) {
			memcpy(piv, op_dst + len - sizeof(op_iv),
			    sizeof(op_iv));
		} else {
			/* Decryption, only need this if another iteration */
			if (tlen > 0) {
				memcpy(piv, op_src + len - sizeof(op_iv),
				    sizeof(op_iv));
			}
		}
	}

	/* All AES processing has now been done. */

	memset(sc->sc_dma.dma_vaddr, 0, xlen * 2);
out:
	crp->crp_etype = err;
	crypto_done(crp);
	splx(s);
	return (err);
}

int
glxsb_dma_alloc(struct glxsb_softc *sc, int size, struct glxsb_dma_map *dma)
{
	int rc;

	dma->dma_nsegs = 1;
	dma->dma_size = size;

	rc = bus_dmamap_create(sc->sc_dmat, size, dma->dma_nsegs, size,
	    0, BUS_DMA_NOWAIT, &dma->dma_map);
	if (rc != 0) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't create DMA map for %d bytes (%d)\n", size, rc);

		goto fail0;
	}

	rc = bus_dmamem_alloc(sc->sc_dmat, size, SB_AES_ALIGN, 0,
	    &dma->dma_seg, dma->dma_nsegs, &dma->dma_nsegs, BUS_DMA_NOWAIT);
	if (rc != 0) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't allocate DMA memory of %d bytes (%d)\n",
		    size, rc);

		goto fail1;
	}

	rc = bus_dmamem_map(sc->sc_dmat, &dma->dma_seg, 1, size,
	    &dma->dma_vaddr, BUS_DMA_NOWAIT);
	if (rc != 0) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't map DMA memory for %d bytes (%d)\n", size, rc);

		goto fail2;
	}

	rc = bus_dmamap_load(sc->sc_dmat, dma->dma_map, dma->dma_vaddr,
	    size, NULL, BUS_DMA_NOWAIT);
	if (rc != 0) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't load DMA memory for %d bytes (%d)\n", size, rc);

		goto fail3;
	}

	dma->dma_paddr = dma->dma_map->dm_segs[0].ds_addr;

	return 0;

fail3:
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_vaddr, size);
fail2:
	bus_dmamem_free(sc->sc_dmat, &dma->dma_seg, dma->dma_nsegs);
fail1:
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
fail0:
	return rc;
}

void
glxsb_dma_pre_op(struct glxsb_softc *sc, struct glxsb_dma_map *dma)
{
	bus_dmamap_sync(sc->sc_dmat, dma->dma_map, 0, dma->dma_size,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
}

void
glxsb_dma_post_op(struct glxsb_softc *sc, struct glxsb_dma_map *dma)
{
	bus_dmamap_sync(sc->sc_dmat, dma->dma_map, 0, dma->dma_size,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
}

void
glxsb_dma_free(struct glxsb_softc *sc, struct glxsb_dma_map *dma)
{
	bus_dmamap_unload(sc->sc_dmat, dma->dma_map);
	bus_dmamem_unmap(sc->sc_dmat, dma->dma_vaddr, dma->dma_size);
	bus_dmamem_free(sc->sc_dmat, &dma->dma_seg, dma->dma_nsegs);
	bus_dmamap_destroy(sc->sc_dmat, dma->dma_map);
}
