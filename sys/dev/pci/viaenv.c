/* $NetBSD: viaenv.c,v 1.1 2000/05/08 16:40:43 joda Exp $ */

/*
 * Copyright (c) 2000 Johan Danielsson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of author nor the names of any contributors may
 *    be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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

/* driver for the hardware monitoring part of the VIA VT82C686A */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/envsys.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/viapmvar.h>

#ifdef VIAENV_DEBUG
unsigned int viaenv_debug = 0;
#define DPRINTF(X) do { if(viaenv_debug) printf X ; } while(0)
#else
#define DPRINTF(X)
#endif

#define VIANUMSENSORS 10 /* three temp, two fan, five voltage */

struct viaenv_softc {
    struct device		sc_dev;
    bus_space_tag_t		sc_iot;
    bus_space_handle_t		sc_ioh;
    struct callout		sc_callout;
    unsigned int		sc_flags;

    int				sc_fan_div[2]; /* fan RPM divisor */

    struct envsys_tre_data	sc_data[VIANUMSENSORS];
    struct envsys_basic_info	sc_info[VIANUMSENSORS];

    struct proc *sc_thread;

    struct lock			sc_lock;
    struct simplelock		sc_interlock;
};

#define SCFLAG_OPEN 1

static int
viaenv_match(struct device *parent, struct cfdata *match, void *aux);
static void
viaenv_attach(struct device *parent, struct device *self, void *aux);

struct cfattach viaenv_ca = {
	sizeof(struct viaenv_softc), viaenv_match, viaenv_attach
};

static int
viaenv_match(struct device *parent, struct cfdata *match, void *aux)
{
    struct viapm_attach_args *va = aux;
    
    if (va->va_type == VIAPM_HWMON)
      return 1;
    return 0;
}

/* XXX there doesn't seem to exist much hard documentation on how to
   convert the raw values to usable units, this code is more or less
   stolen from the Linux driver, but changed to suit our conditions */

/* lookup-table to translate raw values to uK, this is the same table
used by the Linux driver (modulo units); there is a fifth degree
polynomial that supposedly been used to generate this table, but I
haven't been able to figure out how -- it doesn't give the same values */

static long val_to_temp[] = { 
    20225, 20435, 20645, 20855, 21045, 21245, 21425, 21615, 21785, 21955,
    22125, 22285, 22445, 22605, 22755, 22895, 23035, 23175, 23315, 23445,
    23565, 23695, 23815, 23925, 24045, 24155, 24265, 24365, 24465, 24565,
    24665, 24765, 24855, 24945, 25025, 25115, 25195, 25275, 25355, 25435,
    25515, 25585, 25655, 25725, 25795, 25865, 25925, 25995, 26055, 26115,
    26175, 26235, 26295, 26355, 26405, 26465, 26515, 26575, 26625, 26675,
    26725, 26775, 26825, 26875, 26925, 26975, 27025, 27065, 27115, 27165,
    27205, 27255, 27295, 27345, 27385, 27435, 27475, 27515, 27565, 27605,
    27645, 27685, 27735, 27775, 27815, 27855, 27905, 27945, 27985, 28025,
    28065, 28105, 28155, 28195, 28235, 28275, 28315, 28355, 28405, 28445,
    28485, 28525, 28565, 28615, 28655, 28695, 28735, 28775, 28825, 28865,
    28905, 28945, 28995, 29035, 29075, 29125, 29165, 29205, 29245, 29295,
    29335, 29375, 29425, 29465, 29505, 29555, 29595, 29635, 29685, 29725,
    29765, 29815, 29855, 29905, 29945, 29985, 30035, 30075, 30125, 30165,
    30215, 30255, 30305, 30345, 30385, 30435, 30475, 30525, 30565, 30615,
    30655, 30705, 30755, 30795, 30845, 30885, 30935, 30975, 31025, 31075,
    31115, 31165, 31215, 31265, 31305, 31355, 31405, 31455, 31505, 31545,
    31595, 31645, 31695, 31745, 31805, 31855, 31905, 31955, 32005, 32065,
    32115, 32175, 32225, 32285, 32335, 32395, 32455, 32515, 32575, 32635,
    32695, 32755, 32825, 32885, 32955, 33025, 33095, 33155, 33235, 33305,
    33375, 33455, 33525, 33605, 33685, 33765, 33855, 33935, 34025, 34115,
    34205, 34295, 34395, 34495, 34595, 34695, 34805, 34905, 35015, 35135,
    35245, 35365, 35495, 35615, 35745, 35875, 36015, 36145, 36295, 36435,
    36585, 36745, 36895, 37065, 37225, 37395, 37575, 37755, 37935, 38125,
    38325, 38525, 38725, 38935, 39155, 39375, 39605, 39835, 40075, 40325,
    40575, 40835, 41095, 41375, 41655, 41935,
};

/* use above table to convert values to temperatures in micro-Kelvins */
static int
val_to_uK(unsigned int val)
{
    int i = val / 4;
    int j = val % 4;
    assert(i >= 0 && i <= 255);
    if(j == 0 || i == 255)
	return val_to_temp[i] * 10000;
    /* is linear interpolation ok? */
    return (val_to_temp[i] * (4 - j) + 
	    val_to_temp[i+1] * j) * 2500 /* really: / 4 * 10000 */;
}

