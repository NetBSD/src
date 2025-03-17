/*	$NetBSD: umcpmio_subr.h,v 1.2 2025/03/17 18:24:08 riastradh Exp $	*/

/*
 * Copyright (c) 2024 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef	_UMCPMIO_SUBR_H_
#define	_UMCPMIO_SUBR_H_

#include <sys/types.h>

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_hid_reports.h>

struct umcpmio_mapping_put {
	const char	*tname;
	const uint8_t	mask;
};

int umcpmio_get_status(struct umcpmio_softc *, struct mcp2221_status_res *,
    bool);
void umcpmio_set_i2c_speed(struct mcp2221_status_req *, int);
int umcpmio_set_i2c_speed_one(struct umcpmio_softc *, int, bool);
int umcpmio_put_status(struct umcpmio_softc *, struct mcp2221_status_req *,
    struct mcp2221_status_res *, bool);
int umcpmio_get_sram(struct umcpmio_softc *, struct mcp2221_get_sram_res *,
    bool);
int umcpmio_put_sram(struct umcpmio_softc *, struct mcp2221_set_sram_req *,
    struct mcp2221_set_sram_res *, bool);
uint32_t umcpmio_sram_gpio_to_flags(uint8_t);
void umcpmio_set_gpio_value_sram(struct mcp2221_set_sram_req *, int, bool);
void umcpmio_set_gpio_dir_sram(struct mcp2221_set_sram_req *, int, int);
void umcpmio_set_gpio_designation_sram(struct mcp2221_set_sram_req *, int,
    int);
void umcpmio_set_gpio_irq_sram(struct mcp2221_set_sram_req *, int);
void umcpmio_set_dac_vref(struct mcp2221_set_sram_req *, char *);
int umcpmio_set_dac_vref_one(struct umcpmio_softc *, char *, bool);
void umcpmio_set_dac_value(struct mcp2221_set_sram_req *, uint8_t);
int umcpmio_set_dac_value_one(struct umcpmio_softc *, uint8_t, bool);
void umcpmio_set_adc_vref(struct mcp2221_set_sram_req *, char *);
int umcpmio_set_adc_vref_one(struct umcpmio_softc *, char *, bool);
void umcpmio_set_gpioclock_dc(struct mcp2221_set_sram_req *, char *);
int umcpmio_set_gpioclock_dc_one(struct umcpmio_softc *, char *, bool);
void umcpmio_set_gpioclock_cd(struct mcp2221_set_sram_req *, char *);
int umcpmio_set_gpioclock_cd_one(struct umcpmio_softc *, char *, bool);
int umcpmio_get_gpio_cfg(struct umcpmio_softc *,
    struct mcp2221_get_gpio_cfg_res *, bool);
int umcpmio_put_gpio_cfg(struct umcpmio_softc *,
    struct mcp2221_set_gpio_cfg_req *, struct mcp2221_set_gpio_cfg_res *,
    bool);
int umcpmio_get_gpio_value(struct umcpmio_softc *, int, bool);
void umcpmio_set_gpio_value(struct mcp2221_set_gpio_cfg_req *, int, bool);
int umcpmio_set_gpio_value_one(struct umcpmio_softc *, int, bool, bool);
int umcpmio_get_flash(struct umcpmio_softc *, uint8_t,
    struct mcp2221_get_flash_res *, bool);
int umcpmio_put_flash(struct umcpmio_softc *, struct mcp2221_put_flash_req *,
    struct mcp2221_put_flash_res *, bool);

#endif	/* _UMCPMIO_SUBR_H_ */
