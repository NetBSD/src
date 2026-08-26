/* $NetBSD$ */

/*-
 * Copyright (c) 2026 berte
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/stdint.h>
#include <sys/endian.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>
#include <sys/workqueue.h>
#include <sys/atomic.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/callout.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/bpf.h>

#include <netinet/in.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_ioctl.h>
#include <net80211/ieee80211_proto.h>
#include <net80211/ieee80211_node.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/led.h>

#include "if_uwe5622_sdiovar.h"


/*
 * ---- low-level sdio i/o ----
 */

uint32_t
uwe5622_sdio_build_sdio_config(void)
{
	uint32_t v;

	v = 0;
	v |= __BIT(0);

	return v;
}

int
uwe5622_sdio_write_cali_done(struct uwe5622_sdio_softc *sc)
{
	uint32_t sdio_cfg;
	int err;

	if (sc->sc_sync_cfg_written)
		return 0;

	sdio_cfg = uwe5622_sdio_build_sdio_config();

	aprint_normal_dev(sc->sc_dev,
	    "writing sdio_config=0x%08x to SYNC+0x%02x\n",
	    sdio_cfg, UWE_SYNC_SDIO_CONFIG_OFF);

	err = uwe5622_sdio_reg_write32(sc,
	    UWE_SYNC_ADDR_M3L + UWE_SYNC_SDIO_CONFIG_OFF, sdio_cfg);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "sdio_config write failed: %d\n", err);
		return err;
	}

	err = uwe5622_sdio_reg_write32(sc, UWE_SYNC_ADDR_M3L,
	    UWE_SYNC_CALI_WRITE_DONE);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "SYNC_CALI_WRITE_DONE write failed: %d\n", err);
		return err;
	}

	sc->sc_sync_cfg_written = true;
	return 0;
}

int
uwe5622_sdio_wait_for_sync(struct uwe5622_sdio_softc *sc, uint32_t addr,
    int tries)
{
	uint8_t buf[16];
	uint32_t st;
	int i, err;

	for (i = 0; i < tries; i++) {
		memset(buf, 0, sizeof(buf));
		err = uwe5622_sdio_dt_read(sc, addr, buf, sizeof(buf));
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "SYNC poll read failed: %d\n", err);
			return err;
		}

		st = ((uint32_t)buf[0] << 0) |
		     ((uint32_t)buf[1] << 8) |
		     ((uint32_t)buf[2] << 16) |
		     ((uint32_t)buf[3] << 24);

		UWE_DPRINTF(sc, "SYNC poll[%d] init_status=0x%08x\n", i, st);

		if (st == UWE_SYNC_CALI_WAITING) {
			err = uwe5622_sdio_write_cali_done(sc);
			if (err)
				return err;
		}

		if (st == UWE_SYNC_ALL_FINISHED) {
			aprint_normal_dev(sc->sc_dev,
			    "firmware sync completed\n");
			return 0;
		}

		delay(20000);
	}

	aprint_error_dev(sc->sc_dev,
	    "SYNC did not reach ALL_FINISHED\n");
	return ETIMEDOUT;
}

int
uwe5622_sdio_transport_init(struct uwe5622_sdio_softc *sc)
{
	int err;

	err = uwe5622_sdio_enable_func_intr(sc);
	if (err)
		return err;

	/*
	 * Host-side half of the same handshake: arms sunxi_mmc's real
	 * SUNXI_MMC_INT_SDIO_INT hardware IRQ path via the generic sdmmc(4)
	 * card-interrupt API (see uwe5622_sdio_intr()'s comment). If this
	 * platform's host controller doesn't implement card_enable_intr,
	 * sdmmc_intr_establish() just returns NULL - fall back to pure
	 * polling in that case rather than failing attach.
	 */
	sc->sc_sdio_ih = sdmmc_intr_establish(device_parent(sc->sc_dev),
	    uwe5622_sdio_intr, sc, device_xname(sc->sc_dev));
	if (sc->sc_sdio_ih != NULL)
		aprint_normal_dev(sc->sc_dev,
		    "SDIO card interrupt established (host IRQ-driven RX)\n");
	else
		aprint_normal_dev(sc->sc_dev,
		    "SDIO card interrupt unavailable, using poll-only RX\n");

	sc->sc_transport_ready = true;
	aprint_normal_dev(sc->sc_dev,
	    "transport initialized, control channel ready\n");

	return 0;
}