static int
val_to_rpm(unsigned int val, int div)
{
    if(val == 0)
	return 0;
    return 1350000 / val / div;
}

static long
val_to_uV(unsigned int val, int index)
{
    long mult[] = { 1250000, 1250000, 1670000, 2600000, 6300000 };
    assert(index >= 0 && index <= 4);
    return (25LL * val + 133) * mult[index] / 2628;
}

#define VIAENV_TSENS3	0x1f
#define VIAENV_TSENS1	0x20
#define VIAENV_TSENS2	0x21
#define VIAENV_VSENS1	0x22
#define VIAENV_VSENS2	0x23
#define VIAENV_VCORE	0x24
#define VIAENV_VSENS3	0x25
#define VIAENV_VSENS4	0x26
#define VIAENV_FAN1	0x29
#define VIAENV_FAN2	0x2a
#define VIAENV_FANCONF	0x47	/* fan configuration */
#define VIAENV_TLOW	0x49	/* temperature low order value */
#define VIAENV_TIRQ	0x4b	/* temperature interrupt configuration */


static void
viaenv_thread(void *arg)
{
    struct viaenv_softc *sc = arg;
    u_int8_t v, v2;
    int i;

    while(1) {
	lockmgr(&sc->sc_lock, LK_EXCLUSIVE, &sc->sc_interlock);
	/* temperature */
	v = bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAENV_TIRQ);
	v2 = bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAENV_TSENS1);
	DPRINTF(("TSENS1 = %d\n", (v2 << 2) | (v >> 6)));
	sc->sc_data[0].cur.data_us = val_to_uK((v2 << 2) | (v >> 6));
	sc->sc_data[0].validflags = ENVSYS_FVALID | ENVSYS_FCURVALID;

	v = bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAENV_TLOW);
	v2 = bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAENV_TSENS2);
	DPRINTF(("TSENS2 = %d\n", (v2 << 2) | ((v >> 4) & 0x3)));
	sc->sc_data[1].cur.data_us = val_to_uK((v2 << 2) | ((v >> 4) & 0x3));
	sc->sc_data[1].validflags = ENVSYS_FVALID | ENVSYS_FCURVALID;

	v2 = bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAENV_TSENS3);
	DPRINTF(("TSENS3 = %d\n", (v2 << 2) | (v >> 6)));
	sc->sc_data[2].cur.data_us = val_to_uK((v2 << 2) | (v >> 6));
	sc->sc_data[2].validflags = ENVSYS_FVALID | ENVSYS_FCURVALID;

	v = bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAENV_FANCONF);

	sc->sc_fan_div[0] = 1 << ((v >> 4) & 0x3);
	sc->sc_fan_div[1] = 1 << ((v >> 6) & 0x3);

	/* fan */
	for(i = 3; i <= 4; i++) {
	    v = bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAENV_FAN1 + i - 3);
	    DPRINTF(("FAN%d = %d / %d\n", i - 3, v, sc->sc_fan_div[i - 3]));
	    sc->sc_data[i].cur.data_us = val_to_rpm(v, sc->sc_fan_div[i - 3]);
	    sc->sc_data[i].validflags = ENVSYS_FVALID | ENVSYS_FCURVALID;
	}

	/* voltage */
	for(i = 5; i <= 9; i++) {
	    v = bus_space_read_1(sc->sc_iot, sc->sc_ioh, VIAENV_VSENS1 + i - 5);
	    DPRINTF(("V%d = %d\n", i - 5, v));
	    sc->sc_data[i].cur.data_us = val_to_uV(v, i - 5);
	    sc->sc_data[i].validflags = ENVSYS_FVALID | ENVSYS_FCURVALID;
	}

	lockmgr(&sc->sc_lock, LK_RELEASE, &sc->sc_interlock);
	tsleep(sc, PWAIT, "viaenv", 3 * hz / 2);
    }
}

static void
viaenv_thread_create(void *arg)
{
    struct viaenv_softc *sc = arg;
    if (kthread_create1(viaenv_thread, sc, &sc->sc_thread, "%s",
			sc->sc_dev.dv_xname)) {
	printf("%s: failed to create thread\n", sc->sc_dev.dv_xname);
    }
}

