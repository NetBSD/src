/*-
 * Copyright (c) 2002-2004 Sam Leffler, Errno Consulting, Atheros
 * Communications, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the following conditions are met:
 * 1. The materials contained herein are unmodified and are used
 *    unmodified.
 * 2. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following NO
 *    ''WARRANTY'' disclaimer below (''Disclaimer''), without
 *    modification.
 * 3. Redistributions in binary form must reproduce at minimum a
 *    disclaimer similar to the Disclaimer below and any redistribution
 *    must be conditioned upon including a substantially similar
 *    Disclaimer requirement for further binary redistribution.
 * 4. Neither the names of the above-listed copyright holders nor the
 *    names of any contributors may be used to endorse or promote
 *    product derived from this software without specific prior written
 *    permission.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT,
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES.
 *
 * $Id: ah_osdep.c,v 1.3 2004/06/09 16:33:48 samleffler Exp $
 */
#include "opt_ah.h"

#ifndef EXPORT_SYMTAB
#define	EXPORT_SYMTAB
#endif

#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/sysctl.h>
#include <linux/proc_fs.h>

#include <asm/io.h>

#include "ah.h"

#ifndef __MOD_INC_USE_COUNT
#define	__MOD_INC_USE_COUNT(_m)						\
	if (!try_module_get(_m)) {					\
		printk(KERN_WARNING "try_module_get failed\n");		\
		return NULL;						\
	}
#define	__MOD_DEC_USE_COUNT(_m)		module_put(_m)
#endif

#ifdef AH_DEBUG
static	int ath_hal_debug = 0;
#endif

int	ath_hal_dma_beacon_response_time = 2;	/* in TU's */
int	ath_hal_sw_beacon_response_time = 10;	/* in TU's */
int	ath_hal_additional_swba_backoff = 0;	/* in TU's */

struct ath_hal *
_ath_hal_attach(u_int16_t devid, HAL_SOFTC sc,
		HAL_BUS_TAG t, HAL_BUS_HANDLE h, void* s)
{
	HAL_STATUS status;
	struct ath_hal *ah = ath_hal_attach(devid, sc, t, h, &status);

	*(HAL_STATUS *)s = status;
	if (ah)
		__MOD_INC_USE_COUNT(THIS_MODULE);
	return ah;
}

void
ath_hal_detach(struct ath_hal *ah)
{
	(*ah->ah_detach)(ah);
	__MOD_DEC_USE_COUNT(THIS_MODULE);
}

/*
 * Print/log message support.
 */

void __ahdecl
ath_hal_vprintf(struct ath_hal *ah, const char* fmt, va_list ap)
{
	char buf[1024];					/* XXX */
	vsnprintf(buf, sizeof(buf), fmt, ap);
	printk("%s", buf);
}

void __ahdecl
ath_hal_printf(struct ath_hal *ah, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	ath_hal_vprintf(ah, fmt, ap);
	va_end(ap);
}

/*
 * Format an Ethernet MAC for printing.
 */
