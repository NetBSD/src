/*	$NetBSD: apmbios.c,v 1.7.2.3 2008/01/21 09:36:55 yamt Exp $ */

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl and Christopher G. Demetriou.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apmbios.c,v 1.7.2.3 2008/01/21 09:36:55 yamt Exp $");

#include "opt_apm.h"
#include "opt_compat_mach.h"	/* Needed to get the right segment def */

#ifdef APM_NOIDLE
#error APM_NOIDLE option deprecated; use APM_NO_IDLE instead
#endif

#if defined(DEBUG) && !defined(APMDEBUG)
#define	APMDEBUG
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/stdarg.h>
#include <machine/cpufunc.h>
#include <machine/gdt.h>
#include <machine/psl.h>

#include <dev/ic/i8253reg.h>
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/apm/apmvar.h>
#include <i386/isa/nvram.h>

#include <machine/bioscall.h>
#ifdef APM_USE_KVM86
#include <machine/kvm86.h>
#endif
#include <machine/apmvar.h>

#if defined(APMDEBUG)
#define	DPRINTF(f, x)		do { if (apmdebug & (f)) printf x; } while (0)
#else
#define	DPRINTF(f, x)
#endif

static void	apmbiosattach(struct device *, struct device *, void *);
static int	apmbiosmatch(struct device *, struct cfdata *, void *);

#if 0
static void	apm_devpowmgt_enable(int, u_int);
#endif
static void	apm_disconnect(void *);
static void	apm_enable(void *, int);
static int	apm_get_powstat(void *, u_int, struct apm_power_info *);
static int	apm_get_event(void *, u_int *, u_int *);
static void	apm_cpu_busy(void *);
static void	apm_cpu_idle(void *);
static void	apm_get_capabilities(void *, u_int *, u_int *);

struct apm_connect_info apminfo;

static struct apm_accessops apm_accessops = {
	apm_disconnect,
	apm_enable,
	apm_set_powstate,
	apm_get_powstat,
	apm_get_event,
	apm_cpu_busy,
	apm_cpu_idle,
	apm_get_capabilities,
};

extern int apmdebug;
extern int apm_enabled;
extern int apm_force_64k_segments;
extern int apm_allow_bogus_segments;
extern int apm_bogus_bios;
extern int apm_minver;
extern int apm_inited;
extern int apm_do_idle;
extern int apm_v12_enabled;
extern int apm_v11_enabled;

static void	apm_perror(const char *, struct bioscallregs *, ...)
		    __attribute__((__format__(__printf__,1,3)));
static void	apm_powmgt_enable(int);
static void	apm_powmgt_engage(int, u_int);
static int	apm_get_ver(struct apm_softc *);

CFATTACH_DECL(apmbios, sizeof(struct apm_softc),
    apmbiosmatch, apmbiosattach, NULL, NULL);

#ifdef APMDEBUG
int	apmcall_debug(int, struct bioscallregs *, int);
static	void acallpr(int, const char *, struct bioscallregs *);

/* bitmask defns for printing apm call args/results */
#define ACPF_AX		0x00000001
#define ACPF_AX_HI	0x00000002
#define ACPF_EAX	0x00000004
#define ACPF_BX 	0x00000008
#define ACPF_BX_HI 	0x00000010
#define ACPF_EBX 	0x00000020
#define ACPF_CX 	0x00000040
#define ACPF_CX_HI 	0x00000080
#define ACPF_ECX 	0x00000100
#define ACPF_DX 	0x00000200
#define ACPF_DX_HI 	0x00000400
#define ACPF_EDX 	0x00000800
#define ACPF_SI 	0x00001000
#define ACPF_SI_HI 	0x00002000
#define ACPF_ESI 	0x00004000
#define ACPF_DI 	0x00008000
#define ACPF_DI_HI 	0x00010000
#define ACPF_EDI 	0x00020000
#define ACPF_FLAGS 	0x00040000
#define ACPF_FLAGS_HI	0x00080000
#define ACPF_EFLAGS 	0x00100000

struct acallinfo {
	const char *name;
	int inflag;
	int outflag;
};