static void
viaenv_attach(struct device *parent, struct device *self, void *aux)
{
    struct viapm_attach_args *va = aux;
    struct viaenv_softc *sc = (struct viaenv_softc*)self;
    pcireg_t iobase, control;

    int i;

    iobase = pci_conf_read(va->va_pc, va->va_tag, va->va_offset);
    control = pci_conf_read(va->va_pc, va->va_tag, va->va_offset + 4);
    if((iobase & 0xff80) == 0 || (control & 1) == 0) {
	printf(": disabled\n");
	return;
    }

    sc->sc_iot = va->va_iot;
    if(bus_space_map(sc->sc_iot, iobase & 0xff80, 128, 0, &sc->sc_ioh)) {
	printf(": failed to map i/o\n");
	return;
    }

    printf("\n");

    lockinit(&sc->sc_lock, 0, "foo", 0, 0);
    simple_lock_init(&sc->sc_interlock);

    /* Initialize sensors */
    for (i = 0; i < VIANUMSENSORS; ++i) {
	sc->sc_data[i].sensor = sc->sc_info[i].sensor = i;
	sc->sc_data[i].validflags = (ENVSYS_FVALID|ENVSYS_FCURVALID);
	sc->sc_info[i].validflags = ENVSYS_FVALID;
	sc->sc_data[i].warnflags = ENVSYS_WARN_OK;
    }

    for (i = 0; i <= 2; i++) {
	sc->sc_data[i].units = sc->sc_info[i].units = ENVSYS_STEMP;
    }
    strcpy(sc->sc_info[0].desc, "TSENS1");
    strcpy(sc->sc_info[1].desc, "TSENS2");
    strcpy(sc->sc_info[2].desc, "TSENS3");

    for (i = 3; i <= 4; i++) {
	sc->sc_data[i].units = sc->sc_info[i].units = ENVSYS_SFANRPM;
    }
    strcpy(sc->sc_info[3].desc, "FAN1");
    strcpy(sc->sc_info[4].desc, "FAN2");

    for (i = 5; i <= 9; ++i) {
	sc->sc_data[i].units = sc->sc_info[i].units =
	    ENVSYS_SVOLTS_DC;
	sc->sc_info[i].rfact = 1; /* what is this used for? */
    }
    strcpy(sc->sc_info[5].desc, "VSENS1"); /* CPU core (2V) */
    strcpy(sc->sc_info[6].desc, "VSENS2"); /* NB core? (2.5V) */
    strcpy(sc->sc_info[7].desc, "Vcore"); /* Vcore (3.3V) */
    strcpy(sc->sc_info[8].desc, "VSENS3"); /* VSENS3 (5V) */
    strcpy(sc->sc_info[9].desc, "VSENS4"); /* VSENS4 (12V) */
  

    kthread_create(viaenv_thread_create, sc);
}


extern struct cfdriver viaenv_cd;

int viaenv_open(dev_t, int, int, struct proc*);
int viaenv_close(dev_t, int, int, struct proc*);
int viaenv_ioctl(dev_t, u_long, caddr_t, int, struct proc*);

int	
viaenv_open(dev_t dev, int flag, int mode, struct proc *p)
{
    int unit = minor(dev);
    struct viaenv_softc *sc;
	
    if (unit >= viaenv_cd.cd_ndevs)
	return ENXIO;
    sc = viaenv_cd.cd_devs[unit];
    if (sc == NULL)
	return ENXIO;
  
    if (sc->sc_flags & SCFLAG_OPEN)
	return EBUSY;

    sc->sc_flags |= SCFLAG_OPEN;

    return 0;
}


int
viaenv_close(dev_t dev, int flag, int mode, struct proc *p)
{
    struct viaenv_softc *sc = viaenv_cd.cd_devs[minor(dev)];

    sc->sc_flags &= ~SCFLAG_OPEN;

    return 0;
}

int
viaenv_ioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
    struct viaenv_softc *sc = viaenv_cd.cd_devs[minor(dev)];
    struct envsys_range *rng;
    struct envsys_tre_data *tred;
    struct envsys_basic_info *binfo;
    int32_t *vers;
    sc = sc;

    switch (cmd) {
    case ENVSYS_VERSION:
	vers = (int32_t *)data;
	*vers = 1000;

	return (0);

    case ENVSYS_GRANGE:
	rng = (struct envsys_range *)data;

	switch(rng->units) {
	case ENVSYS_STEMP:
	    rng->low = 0;
	    rng->high = 2;
	    break;
	case ENVSYS_SFANRPM:
	    rng->low = 3;
	    rng->high = 4;
	    break;
	case ENVSYS_SVOLTS_DC:
	    rng->low = 5;
	    rng->high = 11;
	    break;
	default:
	    rng->low = 1;
	    rng->high = 0;
	    break;
	}
	return 0;

    case ENVSYS_GTREDATA:
	tred = (struct envsys_tre_data *)data;
	tred->validflags = 0;
	if(tred->sensor < VIANUMSENSORS) {
	    lockmgr(&sc->sc_lock, LK_SHARED, &sc->sc_interlock);
	    memcpy(tred, &sc->sc_data[tred->sensor], sizeof(*tred));
	    lockmgr(&sc->sc_lock, LK_RELEASE, &sc->sc_interlock);
	}
    
	return 0;

    case ENVSYS_GTREINFO:
	binfo = (struct envsys_basic_info *)data;

	if (binfo->sensor >= VIANUMSENSORS)
	    binfo->validflags = 0;
	else
	    memcpy(binfo, &sc->sc_info[binfo->sensor], sizeof(*binfo));
    
	return 0;

#if 0
	/* not yet implemented */
    case ENVSYS_STREINFO:
	binfo = (struct envsys_basic_info *)data;
    
	binfo->validflags = 0;

	return 0;
#endif

    default:
	return ENOTTY;
    }
}
