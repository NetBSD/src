/*	$NetBSD: ipmi.c,v 1.53.2.3 2017/12/03 11:36:50 jdolecek Exp $ */

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Copyright (c) 2005 Jordan Hargrave
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipmi.c,v 1.53.2.3 2017/12/03 11:36:50 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/callout.h>
#include <sys/envsys.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <x86/smbiosvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <x86/ipmivar.h>

#include <uvm/uvm_extern.h>

struct ipmi_sensor {
	uint8_t	*i_sdr;
	int		i_num;
	int		i_stype;
	int		i_etype;
	char		i_envdesc[64];
	int 		i_envtype; /* envsys compatible type */
	int		i_envnum; /* envsys index */
	sysmon_envsys_lim_t i_limits, i_deflims;
	uint32_t	i_props, i_defprops;
	SLIST_ENTRY(ipmi_sensor) i_list;
	int32_t		i_prevval;	/* feed rnd source on change */
};

int	ipmi_nintr;
int	ipmi_dbg = 0;
int	ipmi_enabled = 0;

#define SENSOR_REFRESH_RATE (hz / 2)

#define SMBIOS_TYPE_IPMI	0x26

/*
 * Format of SMBIOS IPMI Flags
 *
 * bit0: interrupt trigger mode (1=level, 0=edge)
 * bit1: interrupt polarity (1=active high, 0=active low)
 * bit2: reserved
 * bit3: address LSB (1=odd,0=even)
 * bit4: interrupt (1=specified, 0=not specified)
 * bit5: reserved
 * bit6/7: register spacing (1,4,2,err)
 */
#define SMIPMI_FLAG_IRQLVL		(1L << 0)
#define SMIPMI_FLAG_IRQEN		(1L << 3)
#define SMIPMI_FLAG_ODDOFFSET		(1L << 4)
#define SMIPMI_FLAG_IFSPACING(x)	(((x)>>6)&0x3)
#define	 IPMI_IOSPACING_BYTE		 0
#define	 IPMI_IOSPACING_WORD		 2
#define	 IPMI_IOSPACING_DWORD		 1

#define IPMI_BTMSG_LEN			0
#define IPMI_BTMSG_NFLN			1
#define IPMI_BTMSG_SEQ			2
#define IPMI_BTMSG_CMD			3
#define IPMI_BTMSG_CCODE		4
#define IPMI_BTMSG_DATASND		4
#define IPMI_BTMSG_DATARCV		5

#define IPMI_MSG_NFLN			0
#define IPMI_MSG_CMD			1
#define IPMI_MSG_CCODE			2
#define IPMI_MSG_DATASND		2
#define IPMI_MSG_DATARCV		3

#define IPMI_SENSOR_TYPE_TEMP		0x0101
#define IPMI_SENSOR_TYPE_VOLT		0x0102
#define IPMI_SENSOR_TYPE_FAN		0x0104
#define IPMI_SENSOR_TYPE_INTRUSION	0x6F05
#define IPMI_SENSOR_TYPE_PWRSUPPLY	0x6F08

#define IPMI_NAME_UNICODE		0x00
#define IPMI_NAME_BCDPLUS		0x01
#define IPMI_NAME_ASCII6BIT		0x02
#define IPMI_NAME_ASCII8BIT		0x03

#define IPMI_ENTITY_PWRSUPPLY		0x0A

#define IPMI_SENSOR_SCANNING_ENABLED	(1L << 6)
#define IPMI_SENSOR_UNAVAILABLE		(1L << 5)
#define IPMI_INVALID_SENSOR_P(x) \
	(((x) & (IPMI_SENSOR_SCANNING_ENABLED|IPMI_SENSOR_UNAVAILABLE)) \
	!= IPMI_SENSOR_SCANNING_ENABLED)

#define IPMI_SDR_TYPEFULL		1
#define IPMI_SDR_TYPECOMPACT		2

#define byteof(x) ((x) >> 3)
#define bitof(x)  (1L << ((x) & 0x7))
#define TB(b,m)	  (data[2+byteof(b)] & bitof(b))

#define dbg_printf(lvl, fmt...) \
	if (ipmi_dbg >= lvl) \
		printf(fmt);
#define dbg_dump(lvl, msg, len, buf) \
	if (len && ipmi_dbg >= lvl) \
		dumpb(msg, len, (const uint8_t *)(buf));

long signextend(unsigned long, int);

SLIST_HEAD(ipmi_sensors_head, ipmi_sensor);
struct ipmi_sensors_head ipmi_sensor_list =
    SLIST_HEAD_INITIALIZER(&ipmi_sensor_list);

void	dumpb(const char *, int, const uint8_t *);

int	read_sensor(struct ipmi_softc *, struct ipmi_sensor *);
int	add_sdr_sensor(struct ipmi_softc *, uint8_t *);
int	get_sdr_partial(struct ipmi_softc *, uint16_t, uint16_t,
	    uint8_t, uint8_t, void *, uint16_t *);
int	get_sdr(struct ipmi_softc *, uint16_t, uint16_t *);

char	*ipmi_buf_acquire(struct ipmi_softc *, size_t);
void	ipmi_buf_release(struct ipmi_softc *, char *);
int	ipmi_sendcmd(struct ipmi_softc *, int, int, int, int, int, const void*);
int	ipmi_recvcmd(struct ipmi_softc *, int, int *, void *);
void	ipmi_delay(struct ipmi_softc *, int);

int	ipmi_watchdog_setmode(struct sysmon_wdog *);
int	ipmi_watchdog_tickle(struct sysmon_wdog *);
void	ipmi_dotickle(struct ipmi_softc *);

int	ipmi_intr(void *);
int	ipmi_match(device_t, cfdata_t, void *);
void	ipmi_attach(device_t, device_t, void *);
static int ipmi_detach(device_t, int);

long	ipmi_convert(uint8_t, struct sdrtype1 *, long);
void	ipmi_sensor_name(char *, int, uint8_t, uint8_t *);

/* BMC Helper Functions */
uint8_t bmc_read(struct ipmi_softc *, int);
void	bmc_write(struct ipmi_softc *, int, uint8_t);
int	bmc_io_wait(struct ipmi_softc *, int, uint8_t, uint8_t, const char *);
int	bmc_io_wait_spin(struct ipmi_softc *, int, uint8_t, uint8_t);
int	bmc_io_wait_sleep(struct ipmi_softc *, int, uint8_t, uint8_t);

void	*bt_buildmsg(struct ipmi_softc *, int, int, int, const void *, int *);
void	*cmn_buildmsg(struct ipmi_softc *, int, int, int, const void *, int *);

int	getbits(uint8_t *, int, int);
int	ipmi_sensor_type(int, int, int);

void	ipmi_smbios_probe(struct smbios_ipmi *, struct ipmi_attach_args *);
void	ipmi_refresh_sensors(struct ipmi_softc *);
int	ipmi_map_regs(struct ipmi_softc *, struct ipmi_attach_args *);
void	ipmi_unmap_regs(struct ipmi_softc *);

void	*scan_sig(long, long, int, int, const void *);

int32_t	ipmi_convert_sensor(uint8_t *, struct ipmi_sensor *);
void	ipmi_set_limits(struct sysmon_envsys *, envsys_data_t *,
			sysmon_envsys_lim_t *, uint32_t *);
void	ipmi_get_limits(struct sysmon_envsys *, envsys_data_t *,
			sysmon_envsys_lim_t *, uint32_t *);
void	ipmi_get_sensor_limits(struct ipmi_softc *, struct ipmi_sensor *,
			       sysmon_envsys_lim_t *, uint32_t *);
int	ipmi_sensor_status(struct ipmi_softc *, struct ipmi_sensor *,
			   envsys_data_t *, uint8_t *);

int	 add_child_sensors(struct ipmi_softc *, uint8_t *, int, int, int,
    int, int, int, const char *);

bool	ipmi_suspend(device_t, const pmf_qual_t *);

struct ipmi_if kcs_if = {
	"KCS",
	IPMI_IF_KCS_NREGS,
	cmn_buildmsg,
	kcs_sendmsg,
	kcs_recvmsg,
	kcs_reset,
	kcs_probe,
};

struct ipmi_if smic_if = {
	"SMIC",
	IPMI_IF_SMIC_NREGS,
	cmn_buildmsg,
	smic_sendmsg,
	smic_recvmsg,
	smic_reset,
	smic_probe,
};

struct ipmi_if bt_if = {
	"BT",
	IPMI_IF_BT_NREGS,
	bt_buildmsg,
	bt_sendmsg,
	bt_recvmsg,
	bt_reset,
	bt_probe,
};

struct ipmi_if *ipmi_get_if(int);

struct ipmi_if *
ipmi_get_if(int iftype)
{
	switch (iftype) {
	case IPMI_IF_KCS:
		return &kcs_if;
	case IPMI_IF_SMIC:
		return &smic_if;
	case IPMI_IF_BT:
		return &bt_if;
	default:
		return NULL;
	}
}

/*
 * BMC Helper Functions
 */
uint8_t
bmc_read(struct ipmi_softc *sc, int offset)
{
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh,
	    offset * sc->sc_if_iospacing);
}

void
bmc_write(struct ipmi_softc *sc, int offset, uint8_t val)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
	    offset * sc->sc_if_iospacing, val);
}

int
bmc_io_wait_sleep(struct ipmi_softc *sc, int offset, uint8_t mask,
    uint8_t value)
{
	int retries;
	uint8_t v;

	KASSERT(mutex_owned(&sc->sc_cmd_mtx));

	for (retries = 0; retries < sc->sc_max_retries; retries++) {
		v = bmc_read(sc, offset);
		if ((v & mask) == value)
			return v;
		mutex_enter(&sc->sc_sleep_mtx);
		cv_timedwait(&sc->sc_cmd_sleep, &sc->sc_sleep_mtx, 1);
		mutex_exit(&sc->sc_sleep_mtx);
	}
	return -1;
}

int
bmc_io_wait(struct ipmi_softc *sc, int offset, uint8_t mask, uint8_t value,
    const char *lbl)
{
	int v;

	v = bmc_io_wait_spin(sc, offset, mask, value);
	if (cold || v != -1)
		return v;

	return bmc_io_wait_sleep(sc, offset, mask, value);
}