static struct acallinfo aci[] = {
  { "install_check", ACPF_BX, ACPF_AX|ACPF_BX|ACPF_CX },
  { "connectreal", ACPF_BX, 0 },
  { "connect16", ACPF_BX, ACPF_AX|ACPF_BX|ACPF_CX|ACPF_SI|ACPF_DI },
  { "connect32", ACPF_BX, ACPF_AX|ACPF_EBX|ACPF_CX|ACPF_DX|ACPF_ESI|ACPF_DI },
  { "disconnect", ACPF_BX, 0 },
  { "cpu_idle", 0, 0 },
  { "cpu_busy", 0, 0 },
  { "set_power_state", ACPF_BX|ACPF_CX, 0 },
  { "enable_power_state", ACPF_BX|ACPF_CX, 0 }, 
  { "restore_defaults", ACPF_BX, 0 },
  { "get_power_status", ACPF_BX, ACPF_BX|ACPF_CX|ACPF_DX|ACPF_SI },
  { "get_event", 0, ACPF_BX|ACPF_CX },
  { "get_power_state" , ACPF_BX, ACPF_CX }, 
  { "enable_dev_power_mgt", ACPF_BX|ACPF_CX, 0 },
  { "driver_version", ACPF_BX|ACPF_CX, ACPF_AX },
  { "engage_power_mgt",  ACPF_BX|ACPF_CX, 0 },
  { "get_caps", ACPF_BX, ACPF_BX|ACPF_CX },
  { "resume_timer", ACPF_BX|ACPF_CX|ACPF_SI|ACPF_DI, ACPF_CX|ACPF_SI|ACPF_DI },
  { "resume_ring", ACPF_BX|ACPF_CX, ACPF_CX },
  { "timer_reqs", ACPF_BX|ACPF_CX, ACPF_CX },
};

static void acallpr(int flag, const char *tag, struct bioscallregs *b) {
  if (!flag) return;
  printf("%s ", tag);
  if (flag & ACPF_AX) 		printf("ax=%#x ", b->AX);
  if (flag & ACPF_AX_HI) 	printf("ax_hi=%#x ", b->AX_HI);
  if (flag & ACPF_EAX) 		printf("eax=%#x ", b->EAX);
  if (flag & ACPF_BX ) 		printf("bx=%#x ", b->BX);
  if (flag & ACPF_BX_HI ) 	printf("bx_hi=%#x ", b->BX_HI);
  if (flag & ACPF_EBX ) 	printf("ebx=%#x ", b->EBX);
  if (flag & ACPF_CX ) 		printf("cx=%#x ", b->CX);
  if (flag & ACPF_CX_HI ) 	printf("cx_hi=%#x ", b->CX_HI);
  if (flag & ACPF_ECX ) 	printf("ecx=%#x ", b->ECX);
  if (flag & ACPF_DX ) 		printf("dx=%#x ", b->DX);
  if (flag & ACPF_DX_HI ) 	printf("dx_hi=%#x ", b->DX_HI);
  if (flag & ACPF_EDX ) 	printf("edx=%#x ", b->EDX);
  if (flag & ACPF_SI ) 		printf("si=%#x ", b->SI);
  if (flag & ACPF_SI_HI ) 	printf("si_hi=%#x ", b->SI_HI);
  if (flag & ACPF_ESI ) 	printf("esi=%#x ", b->ESI);
  if (flag & ACPF_DI ) 		printf("di=%#x ", b->DI);
  if (flag & ACPF_DI_HI ) 	printf("di_hi=%#x ", b->DI_HI);
  if (flag & ACPF_EDI ) 	printf("edi=%#x ", b->EDI);
  if (flag & ACPF_FLAGS ) 	printf("flags=%#x ", b->FLAGS);
  if (flag & ACPF_FLAGS_HI) 	printf("flags_hi=%#x ", b->FLAGS_HI);
  if (flag & ACPF_EFLAGS ) 	printf("eflags=%#x ", b->EFLAGS);
}

int
apmcall_debug(int func, struct bioscallregs *regs, int line)
{
	int rv;
	int print = (apmdebug & APMDEBUG_APMCALLS) != 0;
	const char *name;
	int inf;
	int outf = 0; /* XXX: gcc */
		
	if (print) {
		if (func >= sizeof(aci) / sizeof(aci[0])) {
			name = 0;
			inf = outf = 0;
		} else {
			name = aci[func].name;
			inf = aci[func].inflag;
			outf = aci[func].outflag;
		}
		inittodr(time_second);	/* update timestamp */
		if (name)
			printf("apmcall@%03ld: %s/%#x (line=%d) ", 
				time_second % 1000, name, func, line);
		else
			printf("apmcall@%03ld: %#x (line=%d) ", 
				time_second % 1000, func, line);
		acallpr(inf, "in:", regs);
	}
    	rv = apmcall(func, regs);
	if (print) {
		if (rv) {
			printf(" => error %#x (%s)\n", regs->AX >> 8,
				apm_strerror(regs->AX >> 8));
		} else {
			printf(" => ");
			acallpr(outf, "out:", regs);
			printf("\n");
		}
	}
	return (rv);
}

#define apmcall(f, r)	apmcall_debug((f), (r), __LINE__)
#endif	/* APMDEBUG */

static void
apm_perror(const char *str, struct bioscallregs *regs, ...) /* XXX cgd */
{
	va_list ap;

	printf("APM ");

	va_start(ap, regs);
	vprintf(str, ap);			/* XXX cgd */
	va_end(ap);

	printf(": %s (0x%x)\n", apm_strerror(APM_ERR_CODE(regs)), regs->AX);
}