void
uwe5622_sdio_write_sysaddr(struct uwe5622_sdio_softc *sc, uint32_t addr)
{
	struct sdmmc_function *sf0;

	sf0 = sc->sc_sf->sc->sc_fn0;

	sdmmc_io_write_1(sf0, UWE_SDIO_FBR_SYSADDR0, (addr >> 0) & 0xff);
	sdmmc_io_write_1(sf0, UWE_SDIO_FBR_SYSADDR1, (addr >> 8) & 0xff);
	sdmmc_io_write_1(sf0, UWE_SDIO_FBR_SYSADDR2, (addr >> 16) & 0xff);
	sdmmc_io_write_1(sf0, UWE_SDIO_FBR_SYSADDR3, (addr >> 24) & 0xff);
}

uint32_t
uwe5622_sdio_read_dt_port_read4(struct uwe5622_sdio_softc *sc)
{
	return sdmmc_io_read_4(sc->sc_sf, UWE_SDIO_DT_MODE_ADDR);
}

uint32_t
uwe5622_sdio_read_dt_port_multi4(struct uwe5622_sdio_softc *sc)
{
	struct sdmmc_function *sf;
	uint8_t b[4];
	int err;

	sf = sc->sc_sf;
	b[0] = b[1] = b[2] = b[3] = 0;

	err = sdmmc_io_read_multi_1(sf, UWE_SDIO_DT_MODE_ADDR, b, 4);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "read_multi_1 DT port failed: %d\n", err);
		return 0;
	}

	return ((uint32_t)b[0] << 0) |
	       ((uint32_t)b[1] << 8) |
	       ((uint32_t)b[2] << 16) |
	       ((uint32_t)b[3] << 24);
}

uint32_t
uwe5622_sdio_readl_read4(struct uwe5622_sdio_softc *sc, uint32_t system_addr)
{
	uwe5622_sdio_write_sysaddr(sc, system_addr);
	return uwe5622_sdio_read_dt_port_read4(sc);
}

uint32_t
uwe5622_sdio_readl_multi(struct uwe5622_sdio_softc *sc, uint32_t system_addr)
{
	uwe5622_sdio_write_sysaddr(sc, system_addr);
	return uwe5622_sdio_read_dt_port_multi4(sc);
}

int
uwe5622_sdio_dt_read(struct uwe5622_sdio_softc *sc, uint32_t system_addr,
    void *buf, size_t len)
{
	size_t done, chunk;
	int err;
	uint8_t *p;

	done = 0;
	p = buf;

	uwe5622_sdio_write_sysaddr(sc, system_addr);

	while (done < len) {
		chunk = len - done;
		if (chunk > 256)
			chunk = 256;

		err = sdmmc_io_read_region_1(sc->sc_sf, UWE_SDIO_DT_MODE_ADDR,
		    p + done, chunk);
		if (err)
			return err;

		done += chunk;
	}

	return 0;
}

int
uwe5622_sdio_dt_write(struct uwe5622_sdio_softc *sc, uint32_t system_addr,
    const void *buf, size_t len)
{
	size_t done, chunk;
	int err;
	const uint8_t *p;

	done = 0;
	p = buf;

	uwe5622_sdio_write_sysaddr(sc, system_addr);

	while (done < len) {
		chunk = len - done;
		if (chunk > 256)
			chunk = 256;

		err = sdmmc_io_write_region_1(sc->sc_sf, UWE_SDIO_DT_MODE_ADDR,
		    __UNCONST(p + done), chunk);
		if (err)
			return err;

		done += chunk;
	}

	return 0;
}

int
uwe5622_sdio_reg_read32(struct uwe5622_sdio_softc *sc, uint32_t addr,
    uint32_t *val)
{
	uint8_t b[4];
	int err;

	err = uwe5622_sdio_dt_read(sc, addr, b, sizeof(b));
	if (err)
		return err;

	*val = ((uint32_t)b[0] << 0) |
	       ((uint32_t)b[1] << 8) |
	       ((uint32_t)b[2] << 16) |
	       ((uint32_t)b[3] << 24);

	return 0;
}