const char* __ahdecl
ath_hal_ether_sprintf(const u_int8_t *mac)
{
	static char etherbuf[18];
	snprintf(etherbuf, sizeof(etherbuf), "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return etherbuf;
}

#ifdef AH_ASSERT
void __ahdecl
ath_hal_assert_failed(const char* filename, int lineno, const char *msg)
{
	printk("Atheros HAL assertion failure: %s: line %u: %s\n",
		filename, lineno, msg);
	panic("ath_hal_assert");
}
#endif /* AH_ASSERT */

/*
 * Memory-mapped device register read/write.  These are here
 * as routines when debugging support is enabled and/or when
 * explicitly configured to use function calls.  The latter is
 * for architectures that might need to do something before
 * referencing memory (e.g. remap an i/o window).
 *
 * NB: see the comments in ah_osdep.h about byte-swapping register
 *     reads and writes to understand what's going on below.
 */
#if defined(AH_DEBUG) || defined(AH_REGOPS_FUNC)
void __ahdecl
ath_hal_reg_write(struct ath_hal *ah, u_int reg, u_int32_t val)
{
#ifdef AH_DEBUG
	if (ath_hal_debug > 1)
		ath_hal_printf(ah, "WRITE 0x%x <= 0x%x\n", reg, val);
#endif
 	if (reg >= 0x4000 && reg < 0x5000)
 		*((volatile u_int32_t *)(ah->ah_sh + reg)) = __bswap32(val);
 	else
 		*((volatile u_int32_t *)(ah->ah_sh + reg)) = val;
}

u_int32_t __ahdecl
ath_hal_reg_read(struct ath_hal *ah, u_int reg)
{
 	u_int32_t val;

	val = *((volatile u_int32_t *)(ah->ah_sh + reg));
 	if (reg >= 0x4000 && reg < 0x5000)
		val = __bswap32(val);
#ifdef AH_DEBUG
	if (ath_hal_debug > 1)
		ath_hal_printf(ah, "READ 0x%x => 0x%x\n", reg, val);
#endif
	return val;
}
#endif /* AH_DEBUG || AH_REGOPS_FUNC */

#ifdef AH_DEBUG
void __ahdecl
HALDEBUG(struct ath_hal *ah, const char* fmt, ...)
{
	if (ath_hal_debug) {
		__va_list ap;
		va_start(ap, fmt);
		ath_hal_vprintf(ah, fmt, ap);
		va_end(ap);
	}
}


void __ahdecl
HALDEBUGn(struct ath_hal *ah, u_int level, const char* fmt, ...)
{
	if (ath_hal_debug >= level) {
		__va_list ap;
		va_start(ap, fmt);
		ath_hal_vprintf(ah, fmt, ap);
		va_end(ap);
	}
}
#endif /* AH_DEBUG */

/*
 * Delay n microseconds.
 */
void __ahdecl
ath_hal_delay(int n)
{
	udelay(n);
}

u_int32_t __ahdecl
ath_hal_getuptime(struct ath_hal *ah)
{
	return ((jiffies / HZ) * 1000) + (jiffies % HZ) * (1000 / HZ);
}

/*
 * Allocate/free memory.
 */

void * __ahdecl
ath_hal_malloc(size_t size)
{
	void *p;
	p = kmalloc(size, GFP_KERNEL);
	if (p)
		OS_MEMZERO(p, size);
	return p;
		
}

void __ahdecl
ath_hal_free(void* p)
{
	kfree(p);
}

void * __ahdecl
ath_hal_memcpy(void *dst, const void *src, size_t n)
{
	return memcpy(dst, src, n);
}

#ifdef CONFIG_SYSCTL
enum {
	DEV_ATH		= 9,			/* XXX must match driver */
};

#define	CTL_AUTO	-2	/* cannot be CTL_ANY or CTL_NONE */

static ctl_table ath_hal_sysctls[] = {
#ifdef AH_DEBUG
	{ .ctl_name	= CTL_AUTO,
	   .procname	= "debug",
	  .mode		= 0644,
	  .data		= &ath_hal_debug,
	  .maxlen	= sizeof(ath_hal_debug),
	  .proc_handler	= proc_dointvec
	},
#endif
	{ .ctl_name	= CTL_AUTO,
	  .procname	= "dma_beacon_response_time",
	  .data		= &ath_hal_dma_beacon_response_time,
	  .maxlen	= sizeof(ath_hal_dma_beacon_response_time),
	  .mode		= 0644,
	  .proc_handler	= proc_dointvec
	},
	{ .ctl_name	= CTL_AUTO,	
	  .procname	= "sw_beacon_response_time",
	  .mode		= 0644,
	  .data		= &ath_hal_sw_beacon_response_time,
	  .maxlen	= sizeof(ath_hal_sw_beacon_response_time),
	  .proc_handler	= proc_dointvec
	},
	{ .ctl_name	= CTL_AUTO,
	  .procname	= "swba_backoff",
	  .mode		= 0644,
	  .data		= &ath_hal_additional_swba_backoff,
	  .maxlen	= sizeof(ath_hal_additional_swba_backoff),
	  .proc_handler	= proc_dointvec
	},
	{ 0 }
};
static ctl_table ath_hal_table[] = {
	{ .ctl_name	= CTL_AUTO,
	  .procname	= "hal",
	  .mode		= 0555,
	  .child	= ath_hal_sysctls
	}, { 0 }
};
static ctl_table ath_ath_table[] = {
	{ .ctl_name	= DEV_ATH,
	  .procname	= "ath",
	  .mode		= 0555,
	  .child	= ath_hal_table
	}, { 0 }
};
static ctl_table ath_root_table[] = {
	{ .ctl_name	= CTL_DEV,
	  .procname	= "dev",
	  .mode		= 0555,
	  .child	= ath_ath_table
	}, { 0 }
};
static struct ctl_table_header *ath_hal_sysctl_header;

void
ath_hal_sysctl_register(void)
{
	static int initialized = 0;

	if (!initialized) {
		ath_hal_sysctl_header =
			register_sysctl_table(ath_root_table, 1);
		initialized = 1;
	}
}

void
ath_hal_sysctl_unregister(void)
{
	if (ath_hal_sysctl_header)
		unregister_sysctl_table(ath_hal_sysctl_header);
}
#endif /* CONFIG_SYSCTL */

/*
 * Module glue.
 */
static char *dev_info = "ath_hal";

MODULE_AUTHOR("Errno Consulting, Sam Leffler");
MODULE_DESCRIPTION("Atheros Hardware Access Layer (HAL)");
MODULE_SUPPORTED_DEVICE("Atheros WLAN devices");
#ifdef MODULE_LICENSE
MODULE_LICENSE("Proprietary");
#endif

EXPORT_SYMBOL(ath_hal_probe);
EXPORT_SYMBOL(_ath_hal_attach);
EXPORT_SYMBOL(ath_hal_detach);
EXPORT_SYMBOL(ath_hal_init_channels);
EXPORT_SYMBOL(ath_hal_getwirelessmodes);
EXPORT_SYMBOL(ath_hal_computetxtime);
EXPORT_SYMBOL(ath_hal_mhz2ieee);
EXPORT_SYMBOL(ath_hal_ieee2mhz);

static int __init
init_ath_hal(void)
{
	printk(KERN_INFO "%s: %s\n", dev_info, ath_hal_version);
#ifdef CONFIG_SYSCTL
	ath_hal_sysctl_register();
#endif
	return (0);
}
module_init(init_ath_hal);

static void __exit
exit_ath_hal(void)
{
#ifdef CONFIG_SYSCTL
	ath_hal_sysctl_unregister();
#endif
	printk(KERN_INFO "%s: driver unloaded\n", dev_info);
}
module_exit(exit_ath_hal);
