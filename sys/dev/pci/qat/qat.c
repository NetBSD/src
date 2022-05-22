/*	$NetBSD: qat.c,v 1.8 2022/05/22 11:39:27 riastradh Exp $	*/

/*
 * Copyright (c) 2019 Internet Initiative Japan, Inc.
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

/*
 *   Copyright(c) 2007-2019 Intel Corporation. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: qat.c,v 1.8 2022/05/22 11:39:27 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/bitops.h>
#include <sys/atomic.h>
#include <sys/mbuf.h>
#include <sys/cprng.h>
#include <sys/cpu.h>
#include <sys/interrupt.h>
#include <sys/md5.h>
#include <sys/sha1.h>
#include <sys/sha2.h>

#include <opencrypto/cryptodev.h>
#include <opencrypto/cryptosoft.h>
#include <opencrypto/xform.h>

/* XXX same as sys/arch/x86/x86/via_padlock.c */
#include <opencrypto/cryptosoft_xform.c>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include "qatreg.h"
#include "qatvar.h"
#include "qat_aevar.h"

extern struct qat_hw qat_hw_c2xxx;
extern struct qat_hw qat_hw_c3xxx;
extern struct qat_hw qat_hw_c62x;
extern struct qat_hw qat_hw_d15xx;

static const struct qat_product {
	pci_vendor_id_t qatp_vendor;
	pci_product_id_t qatp_product;
	const char *qatp_name;
	enum qat_chip_type qatp_chip;
	const struct qat_hw *qatp_hw;
} qat_products[] = {

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_C2000_IQIA_PHYS,
	  "Intel C2000 QuickAssist Physical Function",
	  QAT_CHIP_C2XXX, &qat_hw_c2xxx },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_C3K_QAT,
	  "Intel C3000 QuickAssist Physical Function",
	  QAT_CHIP_C3XXX, &qat_hw_c3xxx },
#ifdef notyet
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_C3K_QAT_VF,
	  "Intel C3000 QuickAssist Virtual Function",
	  QAT_CHIP_C3XXX_IOV, &qat_hw_c3xxxvf },
#endif
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_C620_QAT,
	  "Intel C620/Xeon D-2100 QuickAssist Physical Function",
	  QAT_CHIP_C62X, &qat_hw_c62x },
#ifdef notyet
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_C620_QAT_VF,
	  "Intel C620/Xeon D-2100 QuickAssist Virtual Function",
	  QAT_CHIP_C62X_IOV, &qat_hw_c62xvf },
#endif
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XEOND_QAT,
	  "Intel Xeon D-1500 QuickAssist Physical Function",
	  QAT_CHIP_D15XX, &qat_hw_d15xx },
#ifdef notyet
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XEOND_QAT_VF,
	  "Intel Xeon D-1500 QuickAssist Virtual Function",
	  QAT_CHIP_D15XX_IOV, &qat_hw_d15xxvf },
#endif
	{ 0, 0, NULL, 0, NULL },
};

/* md5 16 bytes - Initialiser state can be found in RFC 1321*/
static const uint8_t md5_initial_state[QAT_HASH_MD5_STATE_SIZE] = {
	0x01, 0x23, 0x45, 0x67,
	0x89, 0xab, 0xcd, 0xef,
	0xfe, 0xdc, 0xba, 0x98,
	0x76, 0x54, 0x32, 0x10,
};

/* SHA1 - 20 bytes - Initialiser state can be found in FIPS stds 180-2 */
static const uint8_t sha1_initial_state[QAT_HASH_SHA1_STATE_SIZE] = {
	0x67, 0x45, 0x23, 0x01,
	0xef, 0xcd, 0xab, 0x89,
	0x98, 0xba, 0xdc, 0xfe,
	0x10, 0x32, 0x54, 0x76,
	0xc3, 0xd2, 0xe1, 0xf0
};

/* SHA 256 - 32 bytes - Initialiser state can be found in FIPS stds 180-2 */
static const uint8_t sha256_initial_state[QAT_HASH_SHA256_STATE_SIZE] = {
	0x6a, 0x09, 0xe6, 0x67,
	0xbb, 0x67, 0xae, 0x85,
	0x3c, 0x6e, 0xf3, 0x72,
	0xa5, 0x4f, 0xf5, 0x3a,
	0x51, 0x0e, 0x52, 0x7f,
	0x9b, 0x05, 0x68, 0x8c,
	0x1f, 0x83, 0xd9, 0xab,
	0x5b, 0xe0, 0xcd, 0x19
};

/* SHA 384 - 64 bytes - Initialiser state can be found in FIPS stds 180-2 */
static const uint8_t sha384_initial_state[QAT_HASH_SHA384_STATE_SIZE] = {
	0xcb, 0xbb, 0x9d, 0x5d, 0xc1, 0x05, 0x9e, 0xd8,
	0x62, 0x9a, 0x29, 0x2a, 0x36, 0x7c, 0xd5, 0x07,
	0x91, 0x59, 0x01, 0x5a, 0x30, 0x70, 0xdd, 0x17,
	0x15, 0x2f, 0xec, 0xd8, 0xf7, 0x0e, 0x59, 0x39,
	0x67, 0x33, 0x26, 0x67, 0xff, 0xc0, 0x0b, 0x31,
	0x8e, 0xb4, 0x4a, 0x87, 0x68, 0x58, 0x15, 0x11,
	0xdb, 0x0c, 0x2e, 0x0d, 0x64, 0xf9, 0x8f, 0xa7,
	0x47, 0xb5, 0x48, 0x1d, 0xbe, 0xfa, 0x4f, 0xa4
};

/* SHA 512 - 64 bytes - Initialiser state can be found in FIPS stds 180-2 */
static const uint8_t sha512_initial_state[QAT_HASH_SHA512_STATE_SIZE] = {
	0x6a, 0x09, 0xe6, 0x67, 0xf3, 0xbc, 0xc9, 0x08,
	0xbb, 0x67, 0xae, 0x85, 0x84, 0xca, 0xa7, 0x3b,
	0x3c, 0x6e, 0xf3, 0x72, 0xfe, 0x94, 0xf8, 0x2b,
	0xa5, 0x4f, 0xf5, 0x3a, 0x5f, 0x1d, 0x36, 0xf1,
	0x51, 0x0e, 0x52, 0x7f, 0xad, 0xe6, 0x82, 0xd1,
	0x9b, 0x05, 0x68, 0x8c, 0x2b, 0x3e, 0x6c, 0x1f,
	0x1f, 0x83, 0xd9, 0xab, 0xfb, 0x41, 0xbd, 0x6b,
	0x5b, 0xe0, 0xcd, 0x19, 0x13, 0x7e, 0x21, 0x79
};

/* Hash Algorithm specific structure */

static const struct qat_sym_hash_alg_info md5_info = {
	QAT_HASH_MD5_DIGEST_SIZE,
	QAT_HASH_MD5_BLOCK_SIZE,
	md5_initial_state,
	QAT_HASH_MD5_STATE_SIZE,
	&swcr_auth_hash_hmac_md5_96,
	offsetof(MD5_CTX, state),
	4,
};

static const struct qat_sym_hash_alg_info sha1_info = {
	QAT_HASH_SHA1_DIGEST_SIZE,
	QAT_HASH_SHA1_BLOCK_SIZE,
	sha1_initial_state,
	QAT_HASH_SHA1_STATE_SIZE,
	&swcr_auth_hash_hmac_sha1_96,
	offsetof(SHA1_CTX, state),
	4,
};

static const struct qat_sym_hash_alg_info sha256_info = {
	QAT_HASH_SHA256_DIGEST_SIZE,
	QAT_HASH_SHA256_BLOCK_SIZE,
	sha256_initial_state,
	QAT_HASH_SHA256_STATE_SIZE,
	&swcr_auth_hash_hmac_sha2_256,
	offsetof(SHA256_CTX, state),
	4,
};

static const struct qat_sym_hash_alg_info sha384_info = {
	QAT_HASH_SHA384_DIGEST_SIZE,
	QAT_HASH_SHA384_BLOCK_SIZE,
	sha384_initial_state,
	QAT_HASH_SHA384_STATE_SIZE,
	&swcr_auth_hash_hmac_sha2_384,
	offsetof(SHA384_CTX, state),
	8,
};

static const struct qat_sym_hash_alg_info sha512_info = {
	QAT_HASH_SHA512_DIGEST_SIZE,
	QAT_HASH_SHA512_BLOCK_SIZE,
	sha512_initial_state,
	QAT_HASH_SHA512_STATE_SIZE,
	&swcr_auth_hash_hmac_sha2_512,
	offsetof(SHA512_CTX, state),
	8,
};

static const struct qat_sym_hash_alg_info aes_gcm_info = {
	QAT_HASH_AES_GCM_DIGEST_SIZE,
	QAT_HASH_AES_GCM_BLOCK_SIZE,
	NULL, 0,
	NULL, 0, 0, /* XXX */
};

/* Hash QAT specific structures */

static const struct qat_sym_hash_qat_info md5_config = {
	HW_AUTH_ALGO_MD5,
	QAT_HASH_MD5_BLOCK_SIZE,
	HW_MD5_STATE1_SZ,
	HW_MD5_STATE2_SZ
};

static const struct qat_sym_hash_qat_info sha1_config = {
	HW_AUTH_ALGO_SHA1,
	QAT_HASH_SHA1_BLOCK_SIZE,
	HW_SHA1_STATE1_SZ,
	HW_SHA1_STATE2_SZ
};

static const struct qat_sym_hash_qat_info sha256_config = {
	HW_AUTH_ALGO_SHA256,
	QAT_HASH_SHA256_BLOCK_SIZE,
	HW_SHA256_STATE1_SZ,
	HW_SHA256_STATE2_SZ
};

static const struct qat_sym_hash_qat_info sha384_config = {
	HW_AUTH_ALGO_SHA384,
	QAT_HASH_SHA384_BLOCK_SIZE,
	HW_SHA384_STATE1_SZ,
	HW_SHA384_STATE2_SZ
};

static const struct qat_sym_hash_qat_info sha512_config = {
	HW_AUTH_ALGO_SHA512,
	QAT_HASH_SHA512_BLOCK_SIZE,
	HW_SHA512_STATE1_SZ,
	HW_SHA512_STATE2_SZ
};

static const struct qat_sym_hash_qat_info aes_gcm_config = {
	HW_AUTH_ALGO_GALOIS_128,
	0,
	HW_GALOIS_128_STATE1_SZ,
	HW_GALOIS_H_SZ +
	HW_GALOIS_LEN_A_SZ +
	HW_GALOIS_E_CTR0_SZ
};