int
uwe5622_sdio_reg_write32(struct uwe5622_sdio_softc *sc, uint32_t addr,
    uint32_t val)
{
	uint8_t b[4];

	b[0] = (val >> 0) & 0xff;
	b[1] = (val >> 8) & 0xff;
	b[2] = (val >> 16) & 0xff;
	b[3] = (val >> 24) & 0xff;

	return uwe5622_sdio_dt_write(sc, addr, b, sizeof(b));
}

int
uwe5622_sdio_start_run(struct uwe5622_sdio_softc *sc)
{
	uint32_t v;
	int err;

	err = uwe5622_sdio_reg_read32(sc, UWE_CP_RESET_REG, &v);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "CP_RESET_REG read failed: %d\n", err);
		return err;
	}

	aprint_normal_dev(sc->sc_dev,
	    "CP_RESET_REG before start_run: 0x%08x\n", v);

	v &= ~UWE_RESET_BIT;

	err = uwe5622_sdio_reg_write32(sc, UWE_CP_RESET_REG, v);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "CP_RESET_REG write failed: %d\n", err);
		return err;
	}

	err = uwe5622_sdio_reg_read32(sc, UWE_CP_RESET_REG, &v);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "CP_RESET_REG re-read failed: %d\n", err);
		return err;
	}

	aprint_normal_dev(sc->sc_dev,
	    "CP_RESET_REG after start_run: 0x%08x\n", v);

	return 0;
}

void
uwe5622_sdio_dt_dump(struct uwe5622_sdio_softc *sc, uint32_t addr, size_t len,
    const char *tag)
{
	uint8_t buf[64];
	size_t i, n;
	uint32_t init_status;

	if (len > sizeof(buf))
		len = sizeof(buf);

	memset(buf, 0, sizeof(buf));

	if (uwe5622_sdio_dt_read(sc, addr, buf, len) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "%s dump read failed at 0x%08x\n", tag, addr);
		return;
	}

	for (i = 0; i < len; i += 16) {
		n = len - i;
		if (n > 16)
			n = 16;

		aprint_normal("%s: %s %08x:",
		    device_xname(sc->sc_dev), tag, (unsigned int)(addr + i));

		if (n > 0)  printf(" %02x", buf[i + 0]);
		if (n > 1)  printf(" %02x", buf[i + 1]);
		if (n > 2)  printf(" %02x", buf[i + 2]);
		if (n > 3)  printf(" %02x", buf[i + 3]);
		if (n > 4)  printf(" %02x", buf[i + 4]);
		if (n > 5)  printf(" %02x", buf[i + 5]);
		if (n > 6)  printf(" %02x", buf[i + 6]);
		if (n > 7)  printf(" %02x", buf[i + 7]);
		if (n > 8)  printf(" %02x", buf[i + 8]);
		if (n > 9)  printf(" %02x", buf[i + 9]);
		if (n > 10) printf(" %02x", buf[i + 10]);
		if (n > 11) printf(" %02x", buf[i + 11]);
		if (n > 12) printf(" %02x", buf[i + 12]);
		if (n > 13) printf(" %02x", buf[i + 13]);
		if (n > 14) printf(" %02x", buf[i + 14]);
		if (n > 15) printf(" %02x", buf[i + 15]);
		printf("\n");
	}

	if (len >= 4) {
		init_status =
		    ((uint32_t)buf[0] << 0) |
		    ((uint32_t)buf[1] << 8) |
		    ((uint32_t)buf[2] << 16) |
		    ((uint32_t)buf[3] << 24);

		aprint_normal_dev(sc->sc_dev,
		    "%s init_status=0x%08x\n", tag, init_status);
	}
}

/*
 * ---- firmware ----
 */

