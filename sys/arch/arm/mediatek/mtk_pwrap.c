/*-
* Copyright (c) 2017 Henry Chen <henryc.chen@mediatek.com>
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

#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timevar.h>

#include <arm/mediatek/mercury_reg.h>
#include <arm/mediatek/mtk_pwrap.h>

#include <dev/fdt/fdtvar.h>

static int mtk_pwrap_match(device_t, cfdata_t, void *);
static void mtk_pwrap_attach(device_t, device_t, void *);

#define read32(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define write32(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

int pwrap_read(void *sc, uint32_t adr, uint32_t *rdata);
int pwrap_write(void *sc, uint32_t adr, uint32_t wdata);

typedef uint32_t (*loop_condition_fp)(uint32_t);

static const char * const compatible[] = {
	"mediatek,mercury-pwrap",
	NULL
};

struct mtk_pwrap_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int phandle;
	bus_size_t sc_size;
	struct pwrap_controller sc_pwrap;
};

CFATTACH_DECL_NEW(mtk_pwrap, sizeof(struct mtk_pwrap_softc),
	mtk_pwrap_match, mtk_pwrap_attach, NULL, NULL);

static int
mtk_pwrap_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mtk_pwrap_attach(device_t parent, device_t self, void *aux)
{
	struct mtk_pwrap_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct pwrapbus_attach_args sba;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->phandle = faa->faa_phandle;
	sc->sc_size = size;
	sc->sc_bst = faa->faa_bst;
	sc->sc_dev = self;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	/* Initialize PWRAP controller */
	sc->sc_pwrap.sct_cookie = sc;
	sc->sc_pwrap.sct_read = pwrap_read;
	sc->sc_pwrap.sct_write = pwrap_write;

	sba.sba_controller = &sc->sc_pwrap;
	sba.sba_controller->phandle = sc->phandle;

	/* attach slave devices */
	(void)config_found_ia(sc->sc_dev, "pwrapbus", &sba, pwrapbus_print);
	aprint_normal("\npwrap attached done\n ");
}

static inline uint32_t
wait_for_fsm_vldclr(uint32_t x)
{
	return ((x >> RDATA_WACS_FSM_SHIFT) & RDATA_WACS_FSM_MASK) !=
		WACS_FSM_WFVLDCLR;
}

#if 0
static inline uint32_t
wait_for_sync(uint32_t x)
{
	return ((x >> RDATA_SYNC_IDLE_SHIFT) & RDATA_SYNC_IDLE_MASK) !=
		WACS_SYNC_IDLE;
}

static inline uint32_t
wait_for_idle_and_sync(uint32_t x)
{
	return ((((x >> RDATA_WACS_FSM_SHIFT) & RDATA_WACS_FSM_MASK) !=
		WACS_FSM_IDLE) || (((x >> RDATA_SYNC_IDLE_SHIFT) &
		RDATA_SYNC_IDLE_MASK)!= WACS_SYNC_IDLE));
}

static inline uint32_t
wait_for_cipher_ready(uint32_t x)
{
	return x != 3;
}
#endif

static int
_pwrap_timeout_ns(struct timeval *now, struct timeval *end)
{
	return timercmp(now, end, >);
}