static const struct qat_sym_hash_def qat_sym_hash_defs[] = {
	[QAT_SYM_HASH_MD5] = { &md5_info, &md5_config },
	[QAT_SYM_HASH_SHA1] = { &sha1_info, &sha1_config },
	[QAT_SYM_HASH_SHA256] = { &sha256_info, &sha256_config },
	[QAT_SYM_HASH_SHA384] = { &sha384_info, &sha384_config },
	[QAT_SYM_HASH_SHA512] = { &sha512_info, &sha512_config },
	[QAT_SYM_HASH_AES_GCM] = { &aes_gcm_info, &aes_gcm_config },
};

const struct qat_product *
		qat_lookup(const struct pci_attach_args *);
int		qat_match(struct device *, struct cfdata *, void *);
void		qat_attach(struct device *, struct device *, void *);
void		qat_init(struct device *);
int		qat_start(struct device *);
int		qat_detach(struct device *, int);

int		qat_alloc_msix_intr(struct qat_softc *,
		    struct pci_attach_args *);
void *		qat_establish_msix_intr(struct qat_softc *, pci_intr_handle_t,
			int (*)(void *), void *, const char *, int);
int		qat_setup_msix_intr(struct qat_softc *);

int		qat_etr_init(struct qat_softc *);
int		qat_etr_bank_init(struct qat_softc *, int);

int		qat_etr_ap_bank_init(struct qat_softc *);
void		qat_etr_ap_bank_set_ring_mask(uint32_t *, uint32_t, int);
void		qat_etr_ap_bank_set_ring_dest(struct qat_softc *, uint32_t *,
		    uint32_t, int);
void		qat_etr_ap_bank_setup_ring(struct qat_softc *,
		    struct qat_ring *);
int		qat_etr_verify_ring_size(uint32_t, uint32_t);

int		qat_etr_ring_intr(struct qat_softc *, struct qat_bank *,
		    struct qat_ring *);
int		qat_etr_bank_intr(void *);

void		qat_arb_update(struct qat_softc *, struct qat_bank *);

struct qat_sym_cookie *
		qat_crypto_alloc_sym_cookie(struct qat_crypto_bank *);
void		qat_crypto_free_sym_cookie(struct qat_crypto_bank *,
		    struct qat_sym_cookie *);
int		qat_crypto_load_buf(struct qat_softc *, struct cryptop *,
		    struct qat_sym_cookie *, struct qat_crypto_desc const *,
		    uint8_t *, int, bus_addr_t *);
int		qat_crypto_load_iv(struct qat_sym_cookie *, struct cryptop *,
		    struct cryptodesc *, struct qat_crypto_desc const *);
int		qat_crypto_process(void *, struct cryptop *, int);
int		qat_crypto_setup_ring(struct qat_softc *,
		    struct qat_crypto_bank *);
int		qat_crypto_new_session(void *, uint32_t *, struct cryptoini *);
void		qat_crypto_free_session0(struct qat_crypto *,
		    struct qat_session *);
void		qat_crypto_check_free_session(struct qat_crypto *,
		    struct qat_session *);
void		qat_crypto_free_session(void *, uint64_t);
int		qat_crypto_bank_init(struct qat_softc *,
		    struct qat_crypto_bank *);
int		qat_crypto_init(struct qat_softc *);
int		qat_crypto_start(struct qat_softc *);
int		qat_crypto_sym_rxintr(struct qat_softc *, void *, void *);

CFATTACH_DECL_NEW(qat, sizeof(struct qat_softc),
    qat_match, qat_attach, qat_detach, NULL);

struct qat_softc *gsc = NULL;

#ifdef QAT_DUMP
int qat_dump = QAT_DUMP;
#endif

const struct qat_product *
qat_lookup(const struct pci_attach_args *pa)
{
	const struct qat_product *qatp;

	for (qatp = qat_products; qatp->qatp_name != NULL; qatp++) {
		if (PCI_VENDOR(pa->pa_id) == qatp->qatp_vendor &&
		    PCI_PRODUCT(pa->pa_id) == qatp->qatp_product)
			return qatp;
	}
	return NULL;
}

int
qat_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (qat_lookup(pa) != NULL)
		return 1;

	return 0;
}

void
qat_attach(struct device *parent, struct device *self, void *aux)
{
	struct qat_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	const struct qat_product *qatp;
	char cap[256];
	pcireg_t cmd, memtype, msixoff, fusectl;
	bus_size_t msixtbl_offset;
	int i, bar, msixtbl_bar;

	sc->sc_dev = self;
	sc->sc_pc = pc;
	sc->sc_pcitag = pa->pa_tag;

	gsc = sc; /* for debug */

	qatp = qat_lookup(pa);
	KASSERT(qatp != NULL);

	if (pci_dma64_available(pa))
		sc->sc_dmat = pa->pa_dmat64;
	else
		sc->sc_dmat = pa->pa_dmat;

	aprint_naive(": Crypto processor\n");
	sc->sc_rev = PCI_REVISION(pa->pa_class);
	aprint_normal(": %s (rev. 0x%02x)\n", qatp->qatp_name, sc->sc_rev);

	memcpy(&sc->sc_hw, qatp->qatp_hw, sizeof(struct qat_hw));

	/* Determine active accelerators and engines */
	sc->sc_accel_mask = sc->sc_hw.qhw_get_accel_mask(sc);
	sc->sc_ae_mask = sc->sc_hw.qhw_get_ae_mask(sc);

	sc->sc_accel_num = 0;
	for (i = 0; i < sc->sc_hw.qhw_num_accel; i++) {
		if (sc->sc_accel_mask & (1 << i))
			sc->sc_accel_num++;
	}
	sc->sc_ae_num = 0;
	for (i = 0; i < sc->sc_hw.qhw_num_engines; i++) {
		if (sc->sc_ae_mask & (1 << i)) {
			sc->sc_ae_num++;
		}
	}

	if (!sc->sc_accel_mask || (sc->sc_ae_mask & 0x01) == 0) {
		aprint_error_dev(sc->sc_dev, "couldn't find acceleration");
		goto fail;
	}

	KASSERT(sc->sc_accel_num <= MAX_NUM_ACCEL);
	KASSERT(sc->sc_ae_num <= MAX_NUM_AE);

	/* Determine SKU and capabilities */
	sc->sc_sku = sc->sc_hw.qhw_get_sku(sc);
	sc->sc_accel_cap = sc->sc_hw.qhw_get_accel_cap(sc);
	sc->sc_fw_uof_name = sc->sc_hw.qhw_get_fw_uof_name(sc);

	aprint_normal_dev(sc->sc_dev,
	    "sku %d accel %d accel_mask 0x%x ae %d ae_mask 0x%x\n",
	    sc->sc_sku, sc->sc_accel_num, sc->sc_accel_mask,
	    sc->sc_ae_num, sc->sc_ae_mask);
	snprintb(cap, sizeof(cap), QAT_ACCEL_CAP_BITS, sc->sc_accel_cap);
	aprint_normal_dev(sc->sc_dev, "accel capabilities %s\n", cap);

	/* Map BARs */

	msixtbl_bar = 0;
	msixtbl_offset = 0;
	if (pci_get_capability(pc, pa->pa_tag, PCI_CAP_MSIX, &msixoff, NULL)) {
		pcireg_t msixtbl;
		msixtbl = pci_conf_read(pc, pa->pa_tag,
		    msixoff + PCI_MSIX_TBLOFFSET);
		msixtbl_offset = msixtbl & PCI_MSIX_TBLOFFSET_MASK;
		msixtbl_bar = PCI_MAPREG_START +
		    ((msixtbl & PCI_MSIX_TBLBIR_MASK) << 2);
	}

	i = 0;
	if (sc->sc_hw.qhw_sram_bar_id != NO_PCI_REG) {
		KASSERT(sc->sc_hw.qhw_sram_bar_id == 0);
		fusectl = pci_conf_read(sc->sc_pc, sc->sc_pcitag, FUSECTL_REG);
		/* Skip SRAM BAR */
		i = (fusectl & FUSECTL_MASK) ? 1 : 0;
	}
	for (bar = PCI_MAPREG_START; bar <= PCI_MAPREG_END; bar += 4) {
		bus_size_t size;
		bus_addr_t addr;

		if (pci_mapreg_probe(pc, pa->pa_tag, bar, &memtype) == 0)
			continue;

		if (PCI_MAPREG_TYPE(memtype) != PCI_MAPREG_TYPE_MEM)
			continue;

		/* MSI-X table will be mapped by pci_msix_alloc_map */
		if (bar == msixtbl_bar)
			size = msixtbl_offset;
		else
			size = 0;

		if (pci_mapreg_submap(pa, bar, memtype, 0, size, 0,
		    &sc->sc_csrt[i], &sc->sc_csrh[i], &addr, &sc->sc_csrs[i])) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't map bar 0x%02x\n", bar);
			goto fail;
		}

		aprint_verbose_dev(sc->sc_dev,
		    "region #%d bar 0x%02x size 0x%x at 0x%llx"
		    " mapped to %p\n", i, bar,
		    (int)sc->sc_csrs[i], (unsigned long long)addr,
		    bus_space_vaddr(sc->sc_csrt[i], sc->sc_csrh[i]));

		i++;
		if (PCI_MAPREG_MEM_TYPE(memtype) == PCI_MAPREG_MEM_TYPE_64BIT)
			bar += 4;
	}

	/* XXX Enable advanced error reporting */

	/* Enable bus mastering */
	cmd = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	cmd |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, cmd);

	if (qat_alloc_msix_intr(sc, pa))
		goto fail;

	config_mountroot(self, qat_init);

fail:
	/* XXX */
	return;
}