int
bmc_io_wait_spin(struct ipmi_softc *sc, int offset, uint8_t mask,
    uint8_t value)
{
	uint8_t	v;
	int			count = cold ? 15000 : 500;
	/* ~us */

	while (count--) {
		v = bmc_read(sc, offset);
		if ((v & mask) == value)
			return v;

		delay(1);
	}

	return -1;

}

#define NETFN_LUN(nf,ln) (((nf) << 2) | ((ln) & 0x3))

/*
 * BT interface
 */
#define _BT_CTRL_REG			0
#define	  BT_CLR_WR_PTR			(1L << 0)
#define	  BT_CLR_RD_PTR			(1L << 1)
#define	  BT_HOST2BMC_ATN		(1L << 2)
#define	  BT_BMC2HOST_ATN		(1L << 3)
#define	  BT_EVT_ATN			(1L << 4)
#define	  BT_HOST_BUSY			(1L << 6)
#define	  BT_BMC_BUSY			(1L << 7)

#define	  BT_READY	(BT_HOST_BUSY|BT_HOST2BMC_ATN|BT_BMC2HOST_ATN)

#define _BT_DATAIN_REG			1
#define _BT_DATAOUT_REG			1

#define _BT_INTMASK_REG			2
#define	 BT_IM_HIRQ_PEND		(1L << 1)
#define	 BT_IM_SCI_EN			(1L << 2)
#define	 BT_IM_SMI_EN			(1L << 3)
#define	 BT_IM_NMI2SMI			(1L << 4)

int bt_read(struct ipmi_softc *, int);
int bt_write(struct ipmi_softc *, int, uint8_t);

int
bt_read(struct ipmi_softc *sc, int reg)
{
	return bmc_read(sc, reg);
}

int
bt_write(struct ipmi_softc *sc, int reg, uint8_t data)
{
	if (bmc_io_wait(sc, _BT_CTRL_REG, BT_BMC_BUSY, 0, __func__) < 0)
		return -1;

	bmc_write(sc, reg, data);
	return 0;
}

int
bt_sendmsg(struct ipmi_softc *sc, int len, const uint8_t *data)
{
	int i;

	bt_write(sc, _BT_CTRL_REG, BT_CLR_WR_PTR);
	for (i = 0; i < len; i++)
		bt_write(sc, _BT_DATAOUT_REG, data[i]);

	bt_write(sc, _BT_CTRL_REG, BT_HOST2BMC_ATN);
	if (bmc_io_wait(sc, _BT_CTRL_REG, BT_HOST2BMC_ATN | BT_BMC_BUSY, 0,
	    __func__) < 0)
		return -1;

	return 0;
}

int
bt_recvmsg(struct ipmi_softc *sc, int maxlen, int *rxlen, uint8_t *data)
{
	uint8_t len, v, i;

	if (bmc_io_wait(sc, _BT_CTRL_REG, BT_BMC2HOST_ATN, BT_BMC2HOST_ATN,
	    __func__) < 0)
		return -1;

	bt_write(sc, _BT_CTRL_REG, BT_HOST_BUSY);
	bt_write(sc, _BT_CTRL_REG, BT_BMC2HOST_ATN);
	bt_write(sc, _BT_CTRL_REG, BT_CLR_RD_PTR);
	len = bt_read(sc, _BT_DATAIN_REG);
	for (i = IPMI_BTMSG_NFLN; i <= len; i++) {
		v = bt_read(sc, _BT_DATAIN_REG);
		if (i != IPMI_BTMSG_SEQ)
			*(data++) = v;
	}
	bt_write(sc, _BT_CTRL_REG, BT_HOST_BUSY);
	*rxlen = len - 1;

	return 0;
}

int
bt_reset(struct ipmi_softc *sc)
{
	return -1;
}

int
bt_probe(struct ipmi_softc *sc)
{
	uint8_t rv;

	rv = bmc_read(sc, _BT_CTRL_REG);
	rv &= BT_HOST_BUSY;
	rv |= BT_CLR_WR_PTR|BT_CLR_RD_PTR|BT_BMC2HOST_ATN|BT_HOST2BMC_ATN;
	bmc_write(sc, _BT_CTRL_REG, rv);

	rv = bmc_read(sc, _BT_INTMASK_REG);
	rv &= BT_IM_SCI_EN|BT_IM_SMI_EN|BT_IM_NMI2SMI;
	rv |= BT_IM_HIRQ_PEND;
	bmc_write(sc, _BT_INTMASK_REG, rv);

#if 0
	printf("%s: %2x\n", __func__, v);
	printf(" WR    : %2x\n", v & BT_CLR_WR_PTR);
	printf(" RD    : %2x\n", v & BT_CLR_RD_PTR);
	printf(" H2B   : %2x\n", v & BT_HOST2BMC_ATN);
	printf(" B2H   : %2x\n", v & BT_BMC2HOST_ATN);
	printf(" EVT   : %2x\n", v & BT_EVT_ATN);
	printf(" HBSY  : %2x\n", v & BT_HOST_BUSY);
	printf(" BBSY  : %2x\n", v & BT_BMC_BUSY);
#endif
	return 0;
}

/*
 * SMIC interface
 */
#define _SMIC_DATAIN_REG		0
#define _SMIC_DATAOUT_REG		0

#define _SMIC_CTRL_REG			1
#define	  SMS_CC_GET_STATUS		 0x40
#define	  SMS_CC_START_TRANSFER		 0x41
#define	  SMS_CC_NEXT_TRANSFER		 0x42
#define	  SMS_CC_END_TRANSFER		 0x43
#define	  SMS_CC_START_RECEIVE		 0x44
#define	  SMS_CC_NEXT_RECEIVE		 0x45
#define	  SMS_CC_END_RECEIVE		 0x46
#define	  SMS_CC_TRANSFER_ABORT		 0x47

#define	  SMS_SC_READY			 0xc0
#define	  SMS_SC_WRITE_START		 0xc1
#define	  SMS_SC_WRITE_NEXT		 0xc2
#define	  SMS_SC_WRITE_END		 0xc3
#define	  SMS_SC_READ_START		 0xc4
#define	  SMS_SC_READ_NEXT		 0xc5
#define	  SMS_SC_READ_END		 0xc6

#define _SMIC_FLAG_REG			2
#define	  SMIC_BUSY			(1L << 0)
#define	  SMIC_SMS_ATN			(1L << 2)
#define	  SMIC_EVT_ATN			(1L << 3)
#define	  SMIC_SMI			(1L << 4)
#define	  SMIC_TX_DATA_RDY		(1L << 6)
#define	  SMIC_RX_DATA_RDY		(1L << 7)

int	smic_wait(struct ipmi_softc *, uint8_t, uint8_t, const char *);
int	smic_write_cmd_data(struct ipmi_softc *, uint8_t, const uint8_t *);
int	smic_read_data(struct ipmi_softc *, uint8_t *);

int
smic_wait(struct ipmi_softc *sc, uint8_t mask, uint8_t val, const char *lbl)
{
	int v;

	/* Wait for expected flag bits */
	v = bmc_io_wait(sc, _SMIC_FLAG_REG, mask, val, __func__);
	if (v < 0)
		return -1;

	/* Return current status */
	v = bmc_read(sc, _SMIC_CTRL_REG);
	dbg_printf(99, "%s(%s) = %#.2x\n", __func__, lbl, v);
	return v;
}

int
smic_write_cmd_data(struct ipmi_softc *sc, uint8_t cmd, const uint8_t *data)
{
	int	sts, v;

	dbg_printf(50, "%s: %#.2x %#.2x\n", __func__, cmd, data ? *data : -1);
	sts = smic_wait(sc, SMIC_TX_DATA_RDY | SMIC_BUSY, SMIC_TX_DATA_RDY,
	    "smic_write_cmd_data ready");
	if (sts < 0)
		return sts;

	bmc_write(sc, _SMIC_CTRL_REG, cmd);
	if (data)
		bmc_write(sc, _SMIC_DATAOUT_REG, *data);

	/* Toggle BUSY bit, then wait for busy bit to clear */
	v = bmc_read(sc, _SMIC_FLAG_REG);
	bmc_write(sc, _SMIC_FLAG_REG, v | SMIC_BUSY);

	return smic_wait(sc, SMIC_BUSY, 0, __func__);
}

int
smic_read_data(struct ipmi_softc *sc, uint8_t *data)
{
	int sts;

	sts = smic_wait(sc, SMIC_RX_DATA_RDY | SMIC_BUSY, SMIC_RX_DATA_RDY,
	    __func__);
	if (sts >= 0) {
		*data = bmc_read(sc, _SMIC_DATAIN_REG);
		dbg_printf(50, "%s: %#.2x\n", __func__, *data);
	}
	return sts;
}

#define ErrStat(a, ...) if (a) printf(__VA_ARGS__);

int
smic_sendmsg(struct ipmi_softc *sc, int len, const uint8_t *data)
{
	int sts, idx;

	sts = smic_write_cmd_data(sc, SMS_CC_START_TRANSFER, &data[0]);
	ErrStat(sts != SMS_SC_WRITE_START, "%s: wstart", __func__);
	for (idx = 1; idx < len - 1; idx++) {
		sts = smic_write_cmd_data(sc, SMS_CC_NEXT_TRANSFER,
		    &data[idx]);
		ErrStat(sts != SMS_SC_WRITE_NEXT, "%s: write", __func__);
	}
	sts = smic_write_cmd_data(sc, SMS_CC_END_TRANSFER, &data[idx]);
	if (sts != SMS_SC_WRITE_END) {
		dbg_printf(50, "%s: %d/%d = %#.2x\n", __func__, idx, len, sts);
		return -1;
	}

	return 0;
}

int
smic_recvmsg(struct ipmi_softc *sc, int maxlen, int *len, uint8_t *data)
{
	int sts, idx;

	*len = 0;
	sts = smic_wait(sc, SMIC_RX_DATA_RDY, SMIC_RX_DATA_RDY, __func__);
	if (sts < 0)
		return -1;

	sts = smic_write_cmd_data(sc, SMS_CC_START_RECEIVE, NULL);
	ErrStat(sts != SMS_SC_READ_START, "%s: rstart", __func__);
	for (idx = 0;; ) {
		sts = smic_read_data(sc, &data[idx++]);
		if (sts != SMS_SC_READ_START && sts != SMS_SC_READ_NEXT)
			break;
		smic_write_cmd_data(sc, SMS_CC_NEXT_RECEIVE, NULL);
	}
	ErrStat(sts != SMS_SC_READ_END, "%s: rend", __func__);

	*len = idx;

	sts = smic_write_cmd_data(sc, SMS_CC_END_RECEIVE, NULL);
	if (sts != SMS_SC_READY) {
		dbg_printf(50, "%s: %d/%d = %#.2x\n",
		    __func__, idx, maxlen, sts);
		return -1;
	}

	return 0;
}

