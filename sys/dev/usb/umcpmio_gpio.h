/*	$NetBSD: umcpmio_gpio.h,v 1.1 2025/11/29 18:39:15 brad Exp $	*/

/*
 * Copyright (c) 2024, 2025 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _UMCPMIO_GPIO_H_
#define _UMCPMIO_GPIO_H_

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_info.h>

void mcp2221_set_dac_vref(struct mcp2221_set_sram_req *, char *);
int mcp2221_set_dac_vref_one(struct umcpmio_softc *, char *);
void mcp2221_set_dac_value(struct mcp2221_set_sram_req *, uint8_t);
int mcp2221_set_dac_value_one(struct umcpmio_softc *, uint8_t);
void mcp2221_set_adc_vref(struct mcp2221_set_sram_req *, char *);
int mcp2221_set_adc_vref_one(struct umcpmio_softc *, char *);
void mcp2221_set_gpioclock_dc(struct mcp2221_set_sram_req *, char *);
int mcp2221_set_gpioclock_dc_one(struct umcpmio_softc *, char *);
void mcp2221_set_gpioclock_cd(struct mcp2221_set_sram_req *, char *);
int mcp2221_set_gpioclock_cd_one(struct umcpmio_softc *, char *);
int mcp2221_get_gpio_cfg(struct umcpmio_softc *,
    struct mcp2221_get_gpio_cfg_res *);
int mcp2221_gpio_pin_ctl(void *, int, int);
void umcpmio_gpio_pin_ctl(void *, int, int);
int mcp2210_get_gp6_counter(struct umcpmio_softc *, int *, uint8_t);
void umcpmio_gpio_attach(struct umcpmio_softc *);

#endif