void
qat_init(struct device *self)
{
	int error;
	struct qat_softc *sc = device_private(self);

	aprint_verbose_dev(sc->sc_dev, "Initializing ETR\n");
	error = qat_etr_init(sc);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Could not initialize ETR: %d\n", error);
		return;
	}

	aprint_verbose_dev(sc->sc_dev, "Initializing admin comms\n");
	if (sc->sc_hw.qhw_init_admin_comms != NULL &&
	    (error = sc->sc_hw.qhw_init_admin_comms(sc)) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "Could not initialize admin comms: %d\n", error);
		return;
	}

	aprint_verbose_dev(sc->sc_dev, "Initializing hw arbiter\n");
	if (sc->sc_hw.qhw_init_arb != NULL &&
	    (error = sc->sc_hw.qhw_init_arb(sc)) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "Could not initialize hw arbiter: %d\n", error);
		return;
	}

	aprint_verbose_dev(sc->sc_dev, "Initializing acceleration engine\n");
	error = qat_ae_init(sc);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Could not initialize Acceleration Engine: %d\n", error);
		return;
	}

	aprint_verbose_dev(sc->sc_dev, "Loading acceleration engine firmware\n");
	error = qat_aefw_load(sc);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Could not load firmware: %d\n", error);
		return;
	}

	aprint_verbose_dev(sc->sc_dev, "Establishing interrupts\n");
	error = qat_setup_msix_intr(sc);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Could not setup interrupts: %d\n", error);
		return;
	}

	sc->sc_hw.qhw_enable_intr(sc);

	error = qat_crypto_init(sc);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Could not initialize service: %d\n", error);
		return;
	}

	aprint_verbose_dev(sc->sc_dev, "Enabling error correction\n");
	if (sc->sc_hw.qhw_enable_error_correction != NULL)
		sc->sc_hw.qhw_enable_error_correction(sc);

	aprint_verbose_dev(sc->sc_dev, "Initializing watchdog timer\n");
	if (sc->sc_hw.qhw_set_ssm_wdtimer != NULL &&
	    (error = sc->sc_hw.qhw_set_ssm_wdtimer(sc)) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "Could not initialize watchdog timer: %d\n", error);
		return;
	}

	error = qat_start(self);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Could not start: %d\n", error);
		return;
	}
}

int
qat_start(struct device *self)
{
	struct qat_softc *sc = device_private(self);
	int error;

	error = qat_ae_start(sc);
	if (error)
		return error;
	
	if (sc->sc_hw.qhw_send_admin_init != NULL &&
	    (error = sc->sc_hw.qhw_send_admin_init(sc)) != 0) {
		return error;
	}

	error = qat_crypto_start(sc);
	if (error)
		return error;

	return 0;
}

int
qat_detach(struct device *self, int flags)
{

	return 0;
}

void *
qat_alloc_mem(size_t size)
{
	size_t *sptr;
	sptr = kmem_zalloc(size + sizeof(size), KM_SLEEP);
	*sptr = size;
	return ++sptr;
}

void
qat_free_mem(void *ptr)
{
	size_t *sptr = ptr, size;
	size = *(--sptr);
	kmem_free(sptr, size + sizeof(size));
}

void
qat_free_dmamem(struct qat_softc *sc, struct qat_dmamem *qdm)
{

	bus_dmamap_unload(sc->sc_dmat, qdm->qdm_dma_map);
	bus_dmamap_destroy(sc->sc_dmat, qdm->qdm_dma_map);
	bus_dmamem_unmap(sc->sc_dmat, qdm->qdm_dma_vaddr, qdm->qdm_dma_size);
	bus_dmamem_free(sc->sc_dmat, &qdm->qdm_dma_seg, 1);
	explicit_memset(qdm, 0, sizeof(*qdm));
}

int
qat_alloc_dmamem(struct qat_softc *sc, struct qat_dmamem *qdm,
	bus_size_t size, bus_size_t alignment)
{
	int error = 0, nseg;

	error = bus_dmamem_alloc(sc->sc_dmat, size, alignment,
	    0, &qdm->qdm_dma_seg, 1, &nseg, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't allocate dmamem, error = %d\n", error);
		goto fail_0;
	}
	KASSERT(nseg == 1);
	error = bus_dmamem_map(sc->sc_dmat, &qdm->qdm_dma_seg,
	    nseg, size, &qdm->qdm_dma_vaddr,
	    BUS_DMA_COHERENT | BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't map dmamem, error = %d\n", error);
		goto fail_1;
	}
	qdm->qdm_dma_size = size;
	error = bus_dmamap_create(sc->sc_dmat, size, nseg, size,
	    0, BUS_DMA_NOWAIT, &qdm->qdm_dma_map);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't create dmamem map, error = %d\n", error);
		goto fail_2;
	}
	error = bus_dmamap_load(sc->sc_dmat, qdm->qdm_dma_map,
	    qdm->qdm_dma_vaddr, size, NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't load dmamem map, error = %d\n", error);
		goto fail_3;
	}

	return 0;
fail_3:
	bus_dmamap_destroy(sc->sc_dmat, qdm->qdm_dma_map);
	qdm->qdm_dma_map = NULL;
fail_2:
	bus_dmamem_unmap(sc->sc_dmat, qdm->qdm_dma_vaddr, size);
	qdm->qdm_dma_vaddr = NULL;
	qdm->qdm_dma_size = 0;
fail_1:
	bus_dmamem_free(sc->sc_dmat, &qdm->qdm_dma_seg, 1);
fail_0:
	return error;
}

int
qat_alloc_msix_intr(struct qat_softc *sc, struct pci_attach_args *pa)
{
	u_int *ih_map, vec;
	int error, count, ihi;

	count = sc->sc_hw.qhw_num_banks + 1;
	ih_map = qat_alloc_mem(sizeof(*ih_map) * count);
	ihi = 0;

	for (vec = 0; vec < sc->sc_hw.qhw_num_banks; vec++)
		ih_map[ihi++] = vec;

	vec += sc->sc_hw.qhw_msix_ae_vec_gap;
	ih_map[ihi++] = vec;

	error = pci_msix_alloc_map(pa, &sc->sc_ih, ih_map, count);
	qat_free_mem(ih_map);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't allocate msix %d: %d\n",
		    count, error);
	}

	return error;
}

void *
qat_establish_msix_intr(struct qat_softc *sc, pci_intr_handle_t ih,
	int (*func)(void *), void *arg,
	const char *name, int index)
{
	kcpuset_t *affinity;
	int error;
	char buf[PCI_INTRSTR_LEN];
	char intrxname[INTRDEVNAMEBUF];
	const char *intrstr;
	void *cookie;

	snprintf(intrxname, sizeof(intrxname), "%s%s%d",
	    device_xname(sc->sc_dev), name, index);

	intrstr = pci_intr_string(sc->sc_pc, ih, buf, sizeof(buf));

	pci_intr_setattr(sc->sc_pc, &ih, PCI_INTR_MPSAFE, true);

	cookie = pci_intr_establish_xname(sc->sc_pc, ih,
	    IPL_NET, func, arg, intrxname);

	aprint_normal_dev(sc->sc_dev, "%s%d interrupting at %s\n",
	    name, index, intrstr);

	kcpuset_create(&affinity, true);
	kcpuset_set(affinity, index % ncpu);
	error = interrupt_distribute(cookie, affinity, NULL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't distribute interrupt: %s%d\n", name, index);
	}
	kcpuset_destroy(affinity);

	return cookie;
}

int
qat_setup_msix_intr(struct qat_softc *sc)
{
	int i;
	pci_intr_handle_t ih;

	for (i = 0; i < sc->sc_hw.qhw_num_banks; i++) {
		struct qat_bank *qb = &sc->sc_etr_banks[i];
		ih = sc->sc_ih[i];

		qb->qb_ih_cookie = qat_establish_msix_intr(sc, ih,
		    qat_etr_bank_intr, qb, "bank", i);
		if (qb->qb_ih_cookie == NULL)
			return ENOMEM;
	}

	sc->sc_ae_ih_cookie = qat_establish_msix_intr(sc, sc->sc_ih[i],
	    qat_ae_cluster_intr, sc, "aeclust", 0);
	if (sc->sc_ae_ih_cookie == NULL)
		return ENOMEM;

	return 0;
}

int
qat_etr_init(struct qat_softc *sc)
{
	int i;
	int error = 0;

	sc->sc_etr_banks = qat_alloc_mem(
	    sizeof(struct qat_bank) * sc->sc_hw.qhw_num_banks);

	for (i = 0; i < sc->sc_hw.qhw_num_banks; i++) {
		error = qat_etr_bank_init(sc, i);
		if (error) {
			goto fail;
		}
	}

	if (sc->sc_hw.qhw_num_ap_banks) {
		sc->sc_etr_ap_banks = qat_alloc_mem(
		    sizeof(struct qat_ap_bank) * sc->sc_hw.qhw_num_ap_banks);
		error = qat_etr_ap_bank_init(sc);
		if (error) {
			goto fail;
		}
	}

	return 0;

fail:
	if (sc->sc_etr_banks != NULL) {
		qat_free_mem(sc->sc_etr_banks);
		sc->sc_etr_banks = NULL;
	}
	if (sc->sc_etr_ap_banks != NULL) {
		qat_free_mem(sc->sc_etr_ap_banks);
		sc->sc_etr_ap_banks = NULL;
	}
	return error;
}

int
qat_etr_bank_init(struct qat_softc *sc, int bank)
{
	struct qat_bank *qb = &sc->sc_etr_banks[bank];
	int i, tx_rx_gap = sc->sc_hw.qhw_tx_rx_gap;

	KASSERT(bank < sc->sc_hw.qhw_num_banks);

	mutex_init(&qb->qb_bank_mtx, MUTEX_DEFAULT, IPL_NET);

	qb->qb_sc = sc;
	qb->qb_bank = bank;
	qb->qb_coalescing_time = COALESCING_TIME_INTERVAL_DEFAULT;
	QAT_EVCNT_ATTACH(sc, &qb->qb_ev_rxintr, EVCNT_TYPE_INTR,
	    qb->qb_ev_rxintr_name, "bank%d rxintr", bank);

	/* Clean CSRs for all rings within the bank */
	for (i = 0; i < sc->sc_hw.qhw_num_rings_per_bank; i++) {
		struct qat_ring *qr = &qb->qb_et_rings[i];

		qat_etr_bank_ring_write_4(sc, bank, i,
		    ETR_RING_CONFIG, 0);
		qat_etr_bank_ring_base_write_8(sc, bank, i, 0);

		if (sc->sc_hw.qhw_tx_rings_mask & (1 << i)) {
			qr->qr_inflight = qat_alloc_mem(sizeof(uint32_t));
		} else if (sc->sc_hw.qhw_tx_rings_mask &
		    (1 << (i - tx_rx_gap))) {
			/* Share inflight counter with rx and tx */
			qr->qr_inflight =
			    qb->qb_et_rings[i - tx_rx_gap].qr_inflight;
		}
	}

	if (sc->sc_hw.qhw_init_etr_intr != NULL) {
		sc->sc_hw.qhw_init_etr_intr(sc, bank);
	} else {
		/* common code in qat 1.7 */
		qat_etr_bank_write_4(sc, bank, ETR_INT_REG,
		    ETR_INT_REG_CLEAR_MASK);
		for (i = 0; i < sc->sc_hw.qhw_num_rings_per_bank /
		    ETR_RINGS_PER_INT_SRCSEL; i++) {
			qat_etr_bank_write_4(sc, bank, ETR_INT_SRCSEL +
			    (i * ETR_INT_SRCSEL_NEXT_OFFSET),
			    ETR_INT_SRCSEL_MASK);
		}
	}

	return 0;
}