static inline uint32_t
wait_for_state_idle(struct mtk_pwrap_softc *sc, uint32_t timeout_us,
		    uint32_t wacs_register, uint32_t wacs_vldclr_register,
		    uint32_t *read_reg)
{
	uint32_t reg_rdata;
	struct timeval start, gap, end;

	microtime(&start);
	gap.tv_sec = timeout_us;
	gap.tv_usec= timeout_us;
	timeradd(&start, &gap, &end);

	do {
		struct timeval now_tv;
		reg_rdata = read32(sc, wacs_register);
		/* if last read command timeout,clear vldclr bit
		   read command state machine:FSM_REQ-->wfdle-->WFVLDCLR;
		   write:FSM_REQ-->idle */
		switch (((reg_rdata >> RDATA_WACS_FSM_SHIFT) &
			RDATA_WACS_FSM_MASK)) {
		case WACS_FSM_WFVLDCLR:
			write32(sc, wacs_vldclr_register, 1);
			aprint_error("WACS_FSM = PMIC_WRAP_WACS_VLDCLR\n");
			break;
		case WACS_FSM_WFDLE:
			aprint_error("WACS_FSM = WACS_FSM_WFDLE\n");
			break;
		case WACS_FSM_REQ:
			aprint_error("WACS_FSM = WACS_FSM_REQ\n");
			break;
		default:
			break;
		}
		getmicrotime(&now_tv);
		if (_pwrap_timeout_ns(&now_tv, &end))
			return E_PWR_WAIT_IDLE_TIMEOUT;

	} while (((reg_rdata >> RDATA_WACS_FSM_SHIFT) & RDATA_WACS_FSM_MASK) !=
		 WACS_FSM_IDLE);        /* IDLE State */
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

static inline uint32_t
wait_for_state_ready(struct mtk_pwrap_softc *sc, loop_condition_fp fp,
		     uint32_t timeout_us, uint32_t wacs_register,
		     uint32_t *read_reg)
{
	uint32_t reg_rdata;
	struct timeval start, gap, end;

	microtime(&start);
	gap.tv_sec = timeout_us;
	gap.tv_usec= timeout_us;
	timeradd(&start, &gap, &end);

	do {
		struct timeval now_tv;
		reg_rdata = read32(sc, wacs_register);
		getmicrotime(&now_tv);
		if (_pwrap_timeout_ns(&now_tv, &end)) {
			aprint_error("timeout when waiting for idle\n");
			return E_PWR_WAIT_IDLE_TIMEOUT;
		}
	} while (fp(reg_rdata));        /* IDLE State */
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

static int
pwrap_wacs(struct mtk_pwrap_softc *sc, uint32_t write, uint32_t adr,
	   uint32_t wdata, uint32_t *rdata, uint32_t init_check)
{
	uint32_t reg_rdata = 0;
	uint32_t wacs_write = 0;
	uint32_t wacs_adr = 0;
	uint32_t wacs_cmd = 0;
	uint32_t return_value = 0;

	if (init_check) {
		reg_rdata = read32(sc, PWRAP_WACS2_RDATA);
		/* Prevent someone to used pwrap before pwrap init */
		if (((reg_rdata >> RDATA_INIT_DONE_SHIFT) &
		    RDATA_INIT_DONE_MASK) != WACS_INIT_DONE) {
			aprint_error("initialization isn't finished \n");
			return E_PWR_NOT_INIT_DONE;
		}
	}
	reg_rdata = 0;
	/* Check IDLE in advance */
	return_value = wait_for_state_idle(sc, TIMEOUT_WAIT_IDLE_US,
					   PWRAP_WACS2_RDATA,
					   PWRAP_WACS2_VLDCLR,
					   0);
	if (return_value != 0) {
		aprint_error("wait_for_fsm_idle fail,return_value=%d\n",
			  return_value);
		return E_PWR_WAIT_IDLE_TIMEOUT;
	}
	wacs_write = write << 31;
	wacs_adr = (adr >> 1) << 16;
	wacs_cmd = wacs_write | wacs_adr | wdata;

	write32(sc, PWRAP_WACS2_CMD, wacs_cmd);
	if (write == 0) {
		if (NULL == rdata) {
			aprint_error("rdata is a NULL pointer\n");
			return E_PWR_INVALID_ARG;
		}
		return_value = wait_for_state_ready(sc, wait_for_fsm_vldclr,
						    TIMEOUT_READ_US,
						    PWRAP_WACS2_RDATA,
						    &reg_rdata);
		if (return_value != 0) {
			aprint_error("wait vldclr fail,return_value=%d\n",
				  return_value);
			return E_PWR_WAIT_IDLE_TIMEOUT_READ;
		}
		*rdata = ((reg_rdata >> RDATA_WACS_RDATA_SHIFT)
			  & RDATA_WACS_RDATA_MASK);
		write32(sc, PWRAP_WACS2_VLDCLR, 1);
	}

	return 0;
}

int
pwrap_read(void *sc, uint32_t adr, uint32_t *rdata)
{
	return pwrap_wacs(sc, 0, adr, 0, rdata, 1);
}

int
pwrap_write(void *sc, uint32_t adr, uint32_t wdata)
{
	return pwrap_wacs(sc, 1, adr, wdata, 0, 1);
}
