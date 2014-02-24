/*	$NetBSD: lcd.c,v 1.1 2014/02/24 07:23:42 skrll Exp $	*/
/*	OpenBSD: lcd.c,v 1.2 2007/07/20 22:13:45 kettenis Exp 	*/

/*
 * Copyright (c) 2007 Mark Kettenis
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/callout.h>

#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/pdc.h>

#define LCD_CLS		0x01
#define LCD_HOME	0x02
#define LCD_LOCATE(X, Y)	(((Y) & 1 ? 0xc0 : 0x80) | ((X) & 0x0f))

struct lcd_softc {
	device_t		sc_dv;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t 	sc_cmdh, sc_datah;

	u_int			sc_delay;
	uint8_t			sc_heartbeat[3];

	struct callout		sc_to;
	int			sc_on;
	struct blink_lcd	sc_blink;
};

int	lcd_match(device_t, cfdata_t, void *);
void	lcd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lcd, sizeof(struct lcd_softc), lcd_match,
    lcd_attach, NULL, NULL);

void	lcd_write(struct lcd_softc *, const char *);
void	lcd_blink(void *, int);
void	lcd_blink_finish(void *);

int
lcd_match(device_t parent, cfdata_t match, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "lcd") == 0)
		return 1;

	return 0;
}

void
lcd_attach(device_t parent, device_t self, void *aux)
{
	struct lcd_softc *sc = device_private(self);
	struct confargs *ca = aux;
	struct pdc_chassis_lcd *pdc_lcd = &ca->ca_pcl;
	int i;

	sc->sc_dv = self;
	sc->sc_iot = ca->ca_iot;

	if (bus_space_map(sc->sc_iot, pdc_lcd->cmd_addr, 1, 0, &sc->sc_cmdh)) {
		aprint_error(": can't map cmd register\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, pdc_lcd->data_addr, 1, 0,
	    &sc->sc_datah)) {
		aprint_error(": can't map data register\n");
		bus_space_unmap(sc->sc_iot, sc->sc_cmdh, 1);
		return;
	}

	aprint_normal(": model %d\n", pdc_lcd->model);

	sc->sc_delay = pdc_lcd->delay;
	for (i = 0; i < 3; i++)
		sc->sc_heartbeat[i] = pdc_lcd->heartbeat[i];

	bus_space_write_1(sc->sc_iot, sc->sc_cmdh, 0, LCD_CLS);
	delay(100 * sc->sc_delay);

	bus_space_write_1(sc->sc_iot, sc->sc_cmdh, 0, LCD_LOCATE(0, 0));
	delay(sc->sc_delay);
	lcd_write(sc, "NetBSD/" MACHINE);

	callout_init(&sc->sc_to, 0);
	callout_setfunc(&sc->sc_to, lcd_blink_finish, sc);

	sc->sc_blink.bl_func = lcd_blink;
	sc->sc_blink.bl_arg = sc;
	blink_lcd_register(&sc->sc_blink);
}

void
lcd_write(struct lcd_softc *sc, const char *str)
{
	while (*str) {
		bus_space_write_1(sc->sc_iot, sc->sc_datah, 0, *str++);
		delay(sc->sc_delay);
	}
}

void
lcd_blink(void *v, int on)
{
	struct lcd_softc *sc = v;

	sc->sc_on = on;
	bus_space_write_1(sc->sc_iot, sc->sc_cmdh, 0, sc->sc_heartbeat[0]);
	callout_schedule(&sc->sc_to, max(1, (sc->sc_delay * hz) / 1000000));
}

void
lcd_blink_finish(void *v)
{
	struct lcd_softc *sc = v;
	uint8_t data;

	if (sc->sc_on)
		data = sc->sc_heartbeat[1];
	else
		data = sc->sc_heartbeat[2];

	bus_space_write_1(sc->sc_iot, sc->sc_datah, 0, data);
}