int
uwe5622_sdio_fw_write_chunked(struct uwe5622_sdio_softc *sc,
    uint32_t addr, const void *buf, size_t len)
{
	size_t offset, chunk;
	int err;
	const uint8_t *p;

	offset = 0;
	p = buf;

	while (offset < len) {
		chunk = len - offset;
		if (chunk > UWE_PACKET_SIZE)
			chunk = UWE_PACKET_SIZE;

		err = uwe5622_sdio_dt_write(sc, addr + offset, p + offset, chunk);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "firmware chunk write failed at 0x%08x len=%zu err=%d\n",
			    (unsigned int)(addr + offset), chunk, err);
			return err;
		}

		offset += chunk;
	}

	return 0;
}

const struct uwe_fw_imageinfo *
uwe5622_find_wcnm_image(const uint8_t *fw, size_t fwlen, const char *tag)
{
	const struct uwe_fw_head *h;
	const struct uwe_fw_imageinfo *img;
	uint32_t i, count;

	if (fwlen < sizeof(*h))
		return NULL;

	h = (const struct uwe_fw_head *)fw;
	if (memcmp(h->magic, "WCNM", 4) != 0)
		return NULL;

	count = h->img_count;
	if (fwlen < sizeof(*h) + count * sizeof(*img))
		return NULL;

	img = (const struct uwe_fw_imageinfo *)(fw + sizeof(*h));

	for (i = 0; i < count; i++) {
		if (memcmp(img[i].tag, tag, 4) == 0) {
			if ((size_t)img[i].offset + (size_t)img[i].size > fwlen)
				return NULL;
			return &img[i];
		}
	}

	return NULL;
}

int
uwe5622_fw_try_parse_wcnm_3lab(const uint8_t *fw, size_t fwlen,
    const uint8_t **payload, size_t *payload_len)
{
	const struct uwe_fw_imageinfo *img;

	img = uwe5622_find_wcnm_image(fw, fwlen, "3LAB");
	if (img == NULL)
		return ENOENT;

	*payload = fw + img->offset;
	*payload_len = img->size;
	return 0;
}

void
uwe5622_fw_probe_blob(struct uwe5622_sdio_softc *sc,
    const uint8_t *fw, size_t fwlen)
{
	const uint8_t *payload;
	size_t payload_len;
	int err;

	err = uwe5622_fw_try_parse_wcnm_3lab(fw, fwlen, &payload, &payload_len);
	if (err) {
		aprint_error_dev(sc->sc_dev, "WCNM 3LAB image not found\n");
		return;
	}

	aprint_normal_dev(sc->sc_dev,
	    "WCNM 3LAB payload found: offset=0x%zx len=%zu\n",
	    (size_t)(payload - fw), payload_len);

	err = uwe5622_sdio_fw_write_chunked(sc, UWE_CP_START_ADDR,
	    payload, payload_len);
	if (err) {
		aprint_error_dev(sc->sc_dev, "firmware write failed: %d\n", err);
		return;
	}

	aprint_normal_dev(sc->sc_dev,
	    "firmware write completed, starting CP...\n");

	err = uwe5622_sdio_start_run(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev, "start_run failed: %d\n", err);
		return;
	}

	err = uwe5622_sdio_wait_for_sync(sc, UWE_SYNC_ADDR_M3L, 50);
	if (err) {
		aprint_error_dev(sc->sc_dev, "SYNC wait failed: %d\n", err);
		return;
	}

	err = uwe5622_sdio_transport_init(sc);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "transport init failed: %d\n", err);
		return;
	}
}

enum uwe_chip_model
uwe5622_sdio_classify_chipid(uint32_t chipid)
{
	switch (chipid & UWE_WCN_CHIPID_MASK) {
	case UWE_MARLIN3_AA_CHIPID:
		return UWE_CHIP_MARLIN3;
	case UWE_MARLIN3L_AA_CHIPID:
		return UWE_CHIP_MARLIN3L;
	case UWE_MARLIN3E_AA_CHIPID:
		return UWE_CHIP_MARLIN3E;
	default:
		return UWE_CHIP_INVALID;
	}
}

const char *
uwe5622_sdio_chip_name(enum uwe_chip_model model)
{
	switch (model) {
	case UWE_CHIP_MARLIN3:
		return "Marlin3";
	case UWE_CHIP_MARLIN3L:
		return "Marlin3Lite";
	case UWE_CHIP_MARLIN3E:
		return "Marlin3E";
	default:
		return "Invalid";
	}
}