static void
apm_powmgt_enable(int onoff)
{
	struct bioscallregs regs;

	regs.BX = apm_minver == 0 ? APM_MGT_ALL : APM_DEV_ALLDEVS;
	regs.CX = onoff ? APM_MGT_ENABLE : APM_MGT_DISABLE;
	if (apmcall(APM_PWR_MGT_ENABLE, &regs) != 0)
		apm_perror("power management enable all <%s>", &regs,
		    onoff ? "enable" : "disable");
}

static void
apm_powmgt_engage(int onoff, u_int dev)
{
	struct bioscallregs regs;

	if (apm_minver == 0)
		return;
	regs.BX = dev;
	regs.CX = onoff ? APM_MGT_ENGAGE : APM_MGT_DISENGAGE;
	if (apmcall(APM_PWR_MGT_ENGAGE, &regs) != 0)
		apm_perror("power mgmt engage (device %x)", &regs, dev);
}

#if 0
static void
apm_devpowmgt_enable(int onoff, u_int dev)
{
	struct bioscallregs regs;

	if (apm_minver == 0)
	    return;
	regs.BX = dev;

	/*
	 * enable is auto BIOS management.
	 * disable is program control.
	 */
	regs.CX = onoff ? APM_MGT_ENABLE : APM_MGT_DISABLE;
	if (apmcall(APM_DEVICE_MGMT_ENABLE, &regs) != 0)
		printf("APM device engage (device %x): %s (%d)\n",
		    dev, apm_strerror(APM_ERR_CODE(&regs)),
		    APM_ERR_CODE(&regs));
}
#endif


static int
apm_get_ver(struct apm_softc *self)
{
	struct bioscallregs regs;

	regs.CX = 0x0102;	/* APM Version 1.2 */
	regs.BX = APM_DEV_APM_BIOS;
	
	if (apm_v12_enabled && apmcall(APM_DRIVER_VERSION, &regs) == 0)
		return 0x0102;

	regs.CX = 0x0101;	/* APM Version 1.1 */
	regs.BX = APM_DEV_APM_BIOS;
	
	if (apm_v11_enabled && apmcall(APM_DRIVER_VERSION, &regs) == 0)
		return 0x0101;
	else
		return 0x0100;
}

static int
apm_get_powstat(void *c, u_int batteryid, struct apm_power_info *pi)
{
	struct bioscallregs regs;
	int error;
	u_int nbattery = 0, caps;

	if (apm_minver >= 2) {
		apm_get_capabilities(&regs, &nbattery, &caps);
		if (batteryid > nbattery)
			return EIO;
	} else {
		if (batteryid > 0)
			return EIO;
		nbattery = 0;
	}

	if (batteryid == 0)
		regs.BX = APM_DEV_ALLDEVS;
	else
		regs.BX = APM_DEV_BATTERY(batteryid);

	if ((error = apmcall(APM_POWER_STATUS, &regs)) != 0)
		return regs.AX >> 8;

	pi->batteryid = batteryid;
	pi->nbattery = nbattery;
	pi->battery_life = APM_BATT_LIFE(&regs);
	pi->ac_state = APM_AC_STATE(&regs);
	pi->battery_state = APM_BATT_STATE(&regs);
	pi->battery_flags = APM_BATT_FLAGS(&regs);
	pi->minutes_valid = APM_BATT_REM_VALID(&regs);
	if (pi->minutes_valid)
		pi->minutes_left = APM_BATT_REMAINING(&regs);
	else
		pi->minutes_left = 0;
	return 0;
}

/* XXX cgd: this doesn't belong here. */
#define I386_FLAGBITS "\020\017NT\014OVFL\0130UP\012IEN\011TF\010NF\007ZF\005AF\003PF\001CY"

int
apm_busprobe(void)
{
	struct bioscallregs regs;
#ifdef APM_USE_KVM86
	int res;
#endif
#ifdef APMDEBUG
	char bits[128];
#endif

	memset(&regs, 0, sizeof(struct bioscallregs));
	regs.AX = APM_BIOS_FN(APM_INSTALLATION_CHECK);
	regs.BX = APM_DEV_APM_BIOS;
#ifdef APM_USE_KVM86
	res = kvm86_bioscall_simple(APM_SYSTEM_BIOS, &regs);
	if (res) {
		printf("apm_busprobe: kvm86 error\n");
		return (0);
	}
#else
	bioscall(APM_SYSTEM_BIOS, &regs);
#endif
	DPRINTF(APMDEBUG_PROBE, ("apm: bioscall return: %x %x %x %x %s %x %x\n",
	    regs.AX, regs.BX, regs.CX, regs.DX,
	    bitmask_snprintf(regs.EFLAGS, I386_FLAGBITS, bits, sizeof(bits)),
	    regs.ESI, regs.EDI));

	if (regs.FLAGS & PSL_C) {
		DPRINTF(APMDEBUG_PROBE, ("apm: carry set means no APM bios\n"));
		return 0;	/* no carry -> not installed */
	}
	if (regs.BX != APM_INSTALL_SIGNATURE) {
		DPRINTF(APMDEBUG_PROBE, ("apm: PM signature not found\n"));
		return 0;
	}
	if ((regs.CX & APM_32BIT_SUPPORT) == 0) {
		DPRINTF(APMDEBUG_PROBE, ("apm: no 32bit support (busprobe)\n"));
		return 0;
	}

	return 1;  /* OK to continue probe & complain if something fails */
}