int
qat_etr_ap_bank_init(struct qat_softc *sc)
{
	int ap_bank;

	for (ap_bank = 0; ap_bank < sc->sc_hw.qhw_num_ap_banks; ap_bank++) {
		struct qat_ap_bank *qab = &sc->sc_etr_ap_banks[ap_bank];

		qat_etr_ap_bank_write_4(sc, ap_bank, ETR_AP_NF_MASK,
		    ETR_AP_NF_MASK_INIT);
		qat_etr_ap_bank_write_4(sc, ap_bank, ETR_AP_NF_DEST, 0);
		qat_etr_ap_bank_write_4(sc, ap_bank, ETR_AP_NE_MASK,
		    ETR_AP_NE_MASK_INIT);
		qat_etr_ap_bank_write_4(sc, ap_bank, ETR_AP_NE_DEST, 0);

		memset(qab, 0, sizeof(*qab));
	}

	return 0;
}

void
qat_etr_ap_bank_set_ring_mask(uint32_t *ap_mask, uint32_t ring, int set_mask)
{
	if (set_mask)
		*ap_mask |= (1 << ETR_RING_NUMBER_IN_AP_BANK(ring));
	else
		*ap_mask &= ~(1 << ETR_RING_NUMBER_IN_AP_BANK(ring));
}

void
qat_etr_ap_bank_set_ring_dest(struct qat_softc *sc, uint32_t *ap_dest,
	uint32_t ring, int set_dest)
{
	uint32_t ae_mask;
	uint8_t mailbox, ae, nae;
	uint8_t *dest = (uint8_t *)ap_dest;

	mailbox = ETR_RING_AP_MAILBOX_NUMBER(ring);

	nae = 0;
	ae_mask = sc->sc_ae_mask;
	for (ae = 0; ae < sc->sc_hw.qhw_num_engines; ae++) {
		if ((ae_mask & (1 << ae)) == 0)
			continue;

		if (set_dest) {
			dest[nae] = __SHIFTIN(ae, ETR_AP_DEST_AE) |
			    __SHIFTIN(mailbox, ETR_AP_DEST_MAILBOX) |
			    ETR_AP_DEST_ENABLE;
		} else {
			dest[nae] = 0;
		}
		nae++;
		if (nae == ETR_MAX_AE_PER_MAILBOX)
			break;

	}
}

void
qat_etr_ap_bank_setup_ring(struct qat_softc *sc, struct qat_ring *qr)
{
	struct qat_ap_bank *qab;
	int ap_bank;

	if (sc->sc_hw.qhw_num_ap_banks == 0)
		return;

	ap_bank = ETR_RING_AP_BANK_NUMBER(qr->qr_ring);
	KASSERT(ap_bank < sc->sc_hw.qhw_num_ap_banks);
	qab = &sc->sc_etr_ap_banks[ap_bank];

	if (qr->qr_cb == NULL) {
		qat_etr_ap_bank_set_ring_mask(&qab->qab_ne_mask, qr->qr_ring, 1);
		if (!qab->qab_ne_dest) {
			qat_etr_ap_bank_set_ring_dest(sc, &qab->qab_ne_dest,
			    qr->qr_ring, 1);
			qat_etr_ap_bank_write_4(sc, ap_bank, ETR_AP_NE_DEST,
			    qab->qab_ne_dest);
		}
	} else {
		qat_etr_ap_bank_set_ring_mask(&qab->qab_nf_mask, qr->qr_ring, 1);
		if (!qab->qab_nf_dest) {
			qat_etr_ap_bank_set_ring_dest(sc, &qab->qab_nf_dest,
			    qr->qr_ring, 1);
			qat_etr_ap_bank_write_4(sc, ap_bank, ETR_AP_NF_DEST,
			    qab->qab_nf_dest);
		}
	}
}

int
qat_etr_verify_ring_size(uint32_t msg_size, uint32_t num_msgs)
{
	int i = QAT_MIN_RING_SIZE;

	for (; i <= QAT_MAX_RING_SIZE; i++)
		if ((msg_size * num_msgs) == QAT_SIZE_TO_RING_SIZE_IN_BYTES(i))
			return i;

	return QAT_DEFAULT_RING_SIZE;
}