int
smic_reset(struct ipmi_softc *sc)
{
	return -1;
}

int
smic_probe(struct ipmi_softc *sc)
{
	/* Flag register should not be 0xFF on a good system */
	if (bmc_read(sc, _SMIC_FLAG_REG) == 0xFF)
		return -1;

	return 0;
}

/*
 * KCS interface
 */
#define _KCS_DATAIN_REGISTER		0
#define _KCS_DATAOUT_REGISTER		0
#define	  KCS_READ_NEXT			0x68

#define _KCS_COMMAND_REGISTER		1
#define	  KCS_GET_STATUS		0x60
#define	  KCS_WRITE_START		0x61
#define	  KCS_WRITE_END			0x62

#define _KCS_STATUS_REGISTER		1
#define	  KCS_OBF			(1L << 0)
#define	  KCS_IBF			(1L << 1)
#define	  KCS_SMS_ATN			(1L << 2)
#define	  KCS_CD			(1L << 3)
#define	  KCS_OEM1			(1L << 4)
#define	  KCS_OEM2			(1L << 5)
#define	  KCS_STATE_MASK		0xc0
#define	    KCS_IDLE_STATE		0x00
#define	    KCS_READ_STATE		0x40
#define	    KCS_WRITE_STATE		0x80
#define	    KCS_ERROR_STATE		0xC0

int	kcs_wait(struct ipmi_softc *, uint8_t, uint8_t, const char *);
int	kcs_write_cmd(struct ipmi_softc *, uint8_t);
int	kcs_write_data(struct ipmi_softc *, uint8_t);
int	kcs_read_data(struct ipmi_softc *, uint8_t *);

int
kcs_wait(struct ipmi_softc *sc, uint8_t mask, uint8_t value, const char *lbl)
{
	int v;

	v = bmc_io_wait(sc, _KCS_STATUS_REGISTER, mask, value, lbl);
	if (v < 0)
		return v;

	/* Check if output buffer full, read dummy byte	 */
	if ((v & (KCS_OBF | KCS_STATE_MASK)) == (KCS_OBF | KCS_WRITE_STATE))
		bmc_read(sc, _KCS_DATAIN_REGISTER);

	/* Check for error state */
	if ((v & KCS_STATE_MASK) == KCS_ERROR_STATE) {
		bmc_write(sc, _KCS_COMMAND_REGISTER, KCS_GET_STATUS);
		while (bmc_read(sc, _KCS_STATUS_REGISTER) & KCS_IBF)
			;
		aprint_error_dev(sc->sc_dev, "error code: %#x\n",
		    bmc_read(sc, _KCS_DATAIN_REGISTER));
	}

	return v & KCS_STATE_MASK;
}

int
kcs_write_cmd(struct ipmi_softc *sc, uint8_t cmd)
{
	/* ASSERT: IBF and OBF are clear */
	dbg_printf(50, "%s: %#.2x\n", __func__, cmd);
	bmc_write(sc, _KCS_COMMAND_REGISTER, cmd);

	return kcs_wait(sc, KCS_IBF, 0, "write_cmd");
}

int
kcs_write_data(struct ipmi_softc *sc, uint8_t data)
{
	/* ASSERT: IBF and OBF are clear */
	dbg_printf(50, "%s: %#.2x\n", __func__, data);
	bmc_write(sc, _KCS_DATAOUT_REGISTER, data);

	return kcs_wait(sc, KCS_IBF, 0, "write_data");
}

int
kcs_read_data(struct ipmi_softc *sc, uint8_t * data)
{
	int sts;

	sts = kcs_wait(sc, KCS_IBF | KCS_OBF, KCS_OBF, __func__);
	if (sts != KCS_READ_STATE)
		return sts;

	/* ASSERT: OBF is set read data, request next byte */
	*data = bmc_read(sc, _KCS_DATAIN_REGISTER);
	bmc_write(sc, _KCS_DATAOUT_REGISTER, KCS_READ_NEXT);

	dbg_printf(50, "%s: %#.2x\n", __func__, *data);

	return sts;
}

/* Exported KCS functions */
int
kcs_sendmsg(struct ipmi_softc *sc, int len, const uint8_t * data)
{
	int idx, sts;

	/* ASSERT: IBF is clear */
	dbg_dump(50, __func__, len, data);
	sts = kcs_write_cmd(sc, KCS_WRITE_START);
	for (idx = 0; idx < len; idx++) {
		if (idx == len - 1)
			sts = kcs_write_cmd(sc, KCS_WRITE_END);

		if (sts != KCS_WRITE_STATE)
			break;

		sts = kcs_write_data(sc, data[idx]);
	}
	if (sts != KCS_READ_STATE) {
		dbg_printf(1, "%s: %d/%d <%#.2x>\n", __func__, idx, len, sts);
		dbg_dump(1, __func__, len, data);
		return -1;
	}

	return 0;
}

int
kcs_recvmsg(struct ipmi_softc *sc, int maxlen, int *rxlen, uint8_t * data)
{
	int idx, sts;

	for (idx = 0; idx < maxlen; idx++) {
		sts = kcs_read_data(sc, &data[idx]);
		if (sts != KCS_READ_STATE)
			break;
	}
	sts = kcs_wait(sc, KCS_IBF, 0, __func__);
	*rxlen = idx;
	if (sts != KCS_IDLE_STATE) {
		dbg_printf(1, "%s: %d/%d <%#.2x>\n",
		    __func__, idx, maxlen, sts);
		return -1;
	}

	dbg_dump(50, __func__, idx, data);

	return 0;
}

int
kcs_reset(struct ipmi_softc *sc)
{
	return -1;
}

int
kcs_probe(struct ipmi_softc *sc)
{
	uint8_t v;

	v = bmc_read(sc, _KCS_STATUS_REGISTER);
#if 0
	printf("%s: %2x\n", __func__, v);
	printf(" STS: %2x\n", v & KCS_STATE_MASK);
	printf(" ATN: %2x\n", v & KCS_SMS_ATN);
	printf(" C/D: %2x\n", v & KCS_CD);
	printf(" IBF: %2x\n", v & KCS_IBF);
	printf(" OBF: %2x\n", v & KCS_OBF);
#else
	__USE(v);
#endif
	return 0;
}

/*
 * IPMI code
 */
#define READ_SMS_BUFFER		0x37
#define WRITE_I2C		0x50

#define GET_MESSAGE_CMD		0x33
#define SEND_MESSAGE_CMD	0x34

#define IPMB_CHANNEL_NUMBER	0

#define PUBLIC_BUS		0

#define MIN_I2C_PACKET_SIZE	3
#define MIN_IMB_PACKET_SIZE	7	/* one byte for cksum */

#define MIN_BTBMC_REQ_SIZE	4
#define MIN_BTBMC_RSP_SIZE	5
#define MIN_BMC_REQ_SIZE	2
#define MIN_BMC_RSP_SIZE	3

#define BMC_SA			0x20	/* BMC/ESM3 */
#define FPC_SA			0x22	/* front panel */
#define BP_SA			0xC0	/* Primary Backplane */
#define BP2_SA			0xC2	/* Secondary Backplane */
#define PBP_SA			0xC4	/* Peripheral Backplane */
#define DRAC_SA			0x28	/* DRAC-III */
#define DRAC3_SA		0x30	/* DRAC-III */
#define BMC_LUN			0
#define SMS_LUN			2

struct ipmi_request {
	uint8_t	rsSa;
	uint8_t	rsLun;
	uint8_t	netFn;
	uint8_t	cmd;
	uint8_t	data_len;
	uint8_t	*data;
};

struct ipmi_response {
	uint8_t	cCode;
	uint8_t	data_len;
	uint8_t	*data;
};

struct ipmi_bmc_request {
	uint8_t	bmc_nfLn;
	uint8_t	bmc_cmd;
	uint8_t	bmc_data_len;
	uint8_t	bmc_data[1];
};

struct ipmi_bmc_response {
	uint8_t	bmc_nfLn;
	uint8_t	bmc_cmd;
	uint8_t	bmc_cCode;
	uint8_t	bmc_data_len;
	uint8_t	bmc_data[1];
};


CFATTACH_DECL2_NEW(ipmi, sizeof(struct ipmi_softc),
    ipmi_match, ipmi_attach, ipmi_detach, NULL, NULL, NULL);

/* Scan memory for signature */
void *
scan_sig(long start, long end, int skip, int len, const void *data)
{
	void *va;

	while (start < end) {
		va = ISA_HOLE_VADDR(start);
		if (memcmp(va, data, len) == 0)
			return va;

		start += skip;
	}

	return NULL;
}

void
dumpb(const char *lbl, int len, const uint8_t *data)
{
	int idx;

	printf("%s: ", lbl);
	for (idx = 0; idx < len; idx++)
		printf("%.2x ", data[idx]);

	printf("\n");
}