static int
apmbiosmatch(struct device *parent, struct cfdata *match,
	     void *aux)
{
	/* There can be only one! */
	if (apm_inited)
		return 0;

	/*
	 * apm_busprobe() said 'go' or we wouldn't be here.
	 * APM might not be useful (or might be too weird)
	 * on this machine, but that's handled in attach.
	 *
	 * The apm_enabled global variable is used to allow
	 * users to patch kernels to disable APM support.
	 */
	if (apm_enabled) {
		if (apm_match() == 0) {
			apm_disconnect(NULL);
			return 0;
		} else
			return 1;
	}
	return 0;
}

#define	DPRINTF_BIOSRETURN(regs, bits)					\
	DPRINTF(APMDEBUG_ATTACH,					\
	    ("bioscall return: %x %x %x %x %s %x %x",			\
	    (regs).EAX, (regs).EBX, (regs).ECX, (regs).EDX,		\
	    bitmask_snprintf((regs).EFLAGS, I386_FLAGBITS,		\
	    (bits), sizeof(bits)), (regs).ESI, (regs).EDI))

static void
apmbiosattach(struct device *parent, struct device *self,
	      void *aux)
{
	struct apm_softc *apmsc = (void *)self;
	struct bioscallregs regs;
	int apm_data_seg_ok;
	u_int okbases[] = { 0, biosbasemem*1024 };
	u_int oklimits[] = { PAGE_SIZE, IOM_END};
	u_int i;
	int vers;
#ifdef APM_USE_KVM86
	int res;
#endif
#ifdef APMDEBUG
	char bits[128];
#endif

	aprint_naive(": Power management\n");
	aprint_normal(": Advanced Power Management BIOS");

	memset(&regs, 0, sizeof(struct bioscallregs));
	regs.AX = APM_BIOS_FN(APM_INSTALLATION_CHECK);
	regs.BX = APM_DEV_APM_BIOS;
#ifdef APM_USE_KVM86
	res = kvm86_bioscall_simple(APM_SYSTEM_BIOS, &regs);
	if (res) {
		aprint_error("%s: kvm86 error (APM_INSTALLATION_CHECK)\n",
		    apmsc->sc_dev.dv_xname);
		goto bail_disconnected;
	}
#else
	bioscall(APM_SYSTEM_BIOS, &regs);
#endif
	DPRINTF_BIOSRETURN(regs, bits);
	DPRINTF(APMDEBUG_ATTACH, ("\n%s: ", apmsc->sc_dev.dv_xname));

	apminfo.apm_detail = (u_int)regs.AX | ((u_int)regs.CX << 16);

	/*
	 * call a disconnect in case it was already connected
	 * by some previous code.
	 */
	memset(&regs, 0, sizeof(struct bioscallregs));
	regs.AX = APM_BIOS_FN(APM_DISCONNECT);
	regs.BX = APM_DEV_APM_BIOS;
#ifdef APM_USE_KVM86
	res = kvm86_bioscall_simple(APM_SYSTEM_BIOS, &regs);
	if (res) {
		aprint_error("%s: kvm86 error (APM_DISCONNECT)\n",
		    apmsc->sc_dev.dv_xname);
		goto bail_disconnected;
	}
#else
	bioscall(APM_SYSTEM_BIOS, &regs);
#endif
	DPRINTF_BIOSRETURN(regs, bits);
	DPRINTF(APMDEBUG_ATTACH, ("\n%s: ", apmsc->sc_dev.dv_xname));

	if ((apminfo.apm_detail & APM_32BIT_SUPPORTED) == 0) {
		aprint_error("%s: no 32-bit APM support\n",
		    apmsc->sc_dev.dv_xname);
		goto bail_disconnected;
	}

	/*
	 * And connect to it.
	 */
	memset(&regs, 0, sizeof(struct bioscallregs));
	regs.AX = APM_BIOS_FN(APM_32BIT_CONNECT);
	regs.BX = APM_DEV_APM_BIOS;
#ifdef APM_USE_KVM86
	res = kvm86_bioscall_simple(APM_SYSTEM_BIOS, &regs);
	if (res) {
		aprint_error("%s: kvm86 error (APM_32BIT_CONNECT)\n",
		    apmsc->sc_dev.dv_xname);
		goto bail_disconnected;
	}
#else
	bioscall(APM_SYSTEM_BIOS, &regs);
#endif
	DPRINTF_BIOSRETURN(regs, bits);
	DPRINTF(APMDEBUG_ATTACH, ("\n%s: ", apmsc->sc_dev.dv_xname));

	apminfo.apm_code32_seg_base = regs.AX << 4;
	apminfo.apm_entrypt = regs.BX; /* spec says EBX, can't map >=64k */
	apminfo.apm_code16_seg_base = regs.CX << 4;
	apminfo.apm_data_seg_base = regs.DX << 4;
	apminfo.apm_code32_seg_len = regs.SI;
	apminfo.apm_code16_seg_len = regs.SI_HI;
	apminfo.apm_data_seg_len = regs.DI;

	vers = (APM_MAJOR_VERS(apminfo.apm_detail) << 8) +
	    APM_MINOR_VERS(apminfo.apm_detail);
	if (apm_force_64k_segments) {
		apminfo.apm_code32_seg_len = 65536;
		apminfo.apm_code16_seg_len = 65536;
		apminfo.apm_data_seg_len = 65536;
	} else {
		switch (vers) {
		case 0x0100:
			apminfo.apm_code32_seg_len = 65536;
			apminfo.apm_code16_seg_len = 65536;
			apminfo.apm_data_seg_len = 65536;
			apm_v11_enabled = 0;
			apm_v12_enabled = 0;
			break;
		case 0x0101:
			apminfo.apm_code16_seg_len = apminfo.apm_code32_seg_len;
			apm_v12_enabled = 0;
			/* fall through */
		case 0x0102:
		default:
			if (apminfo.apm_code32_seg_len == 0) {
				/*
				 * some BIOSes are lame, even if v1.1.
				 * (Or maybe they want 64k even though they can
				 * only ask for 64k-1?)
				 */
				apminfo.apm_code32_seg_len = 65536;
				DPRINTF(APMDEBUG_ATTACH,
				    ("lame v%d.%d bios gave zero len code32, pegged to 64k\n%s: ",
				    APM_MAJOR_VERS(apminfo.apm_detail),
				    APM_MINOR_VERS(apminfo.apm_detail),
				    apmsc->sc_dev.dv_xname));
			}
			if (apminfo.apm_code16_seg_len == 0) {
				/*
				 * some BIOSes are lame, even if v1.1.
				 * (Or maybe they want 64k even though they can
				 * only ask for 64k-1?)
				 */
				apminfo.apm_code16_seg_len = 65536;
				DPRINTF(APMDEBUG_ATTACH,
				    ("lame v%d.%d bios gave zero len code16, pegged to 64k\n%s: ",
				    APM_MAJOR_VERS(apminfo.apm_detail),
				    APM_MINOR_VERS(apminfo.apm_detail),
				    apmsc->sc_dev.dv_xname));
			}
			if (apminfo.apm_data_seg_len == 0) {
				/*
				 * some BIOSes are lame, even if v1.1.
				 *
				 * leave it alone and assume it does not
				 * want any sensible data segment
				 * mapping, and mark as bogus (but with
				 * expanded size, in case it's in some place
				 * that costs us nothing to map).
				 */
				apm_bogus_bios = 1;
				apminfo.apm_data_seg_len = 65536;
				DPRINTF(APMDEBUG_ATTACH,
				    ("lame v%d.%d bios gave zero len data, tentative 64k\n%s: ",
				    APM_MAJOR_VERS(apminfo.apm_detail),
				    APM_MINOR_VERS(apminfo.apm_detail),
				    apmsc->sc_dev.dv_xname));
			}
			break;
		}
	}
	if (apminfo.apm_code32_seg_len < apminfo.apm_entrypt + 4) {
		DPRINTF(APMDEBUG_ATTACH,
		    ("nonsensical BIOS code length %d ignored (entry point offset is %d)\n%s: ",
		    apminfo.apm_code32_seg_len,
		    apminfo.apm_entrypt,
		    apmsc->sc_dev.dv_xname));
		apminfo.apm_code32_seg_len = 65536;
	}
	if (apminfo.apm_code32_seg_base < IOM_BEGIN ||
	    apminfo.apm_code32_seg_base >= IOM_END) {
		DPRINTF(APMDEBUG_ATTACH, ("code32 segment starts outside ISA hole [%x]\n%s: ",
		    apminfo.apm_code32_seg_base, apmsc->sc_dev.dv_xname));
		aprint_error("%s: bogus 32-bit code segment start\n",
		    apmsc->sc_dev.dv_xname);
		goto bail;
	} 
	if (apminfo.apm_code32_seg_base +
	    apminfo.apm_code32_seg_len > IOM_END) {
		DPRINTF(APMDEBUG_ATTACH, ("code32 segment oversized: [%x,%x)\n%s: ",
		    apminfo.apm_code32_seg_base,
		    apminfo.apm_code32_seg_base + apminfo.apm_code32_seg_len - 1,
		    apmsc->sc_dev.dv_xname));
#if 0
		aprint_error("%s: bogus 32-bit code segment size\n",
		    apmsc->sc_dev.dv_xname);
		goto bail;
#else
		apminfo.apm_code32_seg_len =
		    IOM_END - apminfo.apm_code32_seg_base;
#endif
	}
	if (apminfo.apm_code16_seg_base < IOM_BEGIN ||
	    apminfo.apm_code16_seg_base >= IOM_END) {
		DPRINTF(APMDEBUG_ATTACH, ("code16 segment starts outside ISA hole [%x]\n%s: ",
		    apminfo.apm_code16_seg_base, apmsc->sc_dev.dv_xname));
		aprint_error("%s: bogus 16-bit code segment start\n",
		    apmsc->sc_dev.dv_xname);
		goto bail;
	}
	if (apminfo.apm_code16_seg_base +
	    apminfo.apm_code16_seg_len > IOM_END) {
		DPRINTF(APMDEBUG_ATTACH,
		    ("code16 segment oversized: [%x,%x), giving up\n%s: ",
		    apminfo.apm_code16_seg_base,
		    apminfo.apm_code16_seg_base + apminfo.apm_code16_seg_len - 1,
		    apmsc->sc_dev.dv_xname));
		/*
		 * give up since we may have to trash the
		 * 32bit segment length otherwise.
		 */
		aprint_error("%s: bogus 16-bit code segment size\n",
		    apmsc->sc_dev.dv_xname);
		goto bail;
	}
	/*
	 * allow data segment to be zero length, within ISA hole or
	 * at page zero or above biosbasemem and below ISA hole end.
	 * truncate it if it doesn't quite fit in the space
	 * we allow.
	 *
	 * Otherwise, give up if not "apm_bogus_bios".
	 */
	apm_data_seg_ok = 0;
	for (i = 0; i < 2; i++) {
		if (apminfo.apm_data_seg_base >= okbases[i] &&
		    apminfo.apm_data_seg_base < oklimits[i]-1) {
			/* starts OK */
			if (apminfo.apm_data_seg_base +
			    apminfo.apm_data_seg_len > oklimits[i]) {
				DPRINTF(APMDEBUG_ATTACH,
				    ("data segment oversized: [%x,%x)",
				    apminfo.apm_data_seg_base,
				    apminfo.apm_data_seg_base + apminfo.apm_data_seg_len));
				apminfo.apm_data_seg_len =
				    oklimits[i] - apminfo.apm_data_seg_base;
				DPRINTF(APMDEBUG_ATTACH,
				    ("; resized to [%x,%x)\n%s: ",
				    apminfo.apm_data_seg_base,
				    apminfo.apm_data_seg_base + apminfo.apm_data_seg_len,
				    apmsc->sc_dev.dv_xname));
			} else {
				DPRINTF(APMDEBUG_ATTACH,
				    ("data segment fine: [%x,%x)\n%s: ",
				    apminfo.apm_data_seg_base,
				    apminfo.apm_data_seg_base + apminfo.apm_data_seg_len,
				    apmsc->sc_dev.dv_xname));
			}
			apm_data_seg_ok = 1;
			break;
		}
	}
	if (!apm_data_seg_ok && apm_bogus_bios) {
		if (apm_allow_bogus_segments) {
			DPRINTF(APMDEBUG_ATTACH,
			    ("bogus bios data seg location, continuing\n%s: ",
			    apmsc->sc_dev.dv_xname));
		} else {
			DPRINTF(APMDEBUG_ATTACH,
			    ("bogus bios data seg location, ignoring\n%s: ",
			    apmsc->sc_dev.dv_xname));
			apminfo.apm_data_seg_base = 0;
			apminfo.apm_data_seg_len = 0;
		}
		apm_data_seg_ok = 1;		/* who are we kidding?! */
	}
	if (!apm_data_seg_ok) {
		DPRINTF(APMDEBUG_ATTACH,
		    ("data segment [%x,%x) not in an available location\n%s: ",
		    apminfo.apm_data_seg_base,
		    apminfo.apm_data_seg_base + apminfo.apm_data_seg_len,
		    apmsc->sc_dev.dv_xname));
		aprint_error("%s: data segment unavailable\n",
		    apmsc->sc_dev.dv_xname);
		goto bail;
	}

	/*
	 * set up GDT descriptors for APM
	 */
	apminfo.apm_segsel = GSEL(GAPM32CODE_SEL,SEL_KPL);

	/*
	 * Some bogus APM V1.1 BIOSes do not return any
	 * size limits in the registers they are supposed to.
	 * We forced them to zero before calling the BIOS
	 * (see apm_init.S), so if we see zero limits here
	 * we assume that means they should be 64k (and trimmed
	 * if needed for legitimate memory needs).
	 */
	DPRINTF(APMDEBUG_ATTACH, ("code32len=%x, datalen=%x\n%s: ",
	    apminfo.apm_code32_seg_len,
	    apminfo.apm_data_seg_len,
	    apmsc->sc_dev.dv_xname));
	setgdt(GAPM32CODE_SEL, ISA_HOLE_VADDR(apminfo.apm_code32_seg_base),
	    apminfo.apm_code32_seg_len - 1,
	    SDT_MEMERA, SEL_KPL, 1, 0);
#ifdef GAPM16CODE_SEL
	setgdt(GAPM16CODE_SEL, ISA_HOLE_VADDR(apminfo.apm_code16_seg_base),
	    apminfo.apm_code16_seg_len - 1,
	    SDT_MEMERA, SEL_KPL, 0, 0);
#endif
	if (apminfo.apm_data_seg_len == 0) {
		/*
		 *if no data area needed, set up the segment
		 * descriptor to just the first byte of the code
		 * segment, read only.
		 */
		setgdt(GAPMDATA_SEL,
		    ISA_HOLE_VADDR(apminfo.apm_code32_seg_base),
		    0, SDT_MEMROA, SEL_KPL, 0, 0);
	} else if (apminfo.apm_data_seg_base < IOM_BEGIN) {
		bus_space_handle_t memh;

		/*
		 * need page zero or biosbasemem area mapping.
		 *
		 * XXX cheat and use knowledge of bus_space_map()
		 * implementation on i386 so it can be done without
		 * extent checking.
		 */
		if (_x86_memio_map(X86_BUS_SPACE_MEM,
		    apminfo.apm_data_seg_base,
		    apminfo.apm_data_seg_len, 0, &memh)) {
			aprint_error("%s: couldn't map data segment\n",
			    apmsc->sc_dev.dv_xname);
			goto bail;
		}
		DPRINTF(APMDEBUG_ATTACH,
		    ("mapping bios data area %x @ 0x%lx\n%s: ",
		    apminfo.apm_data_seg_base, memh,
		    apmsc->sc_dev.dv_xname));
		setgdt(GAPMDATA_SEL, (void *)memh,
		    apminfo.apm_data_seg_len - 1,
		    SDT_MEMRWA, SEL_KPL, 1, 0);
	} else
		setgdt(GAPMDATA_SEL, ISA_HOLE_VADDR(apminfo.apm_data_seg_base),
		    apminfo.apm_data_seg_len - 1,
		    SDT_MEMRWA, SEL_KPL, 1, 0);

	DPRINTF(APMDEBUG_ATTACH,
	    ("detail %x 32b:%x/%p/%x 16b:%x/%p/%x data %x/%p/%x ep %x (%x:%p) %p\n%s: ",
	    apminfo.apm_detail,
	    apminfo.apm_code32_seg_base,
	    ISA_HOLE_VADDR(apminfo.apm_code32_seg_base),
	    apminfo.apm_code32_seg_len,
	    apminfo.apm_code16_seg_base,
	    ISA_HOLE_VADDR(apminfo.apm_code16_seg_base),
	    apminfo.apm_code16_seg_len,
	    apminfo.apm_data_seg_base,
	    ISA_HOLE_VADDR(apminfo.apm_data_seg_base),
	    apminfo.apm_data_seg_len,
	    apminfo.apm_entrypt,
	    apminfo.apm_segsel,
	    apminfo.apm_entrypt +
	     (char *)ISA_HOLE_VADDR(apminfo.apm_code32_seg_base),
	    &apminfo.apm_segsel,
	    apmsc->sc_dev.dv_xname));

	apmsc->sc_ops = &apm_accessops;
	apmsc->sc_cookie = apmsc;
	apmsc->sc_vers = apm_get_ver(apmsc);
	apmsc->sc_detail = apminfo.apm_detail;
	apmsc->sc_hwflags = 0;
	apm_attach(apmsc);

	return;

bail:
	/*
	 * call a disconnect; we're punting.
	 */
	apm_disconnect(apmsc);
bail_disconnected:
	aprint_normal("%s: kernel APM support disabled\n",
	    apmsc->sc_dev.dv_xname);
}