int
qat_etr_setup_ring(struct qat_softc *sc, int bank, uint32_t ring,
	uint32_t num_msgs, uint32_t msg_size, qat_cb_t cb, void *cb_arg,
	const char *name, struct qat_ring **rqr)
{
	struct qat_bank *qb;
	struct qat_ring *qr = NULL;
	int error;
	uint32_t ring_size_bytes, ring_config;
	uint64_t ring_base;
	uint32_t wm_nf = ETR_RING_CONFIG_NEAR_WM_512;
	uint32_t wm_ne = ETR_RING_CONFIG_NEAR_WM_0;

	KASSERT(bank < sc->sc_hw.qhw_num_banks);

	/* Allocate a ring from specified bank */
	qb = &sc->sc_etr_banks[bank];

	if (ring >= sc->sc_hw.qhw_num_rings_per_bank)
		return EINVAL;
	if (qb->qb_allocated_rings & (1 << ring))
		return ENOENT;
	qr = &qb->qb_et_rings[ring];
	qb->qb_allocated_rings |= 1 << ring;

	/* Initialize allocated ring */
	qr->qr_ring = ring;
	qr->qr_bank = bank;
	qr->qr_name = name;
	qr->qr_ring_id = qr->qr_bank * sc->sc_hw.qhw_num_rings_per_bank + ring;
	qr->qr_ring_mask = (1 << ring);
	qr->qr_cb = cb;
	qr->qr_cb_arg = cb_arg;
	QAT_EVCNT_ATTACH(sc, &qr->qr_ev_rxintr, EVCNT_TYPE_INTR,
	    qr->qr_ev_rxintr_name, "bank%d ring%d rxintr", bank, ring);
	QAT_EVCNT_ATTACH(sc, &qr->qr_ev_rxmsg, EVCNT_TYPE_MISC,
	    qr->qr_ev_rxmsg_name, "bank%d ring%d rxmsg", bank, ring);
	QAT_EVCNT_ATTACH(sc, &qr->qr_ev_txmsg, EVCNT_TYPE_MISC,
	    qr->qr_ev_txmsg_name, "bank%d ring%d txmsg", bank, ring);
	QAT_EVCNT_ATTACH(sc, &qr->qr_ev_txfull, EVCNT_TYPE_MISC,
	    qr->qr_ev_txfull_name, "bank%d ring%d txfull", bank, ring);

	/* Setup the shadow variables */
	qr->qr_head = 0;
	qr->qr_tail = 0;
	qr->qr_msg_size = QAT_BYTES_TO_MSG_SIZE(msg_size);
	qr->qr_ring_size = qat_etr_verify_ring_size(msg_size, num_msgs);

	/*
	 * To make sure that ring is aligned to ring size allocate
	 * at least 4k and then tell the user it is smaller.
	 */
	ring_size_bytes = QAT_SIZE_TO_RING_SIZE_IN_BYTES(qr->qr_ring_size);
	ring_size_bytes = QAT_RING_SIZE_BYTES_MIN(ring_size_bytes);
	error = qat_alloc_dmamem(sc, &qr->qr_dma,
	    ring_size_bytes, ring_size_bytes);
	if (error)
		return error;

	KASSERT(qr->qr_dma.qdm_dma_map->dm_nsegs == 1);

	qr->qr_ring_vaddr = qr->qr_dma.qdm_dma_vaddr;
	qr->qr_ring_paddr = qr->qr_dma.qdm_dma_map->dm_segs[0].ds_addr;

	aprint_verbose_dev(sc->sc_dev,
	    "allocate ring %d of bank %d for %s "
	    "size %d %d at vaddr %p paddr 0x%llx\n",
	    ring, bank, name, ring_size_bytes,
	    (int)qr->qr_dma.qdm_dma_map->dm_segs[0].ds_len,
	    qr->qr_ring_vaddr,
	    (unsigned long long)qr->qr_ring_paddr);

	memset(qr->qr_ring_vaddr, QAT_RING_PATTERN,
	    qr->qr_dma.qdm_dma_map->dm_segs[0].ds_len);

	bus_dmamap_sync(sc->sc_dmat, qr->qr_dma.qdm_dma_map, 0,
	    qr->qr_dma.qdm_dma_map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

	if (((uintptr_t)qr->qr_ring_paddr & (ring_size_bytes - 1)) != 0) {
		aprint_error_dev(sc->sc_dev, "ring address not aligned\n");
		return EFAULT;
	}

	if (cb == NULL) {
		ring_config = ETR_RING_CONFIG_BUILD(qr->qr_ring_size);
	} else {
		ring_config =
		    ETR_RING_CONFIG_BUILD_RESP(qr->qr_ring_size, wm_nf, wm_ne);
	}
	qat_etr_bank_ring_write_4(sc, bank, ring, ETR_RING_CONFIG, ring_config);

	ring_base = ETR_RING_BASE_BUILD(qr->qr_ring_paddr, qr->qr_ring_size);
	qat_etr_bank_ring_base_write_8(sc, bank, ring, ring_base);

	if (sc->sc_hw.qhw_init_arb != NULL)
		qat_arb_update(sc, qb);

	mutex_init(&qr->qr_ring_mtx, MUTEX_DEFAULT, IPL_NET);

	qat_etr_ap_bank_setup_ring(sc, qr);

	if (cb != NULL) {
		uint32_t intr_mask;

		qb->qb_intr_mask |= qr->qr_ring_mask;
		intr_mask = qb->qb_intr_mask;

		aprint_verbose_dev(sc->sc_dev,
		    "update intr mask for bank %d "
		    "(coalescing time %dns): 0x%08x\n",
		    bank, qb->qb_coalescing_time, intr_mask);
		qat_etr_bank_write_4(sc, bank, ETR_INT_COL_EN,
		    intr_mask);
		qat_etr_bank_write_4(sc, bank, ETR_INT_COL_CTL,
		    ETR_INT_COL_CTL_ENABLE | qb->qb_coalescing_time);
	}

	*rqr = qr;

	return 0;
}

static inline u_int
qat_modulo(u_int data, u_int shift)
{
	u_int div = data >> shift;
	u_int mult = div << shift;
	return data - mult;
}

int
qat_etr_put_msg(struct qat_softc *sc, struct qat_ring *qr, uint32_t *msg)
{
	uint32_t inflight;
	uint32_t *addr;

	mutex_spin_enter(&qr->qr_ring_mtx);

	inflight = atomic_inc_32_nv(qr->qr_inflight);
	if (inflight > QAT_MAX_INFLIGHTS(qr->qr_ring_size, qr->qr_msg_size)) {
		atomic_dec_32(qr->qr_inflight);
		QAT_EVCNT_INCR(&qr->qr_ev_txfull);
		mutex_spin_exit(&qr->qr_ring_mtx);
		return EBUSY;
	}
	QAT_EVCNT_INCR(&qr->qr_ev_txmsg);

	addr = (uint32_t *)((uintptr_t)qr->qr_ring_vaddr + qr->qr_tail);

	memcpy(addr, msg, QAT_MSG_SIZE_TO_BYTES(qr->qr_msg_size));
#ifdef QAT_DUMP
	qat_dump_raw(QAT_DUMP_RING_MSG, "put_msg", addr,
	    QAT_MSG_SIZE_TO_BYTES(qr->qr_msg_size));
#endif

	bus_dmamap_sync(sc->sc_dmat, qr->qr_dma.qdm_dma_map, qr->qr_tail,
	    QAT_MSG_SIZE_TO_BYTES(qr->qr_msg_size),
	    BUS_DMASYNC_PREWRITE);

	qr->qr_tail = qat_modulo(qr->qr_tail +
	    QAT_MSG_SIZE_TO_BYTES(qr->qr_msg_size),
	    QAT_RING_SIZE_MODULO(qr->qr_ring_size));

	qat_etr_bank_ring_write_4(sc, qr->qr_bank, qr->qr_ring,
	    ETR_RING_TAIL_OFFSET, qr->qr_tail);

	mutex_spin_exit(&qr->qr_ring_mtx);

	return 0;
}

int
qat_etr_ring_intr(struct qat_softc *sc, struct qat_bank *qb,
    struct qat_ring *qr)
{
	int handled = 0;
	uint32_t *msg;
	uint32_t nmsg = 0;

	mutex_spin_enter(&qr->qr_ring_mtx);

	QAT_EVCNT_INCR(&qr->qr_ev_rxintr);

	msg = (uint32_t *)((uintptr_t)qr->qr_ring_vaddr + qr->qr_head);

	bus_dmamap_sync(sc->sc_dmat, qr->qr_dma.qdm_dma_map, qr->qr_head,
	    QAT_MSG_SIZE_TO_BYTES(qr->qr_msg_size),
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	while (*msg != ETR_RING_EMPTY_ENTRY_SIG) {
		atomic_dec_32(qr->qr_inflight);
		QAT_EVCNT_INCR(&qr->qr_ev_rxmsg);

		if (qr->qr_cb != NULL) {
			mutex_spin_exit(&qr->qr_ring_mtx);
			handled |= qr->qr_cb(sc, qr->qr_cb_arg, msg);
			mutex_spin_enter(&qr->qr_ring_mtx);
		}

		*msg = ETR_RING_EMPTY_ENTRY_SIG;

		bus_dmamap_sync(sc->sc_dmat, qr->qr_dma.qdm_dma_map, qr->qr_head,
		    QAT_MSG_SIZE_TO_BYTES(qr->qr_msg_size),
		    BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		qr->qr_head = qat_modulo(qr->qr_head +
		    QAT_MSG_SIZE_TO_BYTES(qr->qr_msg_size),
		    QAT_RING_SIZE_MODULO(qr->qr_ring_size));
		nmsg++;

		msg = (uint32_t *)((uintptr_t)qr->qr_ring_vaddr + qr->qr_head);

		bus_dmamap_sync(sc->sc_dmat, qr->qr_dma.qdm_dma_map, qr->qr_head,
		    QAT_MSG_SIZE_TO_BYTES(qr->qr_msg_size),
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	}

	if (nmsg > 0) {
		qat_etr_bank_ring_write_4(sc, qr->qr_bank, qr->qr_ring,
		    ETR_RING_HEAD_OFFSET, qr->qr_head);
	}

	mutex_spin_exit(&qr->qr_ring_mtx);

	return handled;
}

int
qat_etr_bank_intr(void *arg)
{
	struct qat_bank *qb = arg;
	struct qat_softc *sc = qb->qb_sc;
	uint32_t estat;
	int i, handled = 0;

	mutex_spin_enter(&qb->qb_bank_mtx);

	QAT_EVCNT_INCR(&qb->qb_ev_rxintr);

	qat_etr_bank_write_4(sc, qb->qb_bank, ETR_INT_COL_CTL, 0);

	/* Now handle all the responses */
	estat = ~qat_etr_bank_read_4(sc, qb->qb_bank, ETR_E_STAT);
	estat &= qb->qb_intr_mask;

	qat_etr_bank_write_4(sc, qb->qb_bank, ETR_INT_COL_CTL,
	    ETR_INT_COL_CTL_ENABLE | qb->qb_coalescing_time);

	mutex_spin_exit(&qb->qb_bank_mtx);

	while ((i = ffs32(estat)) != 0) {
		struct qat_ring *qr = &qb->qb_et_rings[--i];
		estat &= ~(1 << i);
		handled |= qat_etr_ring_intr(sc, qb, qr);
	}

	return handled;
}

void
qat_arb_update(struct qat_softc *sc, struct qat_bank *qb)
{

	qat_arb_ringsrvarben_write_4(sc, qb->qb_bank,
	    qb->qb_allocated_rings & 0xff);
}

struct qat_sym_cookie *
qat_crypto_alloc_sym_cookie(struct qat_crypto_bank *qcb)
{
	struct qat_sym_cookie *qsc;

	mutex_spin_enter(&qcb->qcb_bank_mtx);

	if (qcb->qcb_symck_free_count == 0) {
		QAT_EVCNT_INCR(&qcb->qcb_ev_no_symck);
		mutex_spin_exit(&qcb->qcb_bank_mtx);
		return NULL;
	}

	qsc = qcb->qcb_symck_free[--qcb->qcb_symck_free_count];

	mutex_spin_exit(&qcb->qcb_bank_mtx);

	return qsc;
}

void
qat_crypto_free_sym_cookie(struct qat_crypto_bank *qcb, struct qat_sym_cookie *qsc)
{

	mutex_spin_enter(&qcb->qcb_bank_mtx);
	qcb->qcb_symck_free[qcb->qcb_symck_free_count++] = qsc;
	mutex_spin_exit(&qcb->qcb_bank_mtx);
}


void
qat_memcpy_htobe64(void *dst, const void *src, size_t len)
{
	uint64_t *dst0 = dst;
	const uint64_t *src0 = src;
	size_t i;

	KASSERT(len % sizeof(*dst0) == 0);

	for (i = 0; i < len / sizeof(*dst0); i++)
		*(dst0 + i) = htobe64(*(src0 + i));
}

void
qat_memcpy_htobe32(void *dst, const void *src, size_t len)
{
	uint32_t *dst0 = dst;
	const uint32_t *src0 = src;
	size_t i;

	KASSERT(len % sizeof(*dst0) == 0);

	for (i = 0; i < len / sizeof(*dst0); i++)
		*(dst0 + i) = htobe32(*(src0 + i));
}

void
qat_memcpy_htobe(void *dst, const void *src, size_t len, uint32_t wordbyte)
{
	switch (wordbyte) {
	case 4:
		qat_memcpy_htobe32(dst, src, len);
		break;
	case 8:
		qat_memcpy_htobe64(dst, src, len);
		break;
	default:
		KASSERT(0);
	}
}

void
qat_crypto_hmac_precompute(struct qat_crypto_desc *desc, struct cryptoini *cria,
    struct qat_sym_hash_def const *hash_def, uint8_t *state1, uint8_t *state2)
{
	int i, state_swap;
	struct swcr_auth_hash const *sah = hash_def->qshd_alg->qshai_sah;
	uint32_t blklen = hash_def->qshd_alg->qshai_block_len;
	uint32_t state_offset = hash_def->qshd_alg->qshai_state_offset;
	uint32_t state_size = hash_def->qshd_alg->qshai_state_size;
	uint32_t state_word = hash_def->qshd_alg->qshai_state_word;
	uint32_t keylen = cria->cri_klen / 8;
	uint32_t padlen = blklen - keylen;
	uint8_t *ipad = desc->qcd_hash_state_prefix_buf;
	uint8_t *opad = desc->qcd_hash_state_prefix_buf +
	    sizeof(desc->qcd_hash_state_prefix_buf) / 2;
	/* XXX
	 * For "stack protector not protecting local variables" error,
	 * use constant variable.
	 * Currently, the max length is sizeof(aesxcbc_ctx) used by
	 * swcr_auth_hash_aes_xcbc_mac
	 */
	uint8_t ctx[sizeof(aesxcbc_ctx)];

	memcpy(ipad, cria->cri_key, keylen);
	memcpy(opad, cria->cri_key, keylen);

	if (padlen > 0) {
		memset(ipad + keylen, 0, padlen);
		memset(opad + keylen, 0, padlen);
	}
	for (i = 0; i < blklen; i++) {
		ipad[i] ^= 0x36;
		opad[i] ^= 0x5c;
	}

	/* ipad */
	sah->Init(ctx);
	/* Check the endian of kernel built-in hash state */
	state_swap = memcmp(hash_def->qshd_alg->qshai_init_state,
	    ((uint8_t *)ctx) + state_offset, state_word);
	sah->Update(ctx, ipad, blklen);
	if (state_swap == 0) {
		memcpy(state1, ((uint8_t *)ctx) + state_offset, state_size);
	} else {
		qat_memcpy_htobe(state1, ((uint8_t *)ctx) + state_offset,
		    state_size, state_word);
	}

	/* opad */
	sah->Init(ctx);
	sah->Update(ctx, opad, blklen);
	if (state_swap == 0) {
		memcpy(state2, ((uint8_t *)ctx) + state_offset, state_size);
	} else {
		qat_memcpy_htobe(state2, ((uint8_t *)ctx) + state_offset,
		    state_size, state_word);
	}
}

uint16_t
qat_crypto_load_cipher_cryptoini(
    struct qat_crypto_desc *desc, struct cryptoini *crie)
{
	enum hw_cipher_algo algo = HW_CIPHER_ALGO_NULL;
	enum hw_cipher_mode mode = HW_CIPHER_CBC_MODE;
	enum hw_cipher_convert key_convert = HW_CIPHER_NO_CONVERT;

	switch (crie->cri_alg) {
	case CRYPTO_DES_CBC:
		algo = HW_CIPHER_ALGO_DES;
		desc->qcd_cipher_blk_sz = HW_DES_BLK_SZ;
		break;
	case CRYPTO_3DES_CBC:
		algo = HW_CIPHER_ALGO_3DES;
		desc->qcd_cipher_blk_sz = HW_3DES_BLK_SZ;
		break;
	case CRYPTO_AES_CBC:
		switch (crie->cri_klen / 8) {
		case HW_AES_128_KEY_SZ:
			algo = HW_CIPHER_ALGO_AES128;
			break;
		case HW_AES_192_KEY_SZ:
			algo = HW_CIPHER_ALGO_AES192;
			break;
		case HW_AES_256_KEY_SZ:
			algo = HW_CIPHER_ALGO_AES256;
			break;
		default:
			KASSERT(0);
			break;
		}
		desc->qcd_cipher_blk_sz = HW_AES_BLK_SZ;
		/*
		 * AES decrypt key needs to be reversed.
		 * Instead of reversing the key at session registration,
		 * it is instead reversed on-the-fly by setting the KEY_CONVERT
		 * bit here
		 */
		if (desc->qcd_cipher_dir == HW_CIPHER_DECRYPT)
			key_convert = HW_CIPHER_KEY_CONVERT;

		break;
	default:
		KASSERT(0);
		break;
	}

	return HW_CIPHER_CONFIG_BUILD(mode, algo, key_convert,
	    desc->qcd_cipher_dir);
}

uint16_t
qat_crypto_load_auth_cryptoini(
    struct qat_crypto_desc *desc, struct cryptoini *cria,
    struct qat_sym_hash_def const **hash_def)
{
	const struct swcr_auth_hash *sah;
	enum qat_sym_hash_algorithm algo = 0;

	switch (cria->cri_alg) {
	case CRYPTO_MD5_HMAC_96:
		algo = QAT_SYM_HASH_MD5;
		break;
	case CRYPTO_SHA1_HMAC_96:
		algo = QAT_SYM_HASH_SHA1;
		break;
	case CRYPTO_SHA2_256_HMAC:
		algo = QAT_SYM_HASH_SHA256;
		break;
	case CRYPTO_SHA2_384_HMAC:
		algo = QAT_SYM_HASH_SHA384;
		break;
	case CRYPTO_SHA2_512_HMAC:
		algo = QAT_SYM_HASH_SHA512;
		break;
	default:
		KASSERT(0);
		break;
	}
	*hash_def = &qat_sym_hash_defs[algo];
	sah = (*hash_def)->qshd_alg->qshai_sah;
	KASSERT(sah != NULL);
	desc->qcd_auth_sz = sah->auth_hash->authsize;

	return HW_AUTH_CONFIG_BUILD(HW_AUTH_MODE1,
	    (*hash_def)->qshd_qat->qshqi_algo_enc,
	    (*hash_def)->qshd_alg->qshai_digest_len);
}

int
qat_crypto_load_buf(struct qat_softc *sc, struct cryptop *crp,
    struct qat_sym_cookie *qsc, struct qat_crypto_desc const *desc,
    uint8_t *icv_buf, int icv_offset, bus_addr_t *icv_paddr)
{
	int error, i, nsegs;

	if (crp->crp_flags & CRYPTO_F_IMBUF) {
		struct mbuf *m = (struct mbuf *)crp->crp_buf;

		if (icv_offset >= 0) {
			if (m_length(m) == icv_offset) {
				m_copyback(m, icv_offset, desc->qcd_auth_sz,
				    icv_buf);
				if (m_length(m) == icv_offset)
					return ENOBUFS;
			} else {
				struct mbuf *m0;
				m0 = m_pulldown(m, icv_offset,
				    desc->qcd_auth_sz, NULL);
				if (m0 == NULL)
					return ENOBUFS;
			}
		}

		error = bus_dmamap_load_mbuf(sc->sc_dmat, qsc->qsc_buf_dmamap,
		    m, BUS_DMA_WRITE | BUS_DMA_NOWAIT);
		if (error == EFBIG) {
			struct mbuf *m_new;
			m_new = m_defrag(m, M_DONTWAIT);
			if (m_new != NULL) {
				crp->crp_buf = m_new;
				qsc->qsc_buf = m_new;
				error = bus_dmamap_load_mbuf(sc->sc_dmat,
				    qsc->qsc_buf_dmamap, m_new,
				    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
				if (error) {
					m_freem(m_new);
					crp->crp_buf = NULL;
				}
			}
		}

	} else if (crp->crp_flags & CRYPTO_F_IOV) {
		error = bus_dmamap_load_uio(sc->sc_dmat, qsc->qsc_buf_dmamap,
		    (struct uio *)crp->crp_buf, BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	} else {
		error = bus_dmamap_load(sc->sc_dmat, qsc->qsc_buf_dmamap,
		    crp->crp_buf, crp->crp_ilen, NULL, BUS_DMA_NOWAIT);
	}
	if (error) {
		aprint_debug_dev(sc->sc_dev,
		    "can't load crp_buf, error %d\n", error);
		crp->crp_etype = error;
		return error;
	}

	nsegs = qsc->qsc_buf_dmamap->dm_nsegs;
	qsc->qsc_buf_list.num_buffers = nsegs;
	for (i = 0; i < nsegs; i++) {
		struct flat_buffer_desc *flatbuf =
		    &qsc->qsc_buf_list.phy_buffers[i];
		bus_addr_t paddr = qsc->qsc_buf_dmamap->dm_segs[i].ds_addr;
		bus_size_t len = qsc->qsc_buf_dmamap->dm_segs[i].ds_len;

		flatbuf->data_len_in_bytes = len;
		flatbuf->phy_buffer = (uint64_t)paddr;

		if (icv_offset >= 0) {
			if (icv_offset < len)
				*icv_paddr = paddr + icv_offset;
			else
				icv_offset -= len;
		}
	}

	bus_dmamap_sync(sc->sc_dmat, qsc->qsc_buf_dmamap, 0,
	    qsc->qsc_buf_dmamap->dm_mapsize,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	return 0;
}

int
qat_crypto_load_iv(struct qat_sym_cookie *qsc, struct cryptop *crp,
    struct cryptodesc *crde, struct qat_crypto_desc const *desc)
{
	uint32_t ivlen = desc->qcd_cipher_blk_sz;

	if (crde->crd_flags & CRD_F_IV_EXPLICIT) {
		memcpy(qsc->qsc_iv_buf, crde->crd_iv, ivlen);
	} else {
		if (crde->crd_flags & CRD_F_ENCRYPT) {
			cprng_fast(qsc->qsc_iv_buf, ivlen);
		} else if (crp->crp_flags & CRYPTO_F_IMBUF) {
			/* get iv from buf */
			m_copydata(qsc->qsc_buf, crde->crd_inject, ivlen,
			    qsc->qsc_iv_buf);
		} else if (crp->crp_flags & CRYPTO_F_IOV) {
			cuio_copydata(qsc->qsc_buf, crde->crd_inject, ivlen,
			    qsc->qsc_iv_buf);
		}
	}

	if ((crde->crd_flags & CRD_F_ENCRYPT) != 0 &&
	    (crde->crd_flags & CRD_F_IV_PRESENT) == 0) {
		if (crp->crp_flags & CRYPTO_F_IMBUF) {
			m_copyback(qsc->qsc_buf, crde->crd_inject, ivlen,
			    qsc->qsc_iv_buf);
		} else if (crp->crp_flags & CRYPTO_F_IOV) {
			cuio_copyback(qsc->qsc_buf, crde->crd_inject, ivlen,
			    qsc->qsc_iv_buf);
		}
	}

	return 0;
}

static inline struct qat_crypto_bank *
qat_crypto_select_bank(struct qat_crypto *qcy)
{
	u_int cpuid = cpu_index(curcpu());

	return &qcy->qcy_banks[cpuid % qcy->qcy_num_banks];
}

int
qat_crypto_process(void *arg, struct cryptop *crp, int hint)
{
	struct qat_crypto *qcy = arg;
	struct qat_crypto_bank *qcb;
	struct qat_session *qs = NULL;
	struct qat_crypto_desc const *desc;
	struct qat_sym_cookie *qsc = NULL;
	struct qat_sym_bulk_cookie *qsbc;
	struct cryptodesc *crd, *crda = NULL, *crde = NULL;
	bus_addr_t icv_paddr = 0;
	int error, icv_offset = -1;
	uint8_t icv_buf[CRYPTO_MAX_MAC_LEN];

	qs = qcy->qcy_sessions[CRYPTO_SESID2LID(crp->crp_sid)];
	mutex_spin_enter(&qs->qs_session_mtx);
	KASSERT(qs->qs_status & QAT_SESSION_STATUS_ACTIVE);
	qs->qs_inflight++;
	mutex_spin_exit(&qs->qs_session_mtx);

	qcb = qat_crypto_select_bank(qcy);

	qsc = qat_crypto_alloc_sym_cookie(qcb);
	if (qsc == NULL) {
		error = ENOBUFS;
		goto fail;
	}

	error = 0;
	desc = &qs->qs_dec_desc;
	crd = crp->crp_desc;
	while (crd != NULL) {
		switch (crd->crd_alg) {
		case CRYPTO_DES_CBC:
		case CRYPTO_3DES_CBC:
		case CRYPTO_AES_CBC:
			if (crde != NULL)
				error = EINVAL;
			if (crd->crd_flags & CRD_F_ENCRYPT) {
				/* use encrypt desc */
				desc = &qs->qs_enc_desc;
				if (crda != NULL)
					error = ENOTSUP;
			}
			crde = crd;
			break;
		case CRYPTO_MD5_HMAC_96:
		case CRYPTO_SHA1_HMAC_96:
		case CRYPTO_SHA2_256_HMAC:
		case CRYPTO_SHA2_384_HMAC:
		case CRYPTO_SHA2_512_HMAC:
			if (crda != NULL)
				error = EINVAL;
			if (crde != NULL &&
			    (crde->crd_flags & CRD_F_ENCRYPT) == 0)
				error = EINVAL;
			crda = crd;
			icv_offset = crd->crd_inject;
			break;
		}
		if (error)
			goto fail;

		crd = crd->crd_next;
	}

	qsc->qsc_buf = crp->crp_buf;

	if (crde != NULL) {
		error = qat_crypto_load_iv(qsc, crp, crde, desc);
		if (error)
			goto fail;
	}

	error = qat_crypto_load_buf(qcy->qcy_sc, crp, qsc, desc, icv_buf,
	    icv_offset, &icv_paddr);
	if (error)
		goto fail;

	qsbc = &qsc->u.qsc_bulk_cookie;

	qsbc->qsbc_crypto = qcy;
	qsbc->qsbc_session = qs;
	qsbc->qsbc_cb_tag = crp;

	qcy->qcy_sc->sc_hw.qhw_crypto_setup_req_params(qcb, qs, desc, qsc,
	    crde, crda, icv_paddr);

	bus_dmamap_sync(qcy->qcy_sc->sc_dmat, *qsc->qsc_self_dmamap, 0,
	    offsetof(struct qat_sym_cookie, qsc_self_dmamap),
	    BUS_DMASYNC_PREWRITE);

	error = qat_etr_put_msg(qcy->qcy_sc, qcb->qcb_sym_tx,
	    (uint32_t *)qsbc->qsbc_msg);
	if (error)
		goto fail;

	return 0;
fail:
	if (qsc)
		qat_crypto_free_sym_cookie(qcb, qsc);
	mutex_spin_enter(&qs->qs_session_mtx);
	qs->qs_inflight--;
	qat_crypto_check_free_session(qcy, qs);
	crp->crp_etype = error;
	crypto_done(crp);
	return 0;
}

int
qat_crypto_setup_ring(struct qat_softc *sc, struct qat_crypto_bank *qcb)
{
	int error, i, bank;
	int curname = 0;
	char *name;

	bank = qcb->qcb_bank;

	name = qcb->qcb_ring_names[curname++];
	snprintf(name, QAT_RING_NAME_SIZE, "bank%d sym_tx", bank);
	error = qat_etr_setup_ring(sc, qcb->qcb_bank,
	    sc->sc_hw.qhw_ring_sym_tx, QAT_NSYMREQ, sc->sc_hw.qhw_fw_req_size,
	    NULL, NULL, name, &qcb->qcb_sym_tx);
	if (error)
		return error;

	name = qcb->qcb_ring_names[curname++];
	snprintf(name, QAT_RING_NAME_SIZE, "bank%d sym_rx", bank);
	error = qat_etr_setup_ring(sc, qcb->qcb_bank,
	    sc->sc_hw.qhw_ring_sym_rx, QAT_NSYMREQ, sc->sc_hw.qhw_fw_resp_size,
	    qat_crypto_sym_rxintr, qcb, name, &qcb->qcb_sym_rx);
	if (error)
		return error;

	for (i = 0; i < QAT_NSYMCOOKIE; i++) {
		struct qat_dmamem *qdm = &qcb->qcb_symck_dmamems[i];
		struct qat_sym_cookie *qsc;

		error = qat_alloc_dmamem(sc, qdm, sizeof(struct qat_sym_cookie),
		    QAT_OPTIMAL_ALIGN);
		if (error)
			return error;

		qsc = qdm->qdm_dma_vaddr;
		qsc->qsc_self_dmamap = &qdm->qdm_dma_map;
		qsc->qsc_bulk_req_params_buf_paddr =
		    qdm->qdm_dma_seg.ds_addr + offsetof(struct qat_sym_cookie,
		    u.qsc_bulk_cookie.qsbc_req_params_buf);
		qsc->qsc_buffer_list_desc_paddr =
		    qdm->qdm_dma_seg.ds_addr + offsetof(struct qat_sym_cookie,
		    qsc_buf_list);
		qsc->qsc_iv_buf_paddr =
		    qdm->qdm_dma_seg.ds_addr + offsetof(struct qat_sym_cookie,
		    qsc_iv_buf);
		qcb->qcb_symck_free[i] = qsc;
		qcb->qcb_symck_free_count++;

		error = bus_dmamap_create(sc->sc_dmat, QAT_MAXLEN,
		    QAT_MAXSEG, MCLBYTES, 0, 0, &qsc->qsc_buf_dmamap);
		if (error)
			return error;
	}

	return 0;
}

int
qat_crypto_bank_init(struct qat_softc *sc, struct qat_crypto_bank *qcb)
{
	int error;

	mutex_init(&qcb->qcb_bank_mtx, MUTEX_DEFAULT, IPL_NET);

	QAT_EVCNT_ATTACH(sc, &qcb->qcb_ev_no_symck, EVCNT_TYPE_MISC,
	    qcb->qcb_ev_no_symck_name, "crypto no_symck");

	error = qat_crypto_setup_ring(sc, qcb);
	if (error)
		return error;

	return 0;
}

int
qat_crypto_init(struct qat_softc *sc)
{
	struct qat_crypto *qcy = &sc->sc_crypto;
	int error, bank, i;
	int num_banks;

	qcy->qcy_sc = sc;

	if (sc->sc_hw.qhw_init_arb != NULL)
		num_banks = uimin(ncpu, sc->sc_hw.qhw_num_banks);
	else
		num_banks = sc->sc_ae_num;

	qcy->qcy_num_banks = num_banks;

	qcy->qcy_banks =
	    qat_alloc_mem(sizeof(struct qat_crypto_bank) * num_banks);

	for (bank = 0; bank < num_banks; bank++) {
		struct qat_crypto_bank *qcb = &qcy->qcy_banks[bank];
		qcb->qcb_bank = bank;
		qcb->qcb_crypto = qcy;
		error = qat_crypto_bank_init(sc, qcb);
		if (error)
			return error;
	}

	mutex_init(&qcy->qcy_crypto_mtx, MUTEX_DEFAULT, IPL_NET);

	for (i = 0; i < QAT_NSESSION; i++) {
		struct qat_dmamem *qdm = &qcy->qcy_session_dmamems[i];
		struct qat_session *qs;

		error = qat_alloc_dmamem(sc, qdm, sizeof(struct qat_session),
		    QAT_OPTIMAL_ALIGN);
		if (error)
			return error;

		qs = qdm->qdm_dma_vaddr;
		qs->qs_lid = i;
		qs->qs_dec_desc.qcd_desc_paddr = qdm->qdm_dma_seg.ds_addr;
		qs->qs_dec_desc.qcd_hash_state_paddr =
		    qs->qs_dec_desc.qcd_desc_paddr +
		    offsetof(struct qat_crypto_desc, qcd_hash_state_prefix_buf);
		qs->qs_enc_desc.qcd_desc_paddr = qdm->qdm_dma_seg.ds_addr +
		    offsetof(struct qat_session, qs_enc_desc);
		qs->qs_enc_desc.qcd_hash_state_paddr =
		    qs->qs_enc_desc.qcd_desc_paddr +
		    offsetof(struct qat_crypto_desc, qcd_hash_state_prefix_buf);

		mutex_init(&qs->qs_session_mtx, MUTEX_DEFAULT, IPL_NET);

		qcy->qcy_sessions[i] = qs;
		qcy->qcy_session_free[i] = qs;
		qcy->qcy_session_free_count++;
	}

	QAT_EVCNT_ATTACH(sc, &qcy->qcy_ev_new_sess, EVCNT_TYPE_MISC,
	    qcy->qcy_ev_new_sess_name, "crypto new_sess");
	QAT_EVCNT_ATTACH(sc, &qcy->qcy_ev_free_sess, EVCNT_TYPE_MISC,
	    qcy->qcy_ev_free_sess_name, "crypto free_sess");
	QAT_EVCNT_ATTACH(sc, &qcy->qcy_ev_no_sess, EVCNT_TYPE_MISC,
	    qcy->qcy_ev_no_sess_name, "crypto no_sess");

	return 0;
}

int
qat_crypto_new_session(void *arg, uint32_t *lid, struct cryptoini *cri)
{
	struct qat_crypto *qcy = arg;
	struct qat_session *qs = NULL;
	struct cryptoini *crie = NULL;
	struct cryptoini *cria = NULL;
	int slice, error;

	mutex_spin_enter(&qcy->qcy_crypto_mtx);

	if (qcy->qcy_session_free_count == 0) {
		QAT_EVCNT_INCR(&qcy->qcy_ev_no_sess);
		mutex_spin_exit(&qcy->qcy_crypto_mtx);
		return ENOBUFS;
	}
	qs = qcy->qcy_session_free[--qcy->qcy_session_free_count];
	QAT_EVCNT_INCR(&qcy->qcy_ev_new_sess);

	mutex_spin_exit(&qcy->qcy_crypto_mtx);

	qs->qs_status = QAT_SESSION_STATUS_ACTIVE;
	qs->qs_inflight = 0;
	*lid = qs->qs_lid;

	error = 0;
	while (cri) {
		switch (cri->cri_alg) {
		case CRYPTO_DES_CBC:
		case CRYPTO_3DES_CBC:
		case CRYPTO_AES_CBC:
			if (crie != NULL)
				error = EINVAL;
			crie = cri;
			break;
		case CRYPTO_MD5_HMAC_96:
		case CRYPTO_SHA1_HMAC_96:
		case CRYPTO_SHA2_256_HMAC:
		case CRYPTO_SHA2_384_HMAC:
		case CRYPTO_SHA2_512_HMAC:
			if (cria != NULL)
				error = EINVAL;
			cria = cri;
			break;
		default:
			error = EINVAL;
		}
		if (error)
			goto fail;
		cri = cri->cri_next;
	}

	slice = 1;
	if (crie != NULL && cria != NULL) {
		slice = 2;
		/* auth then decrypt */
		qs->qs_dec_desc.qcd_slices[0] = FW_SLICE_AUTH;
		qs->qs_dec_desc.qcd_slices[1] = FW_SLICE_CIPHER;
		qs->qs_dec_desc.qcd_cipher_dir = HW_CIPHER_DECRYPT;
		qs->qs_dec_desc.qcd_cmd_id = FW_LA_CMD_HASH_CIPHER;
		/* encrypt then auth */
		qs->qs_enc_desc.qcd_slices[0] = FW_SLICE_CIPHER;
		qs->qs_enc_desc.qcd_slices[1] = FW_SLICE_AUTH;
		qs->qs_enc_desc.qcd_cipher_dir = HW_CIPHER_ENCRYPT;
		qs->qs_enc_desc.qcd_cmd_id = FW_LA_CMD_CIPHER_HASH;
	} else if (crie != NULL) {
		/* decrypt */
		qs->qs_dec_desc.qcd_slices[0] = FW_SLICE_CIPHER;
		qs->qs_dec_desc.qcd_cipher_dir = HW_CIPHER_DECRYPT;
		qs->qs_dec_desc.qcd_cmd_id = FW_LA_CMD_CIPHER;
		/* encrypt */
		qs->qs_enc_desc.qcd_slices[0] = FW_SLICE_CIPHER;
		qs->qs_enc_desc.qcd_cipher_dir = HW_CIPHER_ENCRYPT;
		qs->qs_enc_desc.qcd_cmd_id = FW_LA_CMD_CIPHER;
	} else if (cria != NULL) {
		/* auth */
		qs->qs_dec_desc.qcd_slices[0] = FW_SLICE_AUTH;
		qs->qs_dec_desc.qcd_cmd_id = FW_LA_CMD_AUTH;
		/* auth */
		qs->qs_enc_desc.qcd_slices[0] = FW_SLICE_AUTH;
		qs->qs_enc_desc.qcd_cmd_id = FW_LA_CMD_AUTH;
	} else {
		error = EINVAL;
		goto fail;
	}
	qs->qs_dec_desc.qcd_slices[slice] = FW_SLICE_DRAM_WR;
	qs->qs_enc_desc.qcd_slices[slice] = FW_SLICE_DRAM_WR;

	qcy->qcy_sc->sc_hw.qhw_crypto_setup_desc(qcy, qs, &qs->qs_dec_desc, crie, cria);
	qcy->qcy_sc->sc_hw.qhw_crypto_setup_desc(qcy, qs, &qs->qs_enc_desc, crie, cria);

	return 0;
fail:
	if (qs != NULL) {
		mutex_spin_enter(&qs->qs_session_mtx);
		qat_crypto_free_session0(qcy, qs);
	}
	return error;
}

static inline void
qat_crypto_clean_desc(struct qat_crypto_desc *desc)
{
	explicit_memset(desc->qcd_content_desc, 0,
	    sizeof(desc->qcd_content_desc));
	explicit_memset(desc->qcd_hash_state_prefix_buf, 0,
	    sizeof(desc->qcd_hash_state_prefix_buf));
	explicit_memset(desc->qcd_req_cache, 0,
	    sizeof(desc->qcd_req_cache));
}

void
qat_crypto_free_session0(struct qat_crypto *qcy, struct qat_session *qs)
{

	qat_crypto_clean_desc(&qs->qs_dec_desc);
	qat_crypto_clean_desc(&qs->qs_enc_desc);
	qs->qs_status &= ~QAT_SESSION_STATUS_ACTIVE;

	mutex_spin_exit(&qs->qs_session_mtx);

	mutex_spin_enter(&qcy->qcy_crypto_mtx);

	qcy->qcy_session_free[qcy->qcy_session_free_count++] = qs;
	QAT_EVCNT_INCR(&qcy->qcy_ev_free_sess);

	mutex_spin_exit(&qcy->qcy_crypto_mtx);
}

void
qat_crypto_check_free_session(struct qat_crypto *qcy, struct qat_session *qs)
{

	if ((qs->qs_status & QAT_SESSION_STATUS_FREEING) &&
	    qs->qs_inflight == 0) {
		qat_crypto_free_session0(qcy, qs);
	} else {
		mutex_spin_exit(&qs->qs_session_mtx);
	}
}

void
qat_crypto_free_session(void *arg, uint64_t sid)
{
	struct qat_crypto *qcy = arg;
	struct qat_session *qs;

	qs = qcy->qcy_sessions[CRYPTO_SESID2LID(sid)];

	mutex_spin_enter(&qs->qs_session_mtx);

	if (qs->qs_inflight > 0) {
		qs->qs_status |= QAT_SESSION_STATUS_FREEING;
		mutex_spin_exit(&qs->qs_session_mtx);
		return;
	}

	qat_crypto_free_session0(qcy, qs);
}

int
qat_crypto_start(struct qat_softc *sc)
{
	struct qat_crypto *qcy = &sc->sc_crypto;
	int error, i;
	static const int algs[] = {
	    CRYPTO_DES_CBC, CRYPTO_3DES_CBC, CRYPTO_AES_CBC,
	    CRYPTO_MD5_HMAC_96, CRYPTO_SHA1_HMAC_96, CRYPTO_SHA2_256_HMAC,
	    CRYPTO_SHA2_384_HMAC, CRYPTO_SHA2_512_HMAC,
	};

	/* opencrypto */
	qcy->qcy_cid = crypto_get_driverid(0);
	if (qcy->qcy_cid < 0) {
		aprint_error_dev(sc->sc_dev,
		    "could not get opencrypto driver id\n");
		return ENOENT;
	}

	for (i = 0; i < __arraycount(algs); i++) {
		error = crypto_register(qcy->qcy_cid, algs[i], 0, 0,
		    qat_crypto_new_session, qat_crypto_free_session,
		    qat_crypto_process, qcy);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "could not register crypto: %d\n", error);
			return error;
		}
	}

	return 0;
}

int
qat_crypto_sym_rxintr(struct qat_softc *sc, void *arg, void *msg)
{
	struct qat_crypto_bank *qcb = arg;
	struct qat_crypto *qcy;
	struct qat_session *qs;
	struct qat_sym_cookie *qsc;
	struct qat_sym_bulk_cookie *qsbc;
	struct cryptop *crp;

	qsc = *(void **)((uintptr_t)msg + sc->sc_hw.qhw_crypto_opaque_offset);

	qsbc = &qsc->u.qsc_bulk_cookie;
	qcy = qsbc->qsbc_crypto;
	qs = qsbc->qsbc_session;
	crp = qsbc->qsbc_cb_tag;

	bus_dmamap_sync(sc->sc_dmat, qsc->qsc_buf_dmamap, 0,
	    qsc->qsc_buf_dmamap->dm_mapsize,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, qsc->qsc_buf_dmamap);
	qat_crypto_free_sym_cookie(qcb, qsc);

	crp->crp_etype = 0;
	crypto_done(crp);

	mutex_spin_enter(&qs->qs_session_mtx);
	KASSERT(qs->qs_status & QAT_SESSION_STATUS_ACTIVE);
	qs->qs_inflight--;
	qat_crypto_check_free_session(qcy, qs);

	return 1;
}

#ifdef QAT_DUMP

void
qat_dump_raw(int flag, const char *label, void *d, size_t len)
{
	uintptr_t pc;
	size_t pos;
	uint8_t *dp = (uint8_t *)d;

	if ((qat_dump & flag) == 0)
		return;

	printf("dumping %s at %p len %zu\n", label, d, len);

	pc = (uintptr_t)__builtin_return_address(0);
	printf("\tcallpc ");
	qat_print_sym(pc);
	printf("\n");

	for (pos = 0; pos < len; pos++) {
		if (pos % 32 == 0)
			printf("%8zx: ", pos);
		else if (pos % 4 == 0)
		        printf(" ");

		printf("%02x", dp[pos]);

		if (pos % 32 == 31 || pos + 1 == len)
			printf("\n");
	}
}

void
qat_dump_ring(int bank, int ring)
{
	struct qat_softc *sc = gsc;
	struct qat_bank *qb = &sc->sc_etr_banks[bank];
	struct qat_ring *qr = &qb->qb_et_rings[ring];
	u_int offset;
	int i;
	uint32_t msg;

	printf("dumping bank %d ring %d\n", bank, ring);
	printf("\tid %d name %s msg size %d ring size %d\n",
	    qr->qr_ring_id, qr->qr_name,
	    QAT_MSG_SIZE_TO_BYTES(qr->qr_msg_size),
	    qr->qr_ring_size);
	printf("\thost   head 0x%08x tail 0x%08x\n", qr->qr_head, qr->qr_tail);
	printf("\ttarget head 0x%08x tail 0x%08x\n",
	    qat_etr_bank_ring_read_4(sc, qr->qr_bank, qr->qr_ring,
	        ETR_RING_HEAD_OFFSET),
	    qat_etr_bank_ring_read_4(sc, qr->qr_bank, qr->qr_ring,
	        ETR_RING_TAIL_OFFSET));

	printf("\n");
	i = 0;
	offset = 0;
	do {
		if (i % 8 == 0)
			printf("%8x:", offset);

		if (offset == qr->qr_head) {
			printf("*");
		} else if (offset == qr->qr_tail) {
			printf("v");
		} else {
			printf(" ");
		}

		msg = *(uint32_t *)((uintptr_t)qr->qr_ring_vaddr + offset);
		printf("%08x", htobe32(msg));

		if (i % 8 == 7)
			printf("\n");

		i++;
		offset = qat_modulo(offset +
		    QAT_MSG_SIZE_TO_BYTES(qr->qr_msg_size),
		    QAT_RING_SIZE_MODULO(qr->qr_ring_size));
	} while (offset != 0);
}

void
qat_dump_mbuf(struct mbuf *m0, int pre, int post)
{
	struct mbuf *m;

	for (m = m0; m != NULL; m = m->m_next) {
		size_t pos, len;
		uint8_t *buf_start, *data_start, *data_end, *buf_end;
		uint8_t *start, *end, *dp;
		bool skip_ind;
		const char *ind;

		printf("dumping mbuf %p len %d flags 0x%08x\n",
		    m, m->m_len, m->m_flags);
		if (m->m_len == 0)
			continue;

		data_start = (uint8_t *)m->m_data;
		data_end = data_start + m->m_len;
		switch (m->m_flags & (M_EXT|M_EXT_CLUSTER|M_EXT_PAGES)) {
		case 0:
			buf_start = (uint8_t *)M_BUFADDR(m);
			buf_end = buf_start +
			    ((m->m_flags & M_PKTHDR) ? MHLEN : MLEN);
			break;
		case M_EXT|M_EXT_CLUSTER:
			buf_start = (uint8_t *)m->m_ext.ext_buf;
			buf_end = buf_start +m->m_ext.ext_size;
			break;
		default:
			/* XXX */
			buf_start = data_start;
			buf_end = data_end;
			break;
		}

		start = data_start - pre;
		if (start < buf_start)
			start = buf_start;
		end = data_end + post;
		if (end > buf_end)
			end = buf_end;

		dp = start;
		len = (size_t)(end - start);
		skip_ind = false;
		for (pos = 0; pos < len; pos++) {

			if (skip_ind)
				ind = "";
			else if (&dp[pos] == data_start)
				ind = "`";
			else
				ind = " ";

			if (pos % 32 == 0)
				printf("%8zx:%s", pos, ind);
			else if (pos % 2 == 0)
				printf("%s", ind);

			printf("%02x", dp[pos]);

			skip_ind = false;
			if (&dp[pos + 1] == data_end) {
				skip_ind = true;
				printf("'");
			}

			if (pos % 32 == 31 || pos + 1 == len) {
				printf("\n");
				skip_ind = false;
			}
		}
	}
}

#endif /* QAT_DUMP */

MODULE(MODULE_CLASS_DRIVER, qat, "pci,opencrypto");

#ifdef _MODULE
#include "ioconf.c"
#endif

int
qat_modcmd(modcmd_t cmd, void *data)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_qat,
		    cfattach_ioconf_qat, cfdata_ioconf_qat);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_qat,
		    cfattach_ioconf_qat, cfdata_ioconf_qat);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