void
ipmi_smbios_probe(struct smbios_ipmi *pipmi, struct ipmi_attach_args *ia)
{
	const char *platform;

	dbg_printf(1, "%s: %#.2x %#.2x %#.2x %#.2x %#08" PRIx64
	    " %#.2x %#.2x\n", __func__,
	    pipmi->smipmi_if_type,
	    pipmi->smipmi_if_rev,
	    pipmi->smipmi_i2c_address,
	    pipmi->smipmi_nvram_address,
	    pipmi->smipmi_base_address,
	    pipmi->smipmi_base_flags,
	    pipmi->smipmi_irq);

	ia->iaa_if_type = pipmi->smipmi_if_type;
	ia->iaa_if_rev = pipmi->smipmi_if_rev;
	ia->iaa_if_irq = (pipmi->smipmi_base_flags & SMIPMI_FLAG_IRQEN) ?
	    pipmi->smipmi_irq : -1;
	ia->iaa_if_irqlvl = (pipmi->smipmi_base_flags & SMIPMI_FLAG_IRQLVL) ?
	    IST_LEVEL : IST_EDGE;

	switch (SMIPMI_FLAG_IFSPACING(pipmi->smipmi_base_flags)) {
	case IPMI_IOSPACING_BYTE:
		ia->iaa_if_iospacing = 1;
		break;

	case IPMI_IOSPACING_DWORD:
		ia->iaa_if_iospacing = 4;
		break;

	case IPMI_IOSPACING_WORD:
		ia->iaa_if_iospacing = 2;
		break;

	default:
		ia->iaa_if_iospacing = 1;
		aprint_error("%s: unknown register spacing\n", __func__);
	}

	/* Calculate base address (PCI BAR format) */
	if (pipmi->smipmi_base_address & 0x1) {
		ia->iaa_if_iotype = 'i';
		ia->iaa_if_iobase = pipmi->smipmi_base_address & ~0x1;
	} else {
		ia->iaa_if_iotype = 'm';
		ia->iaa_if_iobase = pipmi->smipmi_base_address & ~0xF;
	}
	if (pipmi->smipmi_base_flags & SMIPMI_FLAG_ODDOFFSET)
		ia->iaa_if_iobase++;

	platform = pmf_get_platform("system-product");
	if (platform != NULL &&
	    strcmp(platform, "ProLiant MicroServer") == 0 &&
	    pipmi->smipmi_base_address != 0) {
                ia->iaa_if_iospacing = 1;
                ia->iaa_if_iobase = pipmi->smipmi_base_address & ~0x7;
                ia->iaa_if_iotype = 'i';
                return;
        }

	if (pipmi->smipmi_base_flags == 0x7f) {
		/* IBM 325 eServer workaround */
		ia->iaa_if_iospacing = 1;
		ia->iaa_if_iobase = pipmi->smipmi_base_address;
		ia->iaa_if_iotype = 'i';
		return;
	}
}

/*
 * bt_buildmsg builds an IPMI message from a nfLun, cmd, and data
 * This is used by BT protocol
 *
 * Returns a buffer to an allocated message, txlen contains length
 *   of allocated message
 */
void *
bt_buildmsg(struct ipmi_softc *sc, int nfLun, int cmd, int len,
    const void *data, int *txlen)
{
	uint8_t *buf;

	/* Block transfer needs 4 extra bytes: length/netfn/seq/cmd + data */
	*txlen = len + 4;
	buf = ipmi_buf_acquire(sc, *txlen);
	if (buf == NULL)
		return NULL;

	buf[IPMI_BTMSG_LEN] = len + 3;
	buf[IPMI_BTMSG_NFLN] = nfLun;
	buf[IPMI_BTMSG_SEQ] = sc->sc_btseq++;
	buf[IPMI_BTMSG_CMD] = cmd;
	if (len && data)
		memcpy(buf + IPMI_BTMSG_DATASND, data, len);

	return buf;
}

/*
 * cmn_buildmsg builds an IPMI message from a nfLun, cmd, and data
 * This is used by both SMIC and KCS protocols
 *
 * Returns a buffer to an allocated message, txlen contains length
 *   of allocated message
 */
void *
cmn_buildmsg(struct ipmi_softc *sc, int nfLun, int cmd, int len,
    const void *data, int *txlen)
{
	uint8_t *buf;

	/* Common needs two extra bytes: nfLun/cmd + data */
	*txlen = len + 2;
	buf = ipmi_buf_acquire(sc, *txlen);
	if (buf == NULL)
		return NULL;

	buf[IPMI_MSG_NFLN] = nfLun;
	buf[IPMI_MSG_CMD] = cmd;
	if (len && data)
		memcpy(buf + IPMI_MSG_DATASND, data, len);

	return buf;
}

/*
 * ipmi_sendcmd: caller must hold sc_cmd_mtx.
 *
 * Send an IPMI command
 */
int
ipmi_sendcmd(struct ipmi_softc *sc, int rssa, int rslun, int netfn, int cmd,
    int txlen, const void *data)
{
	uint8_t	*buf;
	int		rc = -1;

	dbg_printf(50, "%s: rssa=%#.2x nfln=%#.2x cmd=%#.2x len=%#.2x\n",
	    __func__, rssa, NETFN_LUN(netfn, rslun), cmd, txlen);
	dbg_dump(10, __func__, txlen, data);
	if (rssa != BMC_SA) {
#if 0
		buf = sc->sc_if->buildmsg(sc, NETFN_LUN(APP_NETFN, BMC_LUN),
		    APP_SEND_MESSAGE, 7 + txlen, NULL, &txlen);
		pI2C->bus = (sc->if_ver == 0x09) ?
		    PUBLIC_BUS :
		    IPMB_CHANNEL_NUMBER;

		imbreq->rsSa = rssa;
		imbreq->nfLn = NETFN_LUN(netfn, rslun);
		imbreq->cSum1 = -(imbreq->rsSa + imbreq->nfLn);
		imbreq->rqSa = BMC_SA;
		imbreq->seqLn = NETFN_LUN(sc->imb_seq++, SMS_LUN);
		imbreq->cmd = cmd;
		if (txlen)
			memcpy(imbreq->data, data, txlen);
		/* Set message checksum */
		imbreq->data[txlen] = cksum8(&imbreq->rqSa, txlen + 3);
#endif
		goto done;
	} else
		buf = sc->sc_if->buildmsg(sc, NETFN_LUN(netfn, rslun), cmd,
		    txlen, data, &txlen);

	if (buf == NULL) {
		aprint_error_dev(sc->sc_dev, "sendcmd buffer busy\n");
		goto done;
	}
	rc = sc->sc_if->sendmsg(sc, txlen, buf);
	ipmi_buf_release(sc, buf);

	ipmi_delay(sc, 50); /* give bmc chance to digest command */

done:
	return rc;
}

void
ipmi_buf_release(struct ipmi_softc *sc, char *buf)
{
	KASSERT(sc->sc_buf_rsvd);
	KASSERT(sc->sc_buf == buf);
	sc->sc_buf_rsvd = false;
}

char *
ipmi_buf_acquire(struct ipmi_softc *sc, size_t len)
{
	KASSERT(len <= sizeof(sc->sc_buf));

	if (sc->sc_buf_rsvd || len > sizeof(sc->sc_buf))
		return NULL;
	sc->sc_buf_rsvd = true;
	return sc->sc_buf;
}

/*
 * ipmi_recvcmd: caller must hold sc_cmd_mtx.
 */
int
ipmi_recvcmd(struct ipmi_softc *sc, int maxlen, int *rxlen, void *data)
{
	uint8_t	*buf, rc = 0;
	int		rawlen;

	/* Need three extra bytes: netfn/cmd/ccode + data */
	buf = ipmi_buf_acquire(sc, maxlen + 3);
	if (buf == NULL) {
		aprint_error_dev(sc->sc_dev, "%s: malloc fails\n", __func__);
		return -1;
	}
	/* Receive message from interface, copy out result data */
	if (sc->sc_if->recvmsg(sc, maxlen + 3, &rawlen, buf)) {
		ipmi_buf_release(sc, buf);
		return -1;
	}

	*rxlen = rawlen - IPMI_MSG_DATARCV;
	if (*rxlen > 0 && data)
		memcpy(data, buf + IPMI_MSG_DATARCV, *rxlen);

	if ((rc = buf[IPMI_MSG_CCODE]) != 0)
		dbg_printf(1, "%s: nfln=%#.2x cmd=%#.2x err=%#.2x\n", __func__,
		    buf[IPMI_MSG_NFLN], buf[IPMI_MSG_CMD], buf[IPMI_MSG_CCODE]);

	dbg_printf(50, "%s: nfln=%#.2x cmd=%#.2x err=%#.2x len=%#.2x\n",
	    __func__, buf[IPMI_MSG_NFLN], buf[IPMI_MSG_CMD],
	    buf[IPMI_MSG_CCODE], *rxlen);
	dbg_dump(10, __func__, *rxlen, data);

	ipmi_buf_release(sc, buf);

	return rc;
}

/*
 * ipmi_delay: caller must hold sc_cmd_mtx.
 */
void
ipmi_delay(struct ipmi_softc *sc, int ms)
{
	if (cold) {
		delay(ms * 1000);
		return;
	}
	mutex_enter(&sc->sc_sleep_mtx);
	cv_timedwait(&sc->sc_cmd_sleep, &sc->sc_sleep_mtx, mstohz(ms));
	mutex_exit(&sc->sc_sleep_mtx);
}

/* Read a partial SDR entry */
int
get_sdr_partial(struct ipmi_softc *sc, uint16_t recordId, uint16_t reserveId,
    uint8_t offset, uint8_t length, void *buffer, uint16_t *nxtRecordId)
{
	uint8_t	cmd[256 + 8];
	int		len;

	((uint16_t *) cmd)[0] = reserveId;
	((uint16_t *) cmd)[1] = recordId;
	cmd[4] = offset;
	cmd[5] = length;
	mutex_enter(&sc->sc_cmd_mtx);
	if (ipmi_sendcmd(sc, BMC_SA, 0, STORAGE_NETFN, STORAGE_GET_SDR, 6,
	    cmd)) {
		mutex_exit(&sc->sc_cmd_mtx);
		aprint_error_dev(sc->sc_dev, "%s: sendcmd fails\n", __func__);
		return -1;
	}
	if (ipmi_recvcmd(sc, 8 + length, &len, cmd)) {
		mutex_exit(&sc->sc_cmd_mtx);
		aprint_error_dev(sc->sc_dev, "%s: recvcmd fails\n", __func__);
		return -1;
	}
	mutex_exit(&sc->sc_cmd_mtx);
	if (nxtRecordId)
		*nxtRecordId = *(uint16_t *) cmd;
	memcpy(buffer, cmd + 2, len - 2);

	return 0;
}

int maxsdrlen = 0x10;