static void
apm_enable(void *sc, int on)
{
	/*
	 * XXX some bogus APM BIOSes don't set the disabled bit in
	 * the connect state, yet still complain about the functions
	 * being disabled when other calls are made.  sigh.
	 */
	if (apminfo.apm_detail & APM_BIOS_PM_DISABLED)
		apm_powmgt_enable(on);

	/*
	 * Engage cooperative power mgt (we get to do it)
	 * on all devices (v1.1).
	 */
	apm_powmgt_engage(on, APM_DEV_ALLDEVS);
}

void
apm_disconnect(void *arg)
{
	struct apm_softc *sc = arg;
	struct bioscallregs regs;
#ifdef APMDEBUG
	char bits[128];
#endif
	/*
	 * We were unable to create the APM thread; bail out.
	 */
	memset(&regs, 0, sizeof(struct bioscallregs));
	regs.AX = APM_BIOS_FN(APM_DISCONNECT);
	regs.BX = APM_DEV_APM_BIOS;
#ifdef APM_USE_KVM86
	(void)kvm86_bioscall_simple(APM_SYSTEM_BIOS, &regs);
#else
	bioscall(APM_SYSTEM_BIOS, &regs);
#endif
	if (sc == NULL)
		return;
	DPRINTF(APMDEBUG_ATTACH, ("\n%s: ", sc->sc_dev.dv_xname));
	DPRINTF_BIOSRETURN(regs, bits);
	printf("%s: unable to create thread, kernel APM support disabled\n",
	    sc->sc_dev.dv_xname);
}