/* Read an entire SDR; pass to add sensor */
int
get_sdr(struct ipmi_softc *sc, uint16_t recid, uint16_t *nxtrec)
{
	uint16_t	resid = 0;
	int		len, sdrlen, offset;
	uint8_t	*psdr;
	struct sdrhdr	shdr;

	mutex_enter(&sc->sc_cmd_mtx);
	/* Reserve SDR */
	if (ipmi_sendcmd(sc, BMC_SA, 0, STORAGE_NETFN, STORAGE_RESERVE_SDR,
	    0, NULL)) {
		mutex_exit(&sc->sc_cmd_mtx);
		aprint_error_dev(sc->sc_dev, "reserve send fails\n");
		return -1;
	}
	if (ipmi_recvcmd(sc, sizeof(resid), &len, &resid)) {
		mutex_exit(&sc->sc_cmd_mtx);
		aprint_error_dev(sc->sc_dev, "reserve recv fails\n");
		return -1;
	}
	mutex_exit(&sc->sc_cmd_mtx);
	/* Get SDR Header */
	if (get_sdr_partial(sc, recid, resid, 0, sizeof shdr, &shdr, nxtrec)) {
		aprint_error_dev(sc->sc_dev, "get header fails\n");
		return -1;
	}
	/* Allocate space for entire SDR Length of SDR in header does not
	 * include header length */
	sdrlen = sizeof(shdr) + shdr.record_length;
	psdr = malloc(sdrlen, M_DEVBUF, M_WAITOK|M_CANFAIL);
	if (psdr == NULL)
		return -1;

	memcpy(psdr, &shdr, sizeof(shdr));

	/* Read SDR Data maxsdrlen bytes at a time */
	for (offset = sizeof(shdr); offset < sdrlen; offset += maxsdrlen) {
		len = sdrlen - offset;
		if (len > maxsdrlen)
			len = maxsdrlen;

		if (get_sdr_partial(sc, recid, resid, offset, len,
		    psdr + offset, NULL)) {
			aprint_error_dev(sc->sc_dev,
			    "get chunk : %d,%d fails\n", offset, len);
			free(psdr, M_DEVBUF);
			return -1;
		}
	}

	/* Add SDR to sensor list, if not wanted, free buffer */
	if (add_sdr_sensor(sc, psdr) == 0)
		free(psdr, M_DEVBUF);

	return 0;
}

int
getbits(uint8_t *bytes, int bitpos, int bitlen)
{
	int	v;
	int	mask;

	bitpos += bitlen - 1;
	for (v = 0; bitlen--;) {
		v <<= 1;
		mask = 1L << (bitpos & 7);
		if (bytes[bitpos >> 3] & mask)
			v |= 1;
		bitpos--;
	}

	return v;
}

/* Decode IPMI sensor name */
void
ipmi_sensor_name(char *name, int len, uint8_t typelen, uint8_t *bits)
{
	int	i, slen;
	char	bcdplus[] = "0123456789 -.:,_";

	slen = typelen & 0x1F;
	switch (typelen >> 6) {
	case IPMI_NAME_UNICODE:
		//unicode
		break;

	case IPMI_NAME_BCDPLUS:
		/* Characters are encoded in 4-bit BCDPLUS */
		if (len < slen * 2 + 1)
			slen = (len >> 1) - 1;
		for (i = 0; i < slen; i++) {
			*(name++) = bcdplus[bits[i] >> 4];
			*(name++) = bcdplus[bits[i] & 0xF];
		}
		break;

	case IPMI_NAME_ASCII6BIT:
		/* Characters are encoded in 6-bit ASCII
		 *   0x00 - 0x3F maps to 0x20 - 0x5F */
		/* XXX: need to calculate max len: slen = 3/4 * len */
		if (len < slen + 1)
			slen = len - 1;
		for (i = 0; i < slen * 8; i += 6)
			*(name++) = getbits(bits, i, 6) + ' ';
		break;

	case IPMI_NAME_ASCII8BIT:
		/* Characters are 8-bit ascii */
		if (len < slen + 1)
			slen = len - 1;
		while (slen--)
			*(name++) = *(bits++);
		break;
	}
	*name = 0;
}

/* Sign extend a n-bit value */
long
signextend(unsigned long val, int bits)
{
	long msk = (1L << (bits-1))-1;

	return -(val & ~msk) | val;
}


/* fixpoint arithmetic */
#define FIX2INT(x)   ((int64_t)((x) >> 32))
#define INT2FIX(x)   ((int64_t)((uint64_t)(x) << 32))

#define FIX2            0x0000000200000000ll /* 2.0 */
#define FIX3            0x0000000300000000ll /* 3.0 */
#define FIXE            0x00000002b7e15163ll /* 2.71828182845904523536 */
#define FIX10           0x0000000a00000000ll /* 10.0 */
#define FIXMONE         0xffffffff00000000ll /* -1.0 */
#define FIXHALF         0x0000000080000000ll /* 0.5 */
#define FIXTHIRD        0x0000000055555555ll /* 0.33333333333333333333 */

#define FIX1LOG2        0x0000000171547653ll /* 1.0/log(2) */
#define FIX1LOGE        0x0000000100000000ll /* 1.0/log(2.71828182845904523536) */
#define FIX1LOG10       0x000000006F2DEC55ll /* 1.0/log(10) */

#define FIX1E           0x000000005E2D58D9ll /* 1.0/2.71828182845904523536 */

static int64_t fixlog_a[] = {
	0x0000000100000000ll /* 1.0/1.0 */,
	0xffffffff80000000ll /* -1.0/2.0 */,
	0x0000000055555555ll /* 1.0/3.0 */,
	0xffffffffc0000000ll /* -1.0/4.0 */,
	0x0000000033333333ll /* 1.0/5.0 */,
	0x000000002aaaaaabll /* -1.0/6.0 */,
	0x0000000024924925ll /* 1.0/7.0 */,
	0x0000000020000000ll /* -1.0/8.0 */,
	0x000000001c71c71cll /* 1.0/9.0 */
};

static int64_t fixexp_a[] = {
	0x0000000100000000ll /* 1.0/1.0 */,
	0x0000000100000000ll /* 1.0/1.0 */,
	0x0000000080000000ll /* 1.0/2.0 */,
	0x000000002aaaaaabll /* 1.0/6.0 */,
	0x000000000aaaaaabll /* 1.0/24.0 */,
	0x0000000002222222ll /* 1.0/120.0 */, 
	0x00000000005b05b0ll /* 1.0/720.0 */,
	0x00000000000d00d0ll /* 1.0/5040.0 */,
	0x000000000001a01all /* 1.0/40320.0 */
};

static int64_t
fixmul(int64_t x, int64_t y)
{
	int64_t z;
	int64_t a,b,c,d;
	int neg;

	neg = 0;
	if (x < 0) {
		x = -x;
		neg = !neg;
	}
	if (y < 0) { 
		y = -y;
		neg = !neg;
	}

	a = FIX2INT(x);
	b = x - INT2FIX(a);
	c = FIX2INT(y);
	d = y - INT2FIX(c);

	z = INT2FIX(a*c) + a * d + b * c + (b/2 * d/2 >> 30);

	return neg ? -z : z;
}

static int64_t
poly(int64_t x0, int64_t x, int64_t a[], int n)
{
	int64_t z;
	int i;

	z  = fixmul(x0, a[0]);
	for (i=1; i<n; ++i) {
		x0 = fixmul(x0, x);
		z  = fixmul(x0, a[i]) + z;
	}
	return z;
}

static int64_t
logx(int64_t x, int64_t y)
{
	int64_t z;

	if (x <= INT2FIX(0)) {
		z = INT2FIX(-99999);
		goto done;
	}

	z = INT2FIX(0);
	while (x >= FIXE) {
		x = fixmul(x, FIX1E);
		z += INT2FIX(1);
	}
	while (x < INT2FIX(1)) {
		x = fixmul(x, FIXE);
		z -= INT2FIX(1);
	}

	x -= INT2FIX(1);
	z += poly(x, x, fixlog_a, sizeof(fixlog_a)/sizeof(fixlog_a[0]));
	z  = fixmul(z, y);

done:
	return z;
}

static int64_t
powx(int64_t x, int64_t y)
{
	int64_t k;

	if (x == INT2FIX(0))
		goto done;

	x = logx(x,y);

	if (x < INT2FIX(0)) {
		x = INT2FIX(0) - x;
		k = -FIX2INT(x);
		x = INT2FIX(-k) - x;
	} else {
		k = FIX2INT(x);
		x = x - INT2FIX(k);
	}

	x = poly(INT2FIX(1), x, fixexp_a, sizeof(fixexp_a)/sizeof(fixexp_a[0]));

	while (k < 0) {
		x = fixmul(x, FIX1E);
		++k;
	}
	while (k > 0) {
		x = fixmul(x, FIXE);
		--k;
	}

done:
	return x;
}

/* Convert IPMI reading from sensor factors */
long
ipmi_convert(uint8_t v, struct sdrtype1 *s1, long adj)
{
	int64_t	M, B;
	char	K1, K2;
	int64_t	val, v1, v2, vs;
	int sign = (s1->units1 >> 6) & 0x3;

	vs = (sign == 0x1 || sign == 0x2) ? (int8_t)v : v;
	if ((vs < 0) && (sign == 0x1))
		vs++;

	/* Calculate linear reading variables */
	M  = signextend((((short)(s1->m_tolerance & 0xC0)) << 2) + s1->m, 10);
	B  = signextend((((short)(s1->b_accuracy & 0xC0)) << 2) + s1->b, 10);
	K1 = signextend(s1->rbexp & 0xF, 4);
	K2 = signextend(s1->rbexp >> 4, 4);

	/* Calculate sensor reading:
	 *  y = L((M * v + (B * 10^K1)) * 10^(K2+adj)
	 *
	 * This commutes out to:
	 *  y = L(M*v * 10^(K2+adj) + B * 10^(K1+K2+adj)); */
	v1 = powx(FIX10, INT2FIX(K2 + adj));
	v2 = powx(FIX10, INT2FIX(K1 + K2 + adj));
	val = M * vs * v1 + B * v2;

	/* Linearization function: y = f(x) 0 : y = x 1 : y = ln(x) 2 : y =
	 * log10(x) 3 : y = log2(x) 4 : y = e^x 5 : y = 10^x 6 : y = 2^x 7 : y
	 * = 1/x 8 : y = x^2 9 : y = x^3 10 : y = square root(x) 11 : y = cube
	 * root(x) */
	switch (s1->linear & 0x7f) {
	case 0: break;
	case 1: val = logx(val,FIX1LOGE); break;
	case 2: val = logx(val,FIX1LOG10); break;
	case 3: val = logx(val,FIX1LOG2); break;
	case 4: val = powx(FIXE,val); break;
	case 5: val = powx(FIX10,val); break;
	case 6: val = powx(FIX2,val); break;
	case 7: val = powx(val,FIXMONE); break;
	case 8: val = powx(val,FIX2); break;
	case 9: val = powx(val,FIX3); break;
	case 10: val = powx(val,FIXHALF); break;
	case 11: val = powx(val,FIXTHIRD); break;
	}

	return FIX2INT(val);
}

int32_t
ipmi_convert_sensor(uint8_t *reading, struct ipmi_sensor *psensor)
{
	struct sdrtype1	*s1 = (struct sdrtype1 *)psensor->i_sdr;
	int32_t val;

	switch (psensor->i_envtype) {
	case ENVSYS_STEMP:
		val = ipmi_convert(reading[0], s1, 6) + 273150000;
		break;

	case ENVSYS_SVOLTS_DC:
		val = ipmi_convert(reading[0], s1, 6);
		break;

	case ENVSYS_SFANRPM:
		val = ipmi_convert(reading[0], s1, 0);
		if (((s1->units1>>3)&0x7) == 0x3)
			val *= 60; /* RPS -> RPM */
		break;
	default:
		val = 0;
		break;
	}
	return val;
}

void
ipmi_set_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
		sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct ipmi_sensor *ipmi_s;

	/* Find the ipmi_sensor corresponding to this edata */
	SLIST_FOREACH(ipmi_s, &ipmi_sensor_list, i_list) {
		if (ipmi_s->i_envnum == edata->sensor) {
			if (limits == NULL) {
				limits = &ipmi_s->i_deflims;
				props  = &ipmi_s->i_defprops;
			}
			*props |= PROP_DRIVER_LIMITS;
			ipmi_s->i_limits = *limits;
			ipmi_s->i_props  = *props;
			return;
		}
	}
	return;
}

void
ipmi_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
		sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct ipmi_sensor *ipmi_s;
	struct ipmi_softc *sc = sme->sme_cookie;

	/* Find the ipmi_sensor corresponding to this edata */
	SLIST_FOREACH(ipmi_s, &ipmi_sensor_list, i_list) {
		if (ipmi_s->i_envnum == edata->sensor) {
			ipmi_get_sensor_limits(sc, ipmi_s, limits, props);
			ipmi_s->i_limits = *limits;
			ipmi_s->i_props  = *props;
			if (ipmi_s->i_defprops == 0) {
				ipmi_s->i_defprops = *props;
				ipmi_s->i_deflims  = *limits;
			}
			return;
		}
	}
	return;
}

void
ipmi_get_sensor_limits(struct ipmi_softc *sc, struct ipmi_sensor *psensor,
		       sysmon_envsys_lim_t *limits, uint32_t *props)
{
	struct sdrtype1	*s1 = (struct sdrtype1 *)psensor->i_sdr;
	bool failure;
	int	rxlen;
	uint8_t	data[32];
	uint32_t prop_critmax, prop_warnmax, prop_critmin, prop_warnmin;
	int32_t *pcritmax, *pwarnmax, *pcritmin, *pwarnmin;

	*props &= ~(PROP_CRITMIN | PROP_CRITMAX | PROP_WARNMIN | PROP_WARNMAX);
	data[0] = psensor->i_num;
	mutex_enter(&sc->sc_cmd_mtx);
	failure =
	    ipmi_sendcmd(sc, s1->owner_id, s1->owner_lun,
			 SE_NETFN, SE_GET_SENSOR_THRESHOLD, 1, data) ||
	    ipmi_recvcmd(sc, sizeof(data), &rxlen, data);
	mutex_exit(&sc->sc_cmd_mtx);
	if (failure)
		return;

	dbg_printf(25, "%s: %#.2x %#.2x %#.2x %#.2x %#.2x %#.2x %#.2x\n",
	    __func__, data[0], data[1], data[2], data[3], data[4], data[5],
	    data[6]);

	switch (s1->linear & 0x7f) {
	case 7: /* 1/x sensor, exchange upper and lower limits */
		prop_critmax = PROP_CRITMIN;
		prop_warnmax = PROP_WARNMIN;
		prop_critmin = PROP_CRITMAX;
		prop_warnmin = PROP_WARNMAX;
		pcritmax = &limits->sel_critmin;
		pwarnmax = &limits->sel_warnmin;
		pcritmin = &limits->sel_critmax;
		pwarnmin = &limits->sel_warnmax;
		break;
	default:
		prop_critmax = PROP_CRITMAX;
		prop_warnmax = PROP_WARNMAX;
		prop_critmin = PROP_CRITMIN;
		prop_warnmin = PROP_WARNMIN;
		pcritmax = &limits->sel_critmax;
		pwarnmax = &limits->sel_warnmax;
		pcritmin = &limits->sel_critmin;
		pwarnmin = &limits->sel_warnmin;
		break;
	}

	if (data[0] & 0x20 && data[6] != 0xff) {
		*pcritmax = ipmi_convert_sensor(&data[6], psensor);
		*props |= prop_critmax;
	}
	if (data[0] & 0x10 && data[5] != 0xff) {
		*pcritmax = ipmi_convert_sensor(&data[5], psensor);
		*props |= prop_critmax;
	}
	if (data[0] & 0x08 && data[4] != 0xff) {
		*pwarnmax = ipmi_convert_sensor(&data[4], psensor);
		*props |= prop_warnmax;
	}
	if (data[0] & 0x04 && data[3] != 0x00) {
		*pcritmin = ipmi_convert_sensor(&data[3], psensor);
		*props |= prop_critmin;
	}
	if (data[0] & 0x02 && data[2] != 0x00) {
		*pcritmin = ipmi_convert_sensor(&data[2], psensor);
		*props |= prop_critmin;
	}
	if (data[0] & 0x01 && data[1] != 0x00) {
		*pwarnmin = ipmi_convert_sensor(&data[1], psensor);
		*props |= prop_warnmin;
	}
	return;
}

int
ipmi_sensor_status(struct ipmi_softc *sc, struct ipmi_sensor *psensor,
    envsys_data_t *edata, uint8_t *reading)
{
	int	etype;

	/* Get reading of sensor */
	edata->value_cur = ipmi_convert_sensor(reading, psensor);

	/* Return Sensor Status */
	etype = (psensor->i_etype << 8) + psensor->i_stype;
	switch (etype) {
	case IPMI_SENSOR_TYPE_TEMP:
	case IPMI_SENSOR_TYPE_VOLT:
	case IPMI_SENSOR_TYPE_FAN:
		if (psensor->i_props & PROP_CRITMAX &&
		    edata->value_cur > psensor->i_limits.sel_critmax)
			return ENVSYS_SCRITOVER;

		if (psensor->i_props & PROP_WARNMAX &&
		    edata->value_cur > psensor->i_limits.sel_warnmax)
			return ENVSYS_SWARNOVER;

		if (psensor->i_props & PROP_WARNMIN &&
		    edata->value_cur < psensor->i_limits.sel_warnmin)
			return ENVSYS_SWARNUNDER;

		if (psensor->i_props & PROP_CRITMIN &&
		    edata->value_cur < psensor->i_limits.sel_critmin)
			return ENVSYS_SCRITUNDER;

		break;

	case IPMI_SENSOR_TYPE_INTRUSION:
		edata->value_cur = (reading[2] & 1) ? 0 : 1;
		if (reading[2] & 0x1)
			return ENVSYS_SCRITICAL;
		break;

	case IPMI_SENSOR_TYPE_PWRSUPPLY:
		/* Reading: 1 = present+powered, 0 = otherwise */
		edata->value_cur = (reading[2] & 1) ? 0 : 1;
		if (reading[2] & 0x10) {
			/* XXX: Need envsys type for Power Supply types
			 *   ok: power supply installed && powered
			 * warn: power supply installed && !powered
			 * crit: power supply !installed
			 */
			return ENVSYS_SCRITICAL;
		}
		if (reading[2] & 0x08) {
			/* Power supply AC lost */
			return ENVSYS_SWARNOVER;
		}
		break;
	}

	return ENVSYS_SVALID;
}

int
read_sensor(struct ipmi_softc *sc, struct ipmi_sensor *psensor)
{
	struct sdrtype1	*s1 = (struct sdrtype1 *) psensor->i_sdr;
	uint8_t	data[8];
	int		rxlen;
	envsys_data_t *edata = &sc->sc_sensor[psensor->i_envnum];

	memset(data, 0, sizeof(data));
	data[0] = psensor->i_num;

	mutex_enter(&sc->sc_cmd_mtx);
	if (ipmi_sendcmd(sc, s1->owner_id, s1->owner_lun, SE_NETFN,
	    SE_GET_SENSOR_READING, 1, data))
		goto err;

	if (ipmi_recvcmd(sc, sizeof(data), &rxlen, data))
		goto err;
	mutex_exit(&sc->sc_cmd_mtx);

	dbg_printf(10, "m=%u, m_tolerance=%u, b=%u, b_accuracy=%u, "
	    "rbexp=%u, linear=%d\n", s1->m, s1->m_tolerance, s1->b,
	    s1->b_accuracy, s1->rbexp, s1->linear);
	dbg_printf(10, "values=%#.2x %#.2x %#.2x %#.2x %s\n",
	    data[0],data[1],data[2],data[3], edata->desc);
	if (IPMI_INVALID_SENSOR_P(data[1])) {
		/* Check if sensor is valid */
		edata->state = ENVSYS_SINVALID;
	} else {
		edata->state = ipmi_sensor_status(sc, psensor, edata, data);
	}
	return 0;
err:
	mutex_exit(&sc->sc_cmd_mtx);
	return -1;
}

int
ipmi_sensor_type(int type, int ext_type, int entity)
{
	switch (ext_type << 8L | type) {
	case IPMI_SENSOR_TYPE_TEMP:
		return ENVSYS_STEMP;

	case IPMI_SENSOR_TYPE_VOLT:
		return ENVSYS_SVOLTS_DC;

	case IPMI_SENSOR_TYPE_FAN:
		return ENVSYS_SFANRPM;

	case IPMI_SENSOR_TYPE_PWRSUPPLY:
		if (entity == IPMI_ENTITY_PWRSUPPLY)
			return ENVSYS_INDICATOR;
		break;

	case IPMI_SENSOR_TYPE_INTRUSION:
		return ENVSYS_INDICATOR;
	}

	return -1;
}