static int
apm_get_event(void *sc, u_int *event_code, u_int *event_info)
{
	int error;
	struct bioscallregs regs;

	if ((error = apmcall(APM_GET_PM_EVENT, &regs)) != 0)
		return regs.AX >> 8;
	*event_code = regs.BX;
	*event_info = regs.AX;
	return 0;
}

int
apm_set_powstate(void *sc, u_int dev, u_int state)
{
	struct bioscallregs regs;
	if (!apm_inited || (apm_minver == 0 && state > APM_SYS_OFF))
		return EINVAL;

	regs.BX = dev;
	regs.CX = state;
	if (apmcall(APM_SET_PWR_STATE, &regs) != 0) {
		apm_perror("set power state <%x,%x>", &regs, dev, state);
		return regs.AX >> 8;
	}
	return 0;
}

static void
apm_cpu_busy(void *sc)
{
	struct bioscallregs regs;

	if (!apm_inited || !apm_do_idle)
	    return;
	if ((apminfo.apm_detail & APM_IDLE_SLOWS) &&
	    apmcall(APM_CPU_BUSY, &regs) != 0) {
		/*
		 * XXX BIOSes use to set carry without valid
		 * error number
		 */
#ifdef APMDEBUG
		apm_perror("set CPU busy", &regs);
#endif
	}
}