/* Add Sensor to BSD Sysctl interface */
int
add_sdr_sensor(struct ipmi_softc *sc, uint8_t *psdr)
{
	int			rc;
	struct sdrtype1		*s1 = (struct sdrtype1 *)psdr;
	struct sdrtype2		*s2 = (struct sdrtype2 *)psdr;
	char			name[64];

	switch (s1->sdrhdr.record_type) {
	case IPMI_SDR_TYPEFULL:
		ipmi_sensor_name(name, sizeof(name), s1->typelen, s1->name);
		rc = add_child_sensors(sc, psdr, 1, s1->sensor_num,
		    s1->sensor_type, s1->event_code, 0, s1->entity_id, name);
		break;

	case IPMI_SDR_TYPECOMPACT:
		ipmi_sensor_name(name, sizeof(name), s2->typelen, s2->name);
		rc = add_child_sensors(sc, psdr, s2->share1 & 0xF,
		    s2->sensor_num, s2->sensor_type, s2->event_code,
		    s2->share2 & 0x7F, s2->entity_id, name);
		break;

	default:
		return 0;
	}

	return rc;
}

static int
ipmi_is_dupname(char *name)
{
	struct ipmi_sensor *ipmi_s;

	SLIST_FOREACH(ipmi_s, &ipmi_sensor_list, i_list) {
		if (strcmp(ipmi_s->i_envdesc, name) == 0) {
			return 1;
		}
	}
	return 0;
}

int
add_child_sensors(struct ipmi_softc *sc, uint8_t *psdr, int count,
    int sensor_num, int sensor_type, int ext_type, int sensor_base,
    int entity, const char *name)
{
	int			typ, idx, dupcnt, c;
	char			*e;
	struct ipmi_sensor	*psensor;
	struct sdrtype1		*s1 = (struct sdrtype1 *)psdr;
	
	typ = ipmi_sensor_type(sensor_type, ext_type, entity);
	if (typ == -1) {
		dbg_printf(5, "Unknown sensor type:%#.2x et:%#.2x sn:%#.2x "
		    "name:%s\n", sensor_type, ext_type, sensor_num, name);
		return 0;
	}
	dupcnt = 0;
	sc->sc_nsensors += count;
	for (idx = 0; idx < count; idx++) {
		psensor = malloc(sizeof(struct ipmi_sensor), M_DEVBUF,
		    M_WAITOK|M_CANFAIL);
		if (psensor == NULL)
			break;

		memset(psensor, 0, sizeof(struct ipmi_sensor));

		/* Initialize BSD Sensor info */
		psensor->i_sdr = psdr;
		psensor->i_num = sensor_num + idx;
		psensor->i_stype = sensor_type;
		psensor->i_etype = ext_type;
		psensor->i_envtype = typ;
		if (count > 1)
			snprintf(psensor->i_envdesc,
			    sizeof(psensor->i_envdesc),
			    "%s - %d", name, sensor_base + idx);
		else
			strlcpy(psensor->i_envdesc, name,
			    sizeof(psensor->i_envdesc));

		/*
		 * Check for duplicates.  If there are duplicates,
		 * make sure there is space in the name (if not,
		 * truncate to make space) for a count (1-99) to
		 * add to make the name unique.  If we run the
		 * counter out, just accept the duplicate (@name99)
		 * for now.
		 */
		if (ipmi_is_dupname(psensor->i_envdesc)) {
			if (strlen(psensor->i_envdesc) >=
			    sizeof(psensor->i_envdesc) - 3) {
				e = psensor->i_envdesc +
				    sizeof(psensor->i_envdesc) - 3;
			} else {
				e = psensor->i_envdesc +
				    strlen(psensor->i_envdesc);
			}
			c = psensor->i_envdesc +
			    sizeof(psensor->i_envdesc) - e;
			do {
				dupcnt++;
				snprintf(e, c, "%d", dupcnt);
			} while (dupcnt < 100 &&
			         ipmi_is_dupname(psensor->i_envdesc));
		}

		dbg_printf(5, "%s: %#.4x %#.2x:%d ent:%#.2x:%#.2x %s\n",
		    __func__,
		    s1->sdrhdr.record_id, s1->sensor_type,
		    typ, s1->entity_id, s1->entity_instance,
		    psensor->i_envdesc);
		SLIST_INSERT_HEAD(&ipmi_sensor_list, psensor, i_list);
	}

	return 1;
}

/* Interrupt handler */
int
ipmi_intr(void *arg)
{
	struct ipmi_softc	*sc = (struct ipmi_softc *)arg;
	int			v;

	v = bmc_read(sc, _KCS_STATUS_REGISTER);
	if (v & KCS_OBF)
		++ipmi_nintr;

	return 0;
}

/* Handle IPMI Timer - reread sensor values */
void
ipmi_refresh_sensors(struct ipmi_softc *sc)
{

	if (SLIST_EMPTY(&ipmi_sensor_list))
		return;

	sc->current_sensor = SLIST_NEXT(sc->current_sensor, i_list);
	if (sc->current_sensor == NULL)
		sc->current_sensor = SLIST_FIRST(&ipmi_sensor_list);

	if (read_sensor(sc, sc->current_sensor)) {
		dbg_printf(1, "%s: error reading\n", __func__);
	}
}

int
ipmi_map_regs(struct ipmi_softc *sc, struct ipmi_attach_args *ia)
{
	sc->sc_if = ipmi_get_if(ia->iaa_if_type);
	if (sc->sc_if == NULL)
		return -1;

	if (ia->iaa_if_iotype == 'i')
		sc->sc_iot = ia->iaa_iot;
	else
		sc->sc_iot = ia->iaa_memt;

	sc->sc_if_rev = ia->iaa_if_rev;
	sc->sc_if_iospacing = ia->iaa_if_iospacing;
	if (bus_space_map(sc->sc_iot, ia->iaa_if_iobase,
	    sc->sc_if->nregs * sc->sc_if_iospacing,
	    0, &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev,
		    "%s: bus_space_map(..., %x, %x, 0, %p) failed\n",
		    __func__, ia->iaa_if_iobase,
		    sc->sc_if->nregs * sc->sc_if_iospacing, &sc->sc_ioh);
		return -1;
	}
#if 0
	if (iaa->if_if_irq != -1)
		sc->ih = isa_intr_establish(-1, iaa->if_if_irq,
		    iaa->if_irqlvl, IPL_BIO, ipmi_intr, sc,
		    device_xname(sc->sc_dev);
#endif
	return 0;
}

void
ipmi_unmap_regs(struct ipmi_softc *sc)
{
	bus_space_unmap(sc->sc_iot, sc->sc_ioh,
	    sc->sc_if->nregs * sc->sc_if_iospacing);
}

int
ipmi_probe(struct ipmi_attach_args *ia)
{
	struct dmd_ipmi *pipmi;
	struct smbtable tbl;

	tbl.cookie = 0;

	if (smbios_find_table(SMBIOS_TYPE_IPMIDEV, &tbl))
		ipmi_smbios_probe(tbl.tblhdr, ia);
	else {
		pipmi = scan_sig(0xC0000L, 0xFFFFFL, 16, 4, "IPMI");
		/* XXX hack to find Dell PowerEdge 8450 */
		if (pipmi == NULL) {
			/* no IPMI found */
			return 0;
		}

		/* we have an IPMI signature, fill in attach arg structure */
		ia->iaa_if_type = pipmi->dmd_if_type;
		ia->iaa_if_rev = pipmi->dmd_if_rev;
	}

	return 1;
}

int
ipmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ipmi_softc sc;
	struct ipmi_attach_args *ia = aux;
	uint8_t		cmd[32];
	int			len;
	int			rv = 0;

	memset(&sc, 0, sizeof(sc));

	/* Map registers */
	if (ipmi_map_regs(&sc, ia) != 0)
		return 0;

	sc.sc_if->probe(&sc);

	mutex_init(&sc.sc_cmd_mtx, MUTEX_DEFAULT, IPL_SOFTCLOCK);
	cv_init(&sc.sc_cmd_sleep, "ipmimtch");
	mutex_enter(&sc.sc_cmd_mtx);
	/* Identify BMC device early to detect lying bios */
	if (ipmi_sendcmd(&sc, BMC_SA, 0, APP_NETFN, APP_GET_DEVICE_ID,
	    0, NULL)) {
		mutex_exit(&sc.sc_cmd_mtx);
		dbg_printf(1, ": unable to send get device id "
		    "command\n");
		goto unmap;
	}
	if (ipmi_recvcmd(&sc, sizeof(cmd), &len, cmd)) {
		mutex_exit(&sc.sc_cmd_mtx);
		dbg_printf(1, ": unable to retrieve device id\n");
		goto unmap;
	}
	mutex_exit(&sc.sc_cmd_mtx);

	dbg_dump(1, __func__, len, cmd);
	rv = 1; /* GETID worked, we got IPMI */
unmap:
	cv_destroy(&sc.sc_cmd_sleep);
	mutex_destroy(&sc.sc_cmd_mtx);
	ipmi_unmap_regs(&sc);

	return rv;
}

static void
ipmi_thread(void *cookie)
{
	device_t		self = cookie;
	struct ipmi_softc	*sc = device_private(self);
	struct ipmi_attach_args *ia = &sc->sc_ia;
	uint16_t		rec;
	struct ipmi_sensor *ipmi_s;
	int i;

	sc->sc_thread_running = true;

	/* setup ticker */
	sc->sc_max_retries = hz * 90; /* 90 seconds max */

	/* Map registers */
	ipmi_map_regs(sc, ia);

	/* Scan SDRs, add sensors to list */
	for (rec = 0; rec != 0xFFFF;)
		if (get_sdr(sc, rec, &rec))
			break;

	/* allocate and fill sensor arrays */
	sc->sc_sensor =
	    malloc(sizeof(envsys_data_t) * sc->sc_nsensors,
	        M_DEVBUF, M_WAITOK | M_ZERO);
	if (sc->sc_sensor == NULL) {
		aprint_error_dev(self, "can't allocate envsys_data_t\n");
		kthread_exit(0);
	}

	sc->sc_envsys = sysmon_envsys_create();
	sc->sc_envsys->sme_cookie = sc;
	sc->sc_envsys->sme_get_limits = ipmi_get_limits;
	sc->sc_envsys->sme_set_limits = ipmi_set_limits;

	i = 0;
	SLIST_FOREACH(ipmi_s, &ipmi_sensor_list, i_list) {
		ipmi_s->i_props = 0;
		ipmi_s->i_envnum = -1;
		sc->sc_sensor[i].units = ipmi_s->i_envtype;
		sc->sc_sensor[i].state = ENVSYS_SINVALID;
		sc->sc_sensor[i].flags |= ENVSYS_FHAS_ENTROPY;
		/*
		 * Monitor threshold limits in the sensors.
		 */
		switch (sc->sc_sensor[i].units) {
		case ENVSYS_STEMP:
		case ENVSYS_SVOLTS_DC:
		case ENVSYS_SFANRPM:
			sc->sc_sensor[i].flags |= ENVSYS_FMONLIMITS;
			break;
		default:
			sc->sc_sensor[i].flags |= ENVSYS_FMONCRITICAL;
		}
		(void)strlcpy(sc->sc_sensor[i].desc, ipmi_s->i_envdesc,
		    sizeof(sc->sc_sensor[i].desc));
		++i;

		if (sysmon_envsys_sensor_attach(sc->sc_envsys,
						&sc->sc_sensor[i-1]))
			continue;

		/* get reference number from envsys */
		ipmi_s->i_envnum = sc->sc_sensor[i-1].sensor;
	}

	sc->sc_envsys->sme_name = device_xname(sc->sc_dev);
	sc->sc_envsys->sme_flags = SME_DISABLE_REFRESH;

	if (sysmon_envsys_register(sc->sc_envsys)) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_envsys);
	}

	/* initialize sensor list for thread */
	if (!SLIST_EMPTY(&ipmi_sensor_list))
		sc->current_sensor = SLIST_FIRST(&ipmi_sensor_list);

	aprint_verbose_dev(self, "version %d.%d interface %s %sbase "
	    "%#x/%#x spacing %d\n",
	    ia->iaa_if_rev >> 4, ia->iaa_if_rev & 0xF, sc->sc_if->name,
	    ia->iaa_if_iotype == 'i' ? "io" : "mem", ia->iaa_if_iobase,
	    ia->iaa_if_iospacing * sc->sc_if->nregs, ia->iaa_if_iospacing);
	if (ia->iaa_if_irq != -1)
		aprint_verbose_dev(self, " irq %d\n", ia->iaa_if_irq);

	/* setup flag to exclude iic */
	ipmi_enabled = 1;

	/* Setup Watchdog timer */
	sc->sc_wdog.smw_name = device_xname(sc->sc_dev);
	sc->sc_wdog.smw_cookie = sc;
	sc->sc_wdog.smw_setmode = ipmi_watchdog_setmode;
	sc->sc_wdog.smw_tickle = ipmi_watchdog_tickle;
	sysmon_wdog_register(&sc->sc_wdog);

	/* Set up a power handler so we can possibly sleep */
	if (!pmf_device_register(self, ipmi_suspend, NULL))
                aprint_error_dev(self, "couldn't establish a power handler\n");

	mutex_enter(&sc->sc_poll_mtx);
	while (sc->sc_thread_running) {
		ipmi_refresh_sensors(sc);
		cv_timedwait(&sc->sc_poll_cv, &sc->sc_poll_mtx,
		    SENSOR_REFRESH_RATE);
		if (sc->sc_tickle_due) {
			ipmi_dotickle(sc);
			sc->sc_tickle_due = false;
		}
	}
	mutex_exit(&sc->sc_poll_mtx);
	kthread_exit(0);
}

void
ipmi_attach(device_t parent, device_t self, void *aux)
{
	struct ipmi_softc	*sc = device_private(self);

	sc->sc_ia = *(struct ipmi_attach_args *)aux;
	sc->sc_dev = self;
	aprint_naive("\n");
	aprint_normal("\n");

	/* lock around read_sensor so that no one messes with the bmc regs */
	mutex_init(&sc->sc_cmd_mtx, MUTEX_DEFAULT, IPL_SOFTCLOCK);
	mutex_init(&sc->sc_sleep_mtx, MUTEX_DEFAULT, IPL_SOFTCLOCK);
	cv_init(&sc->sc_cmd_sleep, "ipmicmd");

	mutex_init(&sc->sc_poll_mtx, MUTEX_DEFAULT, IPL_SOFTCLOCK);
	cv_init(&sc->sc_poll_cv, "ipmipoll");

	if (kthread_create(PRI_NONE, 0, NULL, ipmi_thread, self,
	    &sc->sc_kthread, "%s", device_xname(self)) != 0) {
		aprint_error_dev(self, "unable to create thread, disabled\n");
	}
}

static int
ipmi_detach(device_t self, int flags)
{
	struct ipmi_sensor *i;
	int rc;
	struct ipmi_softc *sc = device_private(self);

	mutex_enter(&sc->sc_poll_mtx);
	sc->sc_thread_running = false;
	cv_signal(&sc->sc_poll_cv);
	mutex_exit(&sc->sc_poll_mtx);

	if ((rc = sysmon_wdog_unregister(&sc->sc_wdog)) != 0) {
		if (rc == ERESTART)
			rc = EINTR;
		return rc;
	}

	/* cancel any pending countdown */
	sc->sc_wdog.smw_mode &= ~WDOG_MODE_MASK;
	sc->sc_wdog.smw_mode |= WDOG_MODE_DISARMED;
	sc->sc_wdog.smw_period = WDOG_PERIOD_DEFAULT;

	if ((rc = ipmi_watchdog_setmode(&sc->sc_wdog)) != 0)
		return rc;

	ipmi_enabled = 0;

	if (sc->sc_envsys != NULL) {
		/* _unregister also destroys */
		sysmon_envsys_unregister(sc->sc_envsys);
		sc->sc_envsys = NULL;
	}

	while ((i = SLIST_FIRST(&ipmi_sensor_list)) != NULL) {
		SLIST_REMOVE_HEAD(&ipmi_sensor_list, i_list);
		free(i, M_DEVBUF);
	}

	if (sc->sc_sensor != NULL) {
		free(sc->sc_sensor, M_DEVBUF);
		sc->sc_sensor = NULL;
	}

	ipmi_unmap_regs(sc);

	cv_destroy(&sc->sc_poll_cv);
	mutex_destroy(&sc->sc_poll_mtx);
	cv_destroy(&sc->sc_cmd_sleep);
	mutex_destroy(&sc->sc_sleep_mtx);
	mutex_destroy(&sc->sc_cmd_mtx);

	return 0;
}

int
ipmi_watchdog_setmode(struct sysmon_wdog *smwdog)
{
	struct ipmi_softc	*sc = smwdog->smw_cookie;
	struct ipmi_get_watchdog gwdog;
	struct ipmi_set_watchdog swdog;
	int			rc, len;

	if (smwdog->smw_period < 10)
		return EINVAL;
	if (smwdog->smw_period == WDOG_PERIOD_DEFAULT)
		sc->sc_wdog.smw_period = 10;
	else
		sc->sc_wdog.smw_period = smwdog->smw_period;

	mutex_enter(&sc->sc_cmd_mtx);
	/* see if we can properly task to the watchdog */
	rc = ipmi_sendcmd(sc, BMC_SA, BMC_LUN, APP_NETFN,
	    APP_GET_WATCHDOG_TIMER, 0, NULL);
	rc = ipmi_recvcmd(sc, sizeof(gwdog), &len, &gwdog);
	mutex_exit(&sc->sc_cmd_mtx);
	if (rc) {
		aprint_error_dev(sc->sc_dev,
		    "APP_GET_WATCHDOG_TIMER returned %#x\n", rc);
		return EIO;
	}

	memset(&swdog, 0, sizeof(swdog));
	/* Period is 10ths/sec */
	swdog.wdog_timeout = htole16(sc->sc_wdog.smw_period * 10);
	if ((smwdog->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED)
		swdog.wdog_action = IPMI_WDOG_ACT_DISABLED;
	else
		swdog.wdog_action = IPMI_WDOG_ACT_RESET;
	swdog.wdog_use = IPMI_WDOG_USE_USE_OS;

	mutex_enter(&sc->sc_cmd_mtx);
	if ((rc = ipmi_sendcmd(sc, BMC_SA, BMC_LUN, APP_NETFN,
	    APP_SET_WATCHDOG_TIMER, sizeof(swdog), &swdog)) == 0)
		rc = ipmi_recvcmd(sc, 0, &len, NULL);
	mutex_exit(&sc->sc_cmd_mtx);
	if (rc) {
		aprint_error_dev(sc->sc_dev,
		    "APP_SET_WATCHDOG_TIMER returned %#x\n", rc);
		return EIO;
	}

	return 0;
}

int
ipmi_watchdog_tickle(struct sysmon_wdog *smwdog)
{
	struct ipmi_softc	*sc = smwdog->smw_cookie;

	mutex_enter(&sc->sc_poll_mtx);
	sc->sc_tickle_due = true;
	cv_signal(&sc->sc_poll_cv);
	mutex_exit(&sc->sc_poll_mtx);
	return 0;
}

void
ipmi_dotickle(struct ipmi_softc *sc)
{
	int			rc, len;

	mutex_enter(&sc->sc_cmd_mtx);
	/* tickle the watchdog */
	if ((rc = ipmi_sendcmd(sc, BMC_SA, BMC_LUN, APP_NETFN,
	    APP_RESET_WATCHDOG, 0, NULL)) == 0)
		rc = ipmi_recvcmd(sc, 0, &len, NULL);
	mutex_exit(&sc->sc_cmd_mtx);
	if (rc != 0) {
		aprint_error_dev(sc->sc_dev, "watchdog tickle returned %#x\n",
		    rc);
	}
}

bool
ipmi_suspend(device_t dev, const pmf_qual_t *qual)
{
	struct ipmi_softc *sc = device_private(dev);

	/* Don't allow suspend if watchdog is armed */
	if ((sc->sc_wdog.smw_mode & WDOG_MODE_MASK) != WDOG_MODE_DISARMED)
		return false;
	return true;
}