static void
apm_cpu_idle(void *sc)
{
	struct bioscallregs regs;

	if (!apm_inited || !apm_do_idle)
	    return;
	if (apmcall(APM_CPU_IDLE, &regs) != 0) {
		/*
		 * XXX BIOSes use to set carry without valid
		 * error number
		 */
#ifdef APMDEBUG
		apm_perror("set CPU idle", &regs);
#endif
	}
}

/* V1.2 */
static void
apm_get_capabilities(void *sc, u_int *numbatts, u_int *capflags)
{
	struct bioscallregs regs;
	int error;

	regs.BX = APM_DEV_APM_BIOS;
	if ((error = apmcall(APM_GET_CAPABILITIES, &regs)) != 0) {
		apm_perror("get capabilities", &regs);
		return;
	}
	*capflags = regs.CX;
	*numbatts = APM_NBATTERIES(&regs);

#ifdef APMDEBUG
	/* print out stats */
	DPRINTF(APMDEBUG_INFO, ("apm: %d batteries", *numbatts));
	if (*capflags & APM_GLOBAL_STANDBY)
	    DPRINTF(APMDEBUG_INFO, (", global standby"));
	if (*capflags & APM_GLOBAL_SUSPEND)
	    DPRINTF(APMDEBUG_INFO, (", global suspend"));
	if (*capflags & APM_RTIMER_STANDBY)
	    DPRINTF(APMDEBUG_INFO, (", rtimer standby"));
	if (*capflags & APM_RTIMER_SUSPEND)
	    DPRINTF(APMDEBUG_INFO, (", rtimer suspend"));
	if (*capflags & APM_IRRING_STANDBY)
	    DPRINTF(APMDEBUG_INFO, (", internal standby"));
	if (*capflags & APM_IRRING_SUSPEND)
	    DPRINTF(APMDEBUG_INFO, (", internal suspend"));
	if (*capflags & APM_PCRING_STANDBY)
	    DPRINTF(APMDEBUG_INFO, (", pccard standby"));
	if (*capflags & APM_PCRING_SUSPEND)
	    DPRINTF(APMDEBUG_INFO, (", pccard suspend"));
	DPRINTF(APMDEBUG_INFO, ("\n"));
#endif
}

#undef DPRINTF_BIOSRETURN
