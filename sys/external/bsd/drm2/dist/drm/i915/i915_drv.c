/* i915_drv.c -- i830,i845,i855,i865,i915 driver -*- linux-c -*-
 */
/*
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <linux/device.h>
#include <drm/drmP.h>
#include <drm/i915_drm.h>
#include "i915_drv.h"
#include "i915_trace.h"
#include "intel_drv.h"

#include <linux/console.h>
#include <linux/module.h>
#include <drm/drm_crtc_helper.h>

static int i915_modeset __read_mostly = -1;
module_param_named(modeset, i915_modeset, int, 0400);
MODULE_PARM_DESC(modeset,
		"Use kernel modesetting [KMS] (0=DRM_I915_KMS from .config, "
		"1=on, -1=force vga console preference [default])");

unsigned int i915_fbpercrtc __always_unused = 0;
module_param_named(fbpercrtc, i915_fbpercrtc, int, 0400);

int i915_panel_ignore_lid __read_mostly = 1;
module_param_named(panel_ignore_lid, i915_panel_ignore_lid, int, 0600);
MODULE_PARM_DESC(panel_ignore_lid,
		"Override lid status (0=autodetect, 1=autodetect disabled [default], "
		"-1=force lid closed, -2=force lid open)");

unsigned int i915_powersave __read_mostly = 1;
module_param_named(powersave, i915_powersave, int, 0600);
MODULE_PARM_DESC(powersave,
		"Enable powersavings, fbc, downclocking, etc. (default: true)");

int i915_semaphores __read_mostly = -1;
module_param_named(semaphores, i915_semaphores, int, 0600);
MODULE_PARM_DESC(semaphores,
		"Use semaphores for inter-ring sync (default: -1 (use per-chip defaults))");

int i915_enable_rc6 __read_mostly = -1;
module_param_named(i915_enable_rc6, i915_enable_rc6, int, 0400);
MODULE_PARM_DESC(i915_enable_rc6,
		"Enable power-saving render C-state 6. "
		"Different stages can be selected via bitmask values "
		"(0 = disable; 1 = enable rc6; 2 = enable deep rc6; 4 = enable deepest rc6). "
		"For example, 3 would enable rc6 and deep rc6, and 7 would enable everything. "
		"default: -1 (use per-chip default)");

int i915_enable_fbc __read_mostly = -1;
module_param_named(i915_enable_fbc, i915_enable_fbc, int, 0600);
MODULE_PARM_DESC(i915_enable_fbc,
		"Enable frame buffer compression for power savings "
		"(default: -1 (use per-chip default))");

unsigned int i915_lvds_downclock __read_mostly = 0;
module_param_named(lvds_downclock, i915_lvds_downclock, int, 0400);
MODULE_PARM_DESC(lvds_downclock,
		"Use panel (LVDS/eDP) downclocking for power savings "
		"(default: false)");

int i915_lvds_channel_mode __read_mostly;
module_param_named(lvds_channel_mode, i915_lvds_channel_mode, int, 0600);
MODULE_PARM_DESC(lvds_channel_mode,
		 "Specify LVDS channel mode "
		 "(0=probe BIOS [default], 1=single-channel, 2=dual-channel)");

int i915_panel_use_ssc __read_mostly = -1;
module_param_named(lvds_use_ssc, i915_panel_use_ssc, int, 0600);
MODULE_PARM_DESC(lvds_use_ssc,
		"Use Spread Spectrum Clock with panels [LVDS/eDP] "
		"(default: auto from VBT)");

int i915_vbt_sdvo_panel_type __read_mostly = -1;
module_param_named(vbt_sdvo_panel_type, i915_vbt_sdvo_panel_type, int, 0600);
MODULE_PARM_DESC(vbt_sdvo_panel_type,
		"Override/Ignore selection of SDVO panel mode in the VBT "
		"(-2=ignore, -1=auto [default], index in VBT BIOS table)");

static bool i915_try_reset __read_mostly = true;
module_param_named(reset, i915_try_reset, bool, 0600);
MODULE_PARM_DESC(reset, "Attempt GPU resets (default: true)");

bool i915_enable_hangcheck __read_mostly = true;
module_param_named(enable_hangcheck, i915_enable_hangcheck, bool, 0644);
MODULE_PARM_DESC(enable_hangcheck,
		"Periodically check GPU activity for detecting hangs. "
		"WARNING: Disabling this can cause system wide hangs. "
		"(default: true)");

int i915_enable_ppgtt __read_mostly = -1;
module_param_named(i915_enable_ppgtt, i915_enable_ppgtt, int, 0600);
MODULE_PARM_DESC(i915_enable_ppgtt,
		"Enable PPGTT (default: true)");

unsigned int i915_preliminary_hw_support __read_mostly = 0;
module_param_named(preliminary_hw_support, i915_preliminary_hw_support, int, 0600);
MODULE_PARM_DESC(preliminary_hw_support,
		"Enable preliminary hardware support. "
		"Enable Haswell and ValleyView Support. "
		"(default: false)");

static struct drm_driver driver;
extern int intel_agp_enabled;

#define INTEL_VGA_DEVICE(id, info) {		\
	.class = PCI_BASE_CLASS_DISPLAY << 16,	\
	.class_mask = 0xff0000,			\
	.vendor = 0x8086,			\
	.device = id,				\
	.subvendor = PCI_ANY_ID,		\
	.subdevice = PCI_ANY_ID,		\
	.driver_data = (unsigned long) info }

static const struct intel_device_info intel_i830_info = {
	.gen = 2, .is_mobile = 1, .cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
};

static const struct intel_device_info intel_845g_info = {
	.gen = 2,
	.has_overlay = 1, .overlay_needs_physical = 1,
};

static const struct intel_device_info intel_i85x_info = {
	.gen = 2, .is_i85x = 1, .is_mobile = 1,
	.cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
};

static const struct intel_device_info intel_i865g_info = {
	.gen = 2,
	.has_overlay = 1, .overlay_needs_physical = 1,
};

static const struct intel_device_info intel_i915g_info = {
	.gen = 3, .is_i915g = 1, .cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
};
static const struct intel_device_info intel_i915gm_info = {
	.gen = 3, .is_mobile = 1,
	.cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
	.supports_tv = 1,
};
static const struct intel_device_info intel_i945g_info = {
	.gen = 3, .has_hotplug = 1, .cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
};
static const struct intel_device_info intel_i945gm_info = {
	.gen = 3, .is_i945gm = 1, .is_mobile = 1,
	.has_hotplug = 1, .cursor_needs_physical = 1,
	.has_overlay = 1, .overlay_needs_physical = 1,
	.supports_tv = 1,
};

static const struct intel_device_info intel_i965g_info = {
	.gen = 4, .is_broadwater = 1,
	.has_hotplug = 1,
	.has_overlay = 1,
};

static const struct intel_device_info intel_i965gm_info = {
	.gen = 4, .is_crestline = 1,
	.is_mobile = 1, .has_fbc = 1, .has_hotplug = 1,
	.has_overlay = 1,
	.supports_tv = 1,
};

static const struct intel_device_info intel_g33_info = {
	.gen = 3, .is_g33 = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_overlay = 1,
};

static const struct intel_device_info intel_g45_info = {
	.gen = 4, .is_g4x = 1, .need_gfx_hws = 1,
	.has_pipe_cxsr = 1, .has_hotplug = 1,
	.has_bsd_ring = 1,
};

static const struct intel_device_info intel_gm45_info = {
	.gen = 4, .is_g4x = 1,
	.is_mobile = 1, .need_gfx_hws = 1, .has_fbc = 1,
	.has_pipe_cxsr = 1, .has_hotplug = 1,
	.supports_tv = 1,
	.has_bsd_ring = 1,
};

static const struct intel_device_info intel_pineview_info = {
	.gen = 3, .is_g33 = 1, .is_pineview = 1, .is_mobile = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_overlay = 1,
};

static const struct intel_device_info intel_ironlake_d_info = {
	.gen = 5,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_bsd_ring = 1,
};

static const struct intel_device_info intel_ironlake_m_info = {
	.gen = 5, .is_mobile = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_fbc = 1,
	.has_bsd_ring = 1,
};

static const struct intel_device_info intel_sandybridge_d_info = {
	.gen = 6,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
	.has_llc = 1,
	.has_force_wake = 1,
};

static const struct intel_device_info intel_sandybridge_m_info = {
	.gen = 6, .is_mobile = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_fbc = 1,
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
	.has_llc = 1,
	.has_force_wake = 1,
};

static const struct intel_device_info intel_ivybridge_d_info = {
	.is_ivybridge = 1, .gen = 7,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
	.has_llc = 1,
	.has_force_wake = 1,
};

static const struct intel_device_info intel_ivybridge_m_info = {
	.is_ivybridge = 1, .gen = 7, .is_mobile = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_fbc = 0,	/* FBC is not enabled on Ivybridge mobile yet */
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
	.has_llc = 1,
	.has_force_wake = 1,
};

static const struct intel_device_info intel_valleyview_m_info = {
	.gen = 7, .is_mobile = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_fbc = 0,
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
	.is_valleyview = 1,
};

static const struct intel_device_info intel_valleyview_d_info = {
	.gen = 7,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_fbc = 0,
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
	.is_valleyview = 1,
};

static const struct intel_device_info intel_haswell_d_info = {
	.is_haswell = 1, .gen = 7,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
	.has_llc = 1,
	.has_force_wake = 1,
};

static const struct intel_device_info intel_haswell_m_info = {
	.is_haswell = 1, .gen = 7, .is_mobile = 1,
	.need_gfx_hws = 1, .has_hotplug = 1,
	.has_bsd_ring = 1,
	.has_blt_ring = 1,
	.has_llc = 1,
	.has_force_wake = 1,
};

static const struct pci_device_id pciidlist[] = {		/* aka */
	INTEL_VGA_DEVICE(0x3577, &intel_i830_info),		/* I830_M */
	INTEL_VGA_DEVICE(0x2562, &intel_845g_info),		/* 845_G */
	INTEL_VGA_DEVICE(0x3582, &intel_i85x_info),		/* I855_GM */
	INTEL_VGA_DEVICE(0x358e, &intel_i85x_info),
	INTEL_VGA_DEVICE(0x2572, &intel_i865g_info),		/* I865_G */
	INTEL_VGA_DEVICE(0x2582, &intel_i915g_info),		/* I915_G */
	INTEL_VGA_DEVICE(0x258a, &intel_i915g_info),		/* E7221_G */
	INTEL_VGA_DEVICE(0x2592, &intel_i915gm_info),		/* I915_GM */
	INTEL_VGA_DEVICE(0x2772, &intel_i945g_info),		/* I945_G */
	INTEL_VGA_DEVICE(0x27a2, &intel_i945gm_info),		/* I945_GM */
	INTEL_VGA_DEVICE(0x27ae, &intel_i945gm_info),		/* I945_GME */
	INTEL_VGA_DEVICE(0x2972, &intel_i965g_info),		/* I946_GZ */
	INTEL_VGA_DEVICE(0x2982, &intel_i965g_info),		/* G35_G */
	INTEL_VGA_DEVICE(0x2992, &intel_i965g_info),		/* I965_Q */
	INTEL_VGA_DEVICE(0x29a2, &intel_i965g_info),		/* I965_G */
	INTEL_VGA_DEVICE(0x29b2, &intel_g33_info),		/* Q35_G */
	INTEL_VGA_DEVICE(0x29c2, &intel_g33_info),		/* G33_G */
	INTEL_VGA_DEVICE(0x29d2, &intel_g33_info),		/* Q33_G */
	INTEL_VGA_DEVICE(0x2a02, &intel_i965gm_info),		/* I965_GM */
	INTEL_VGA_DEVICE(0x2a12, &intel_i965gm_info),		/* I965_GME */
	INTEL_VGA_DEVICE(0x2a42, &intel_gm45_info),		/* GM45_G */
	INTEL_VGA_DEVICE(0x2e02, &intel_g45_info),		/* IGD_E_G */
	INTEL_VGA_DEVICE(0x2e12, &intel_g45_info),		/* Q45_G */
	INTEL_VGA_DEVICE(0x2e22, &intel_g45_info),		/* G45_G */
	INTEL_VGA_DEVICE(0x2e32, &intel_g45_info),		/* G41_G */
	INTEL_VGA_DEVICE(0x2e42, &intel_g45_info),		/* B43_G */
	INTEL_VGA_DEVICE(0x2e92, &intel_g45_info),		/* B43_G.1 */
	INTEL_VGA_DEVICE(0xa001, &intel_pineview_info),
	INTEL_VGA_DEVICE(0xa011, &intel_pineview_info),
	INTEL_VGA_DEVICE(0x0042, &intel_ironlake_d_info),
	INTEL_VGA_DEVICE(0x0046, &intel_ironlake_m_info),
	INTEL_VGA_DEVICE(0x0102, &intel_sandybridge_d_info),
	INTEL_VGA_DEVICE(0x0112, &intel_sandybridge_d_info),
	INTEL_VGA_DEVICE(0x0122, &intel_sandybridge_d_info),
	INTEL_VGA_DEVICE(0x0106, &intel_sandybridge_m_info),
	INTEL_VGA_DEVICE(0x0116, &intel_sandybridge_m_info),
	INTEL_VGA_DEVICE(0x0126, &intel_sandybridge_m_info),
	INTEL_VGA_DEVICE(0x010A, &intel_sandybridge_d_info),
	INTEL_VGA_DEVICE(0x0156, &intel_ivybridge_m_info), /* GT1 mobile */
	INTEL_VGA_DEVICE(0x0166, &intel_ivybridge_m_info), /* GT2 mobile */
	INTEL_VGA_DEVICE(0x0152, &intel_ivybridge_d_info), /* GT1 desktop */
	INTEL_VGA_DEVICE(0x0162, &intel_ivybridge_d_info), /* GT2 desktop */
	INTEL_VGA_DEVICE(0x015a, &intel_ivybridge_d_info), /* GT1 server */
	INTEL_VGA_DEVICE(0x016a, &intel_ivybridge_d_info), /* GT2 server */
	INTEL_VGA_DEVICE(0x0402, &intel_haswell_d_info), /* GT1 desktop */
	INTEL_VGA_DEVICE(0x0412, &intel_haswell_d_info), /* GT2 desktop */
	INTEL_VGA_DEVICE(0x0422, &intel_haswell_d_info), /* GT2 desktop */
	INTEL_VGA_DEVICE(0x040a, &intel_haswell_d_info), /* GT1 server */
	INTEL_VGA_DEVICE(0x041a, &intel_haswell_d_info), /* GT2 server */
	INTEL_VGA_DEVICE(0x042a, &intel_haswell_d_info), /* GT2 server */
	INTEL_VGA_DEVICE(0x0406, &intel_haswell_m_info), /* GT1 mobile */
	INTEL_VGA_DEVICE(0x0416, &intel_haswell_m_info), /* GT2 mobile */
	INTEL_VGA_DEVICE(0x0426, &intel_haswell_m_info), /* GT2 mobile */
	INTEL_VGA_DEVICE(0x0C02, &intel_haswell_d_info), /* SDV GT1 desktop */
	INTEL_VGA_DEVICE(0x0C12, &intel_haswell_d_info), /* SDV GT2 desktop */
	INTEL_VGA_DEVICE(0x0C22, &intel_haswell_d_info), /* SDV GT2 desktop */
	INTEL_VGA_DEVICE(0x0C0A, &intel_haswell_d_info), /* SDV GT1 server */
	INTEL_VGA_DEVICE(0x0C1A, &intel_haswell_d_info), /* SDV GT2 server */
	INTEL_VGA_DEVICE(0x0C2A, &intel_haswell_d_info), /* SDV GT2 server */
	INTEL_VGA_DEVICE(0x0C06, &intel_haswell_m_info), /* SDV GT1 mobile */
	INTEL_VGA_DEVICE(0x0C16, &intel_haswell_m_info), /* SDV GT2 mobile */
	INTEL_VGA_DEVICE(0x0C26, &intel_haswell_m_info), /* SDV GT2 mobile */
	INTEL_VGA_DEVICE(0x0A02, &intel_haswell_d_info), /* ULT GT1 desktop */
	INTEL_VGA_DEVICE(0x0A12, &intel_haswell_d_info), /* ULT GT2 desktop */
	INTEL_VGA_DEVICE(0x0A22, &intel_haswell_d_info), /* ULT GT2 desktop */
	INTEL_VGA_DEVICE(0x0A0A, &intel_haswell_d_info), /* ULT GT1 server */
	INTEL_VGA_DEVICE(0x0A1A, &intel_haswell_d_info), /* ULT GT2 server */
	INTEL_VGA_DEVICE(0x0A2A, &intel_haswell_d_info), /* ULT GT2 server */
	INTEL_VGA_DEVICE(0x0A06, &intel_haswell_m_info), /* ULT GT1 mobile */
	INTEL_VGA_DEVICE(0x0A16, &intel_haswell_m_info), /* ULT GT2 mobile */
	INTEL_VGA_DEVICE(0x0A26, &intel_haswell_m_info), /* ULT GT2 mobile */
	INTEL_VGA_DEVICE(0x0D12, &intel_haswell_d_info), /* CRW GT1 desktop */
	INTEL_VGA_DEVICE(0x0D22, &intel_haswell_d_info), /* CRW GT2 desktop */
	INTEL_VGA_DEVICE(0x0D32, &intel_haswell_d_info), /* CRW GT2 desktop */
	INTEL_VGA_DEVICE(0x0D1A, &intel_haswell_d_info), /* CRW GT1 server */
	INTEL_VGA_DEVICE(0x0D2A, &intel_haswell_d_info), /* CRW GT2 server */
	INTEL_VGA_DEVICE(0x0D3A, &intel_haswell_d_info), /* CRW GT2 server */
	INTEL_VGA_DEVICE(0x0D16, &intel_haswell_m_info), /* CRW GT1 mobile */
	INTEL_VGA_DEVICE(0x0D26, &intel_haswell_m_info), /* CRW GT2 mobile */
	INTEL_VGA_DEVICE(0x0D36, &intel_haswell_m_info), /* CRW GT2 mobile */
	INTEL_VGA_DEVICE(0x0f30, &intel_valleyview_m_info),
	INTEL_VGA_DEVICE(0x0157, &intel_valleyview_m_info),
	INTEL_VGA_DEVICE(0x0155, &intel_valleyview_d_info),
	{0, 0, 0}
};

#if defined(CONFIG_DRM_I915_KMS)
MODULE_DEVICE_TABLE(pci, pciidlist);
#endif

void intel_detect_pch(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct pci_dev *pch;

	/*
	 * The reason to probe ISA bridge instead of Dev31:Fun0 is to
	 * make graphics device passthrough work easy for VMM, that only
	 * need to expose ISA bridge to let driver know the real hardware
	 * underneath. This is a requirement from virtualization team.
	 */
	pch = pci_get_class(PCI_CLASS_BRIDGE_ISA << 8, NULL);
	if (pch) {
		if (pch->vendor == PCI_VENDOR_ID_INTEL) {
			unsigned short id;
			id = pch->device & INTEL_PCH_DEVICE_ID_MASK;
			dev_priv->pch_id = id;

			if (id == INTEL_PCH_IBX_DEVICE_ID_TYPE) {
				dev_priv->pch_type = PCH_IBX;
				dev_priv->num_pch_pll = 2;
				DRM_DEBUG_KMS("Found Ibex Peak PCH\n");
				WARN_ON(!IS_GEN5(dev));
			} else if (id == INTEL_PCH_CPT_DEVICE_ID_TYPE) {
				dev_priv->pch_type = PCH_CPT;
				dev_priv->num_pch_pll = 2;
				DRM_DEBUG_KMS("Found CougarPoint PCH\n");
				WARN_ON(!(IS_GEN6(dev) || IS_IVYBRIDGE(dev)));
			} else if (id == INTEL_PCH_PPT_DEVICE_ID_TYPE) {
				/* PantherPoint is CPT compatible */
				dev_priv->pch_type = PCH_CPT;
				dev_priv->num_pch_pll = 2;
				DRM_DEBUG_KMS("Found PatherPoint PCH\n");
				WARN_ON(!(IS_GEN6(dev) || IS_IVYBRIDGE(dev)));
			} else if (id == INTEL_PCH_LPT_DEVICE_ID_TYPE) {
				dev_priv->pch_type = PCH_LPT;
				dev_priv->num_pch_pll = 0;
				DRM_DEBUG_KMS("Found LynxPoint PCH\n");
				WARN_ON(!IS_HASWELL(dev));
			} else if (id == INTEL_PCH_LPT_LP_DEVICE_ID_TYPE) {
				dev_priv->pch_type = PCH_LPT;
				dev_priv->num_pch_pll = 0;
				DRM_DEBUG_KMS("Found LynxPoint LP PCH\n");
				WARN_ON(!IS_HASWELL(dev));
			}
			BUG_ON(dev_priv->num_pch_pll > I915_NUM_PLLS);
		}
		pci_dev_put(pch);
	}
}

bool i915_semaphore_is_enabled(struct drm_device *dev)
{
	if (INTEL_INFO(dev)->gen < 6)
		return 0;

	if (i915_semaphores >= 0)
		return i915_semaphores;

#ifdef CONFIG_INTEL_IOMMU
	/* Enable semaphores on SNB when IO remapping is off */
	if (INTEL_INFO(dev)->gen == 6 && intel_iommu_gfx_mapped)
		return false;
#endif

	return 1;
}

static int i915_drm_freeze(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	drm_kms_helper_poll_disable(dev);

	pci_save_state(dev->pdev);

	/* If KMS is active, we do the leavevt stuff here */
	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
		int error = i915_gem_idle(dev);
		if (error) {
			dev_err(&dev->pdev->dev,
				"GEM idle failed, resume might fail\n");
			return error;
		}

		cancel_delayed_work_sync(&dev_priv->rps.delayed_resume_work);

		intel_modeset_disable(dev);

		drm_irq_uninstall(dev);
	}

	i915_save_state(dev);

	intel_opregion_fini(dev);

	/* Modeset on resume, not lid events */
	dev_priv->modeset_on_lid = 0;

	console_lock();
	intel_fbdev_set_suspend(dev, 1);
	console_unlock();

	return 0;
}

int i915_suspend(struct drm_device *dev, pm_message_t state)
{
	int error;

	if (!dev || !dev->dev_private) {
		DRM_ERROR("dev: %p\n", dev);
		DRM_ERROR("DRM not initialized, aborting suspend.\n");
		return -ENODEV;
	}

	if (state.event == PM_EVENT_PRETHAW)
		return 0;


	if (dev->switch_power_state == DRM_SWITCH_POWER_OFF)
		return 0;

	error = i915_drm_freeze(dev);
	if (error)
		return error;

	if (state.event == PM_EVENT_SUSPEND) {
		/* Shut down the device */
		pci_disable_device(dev->pdev);
		pci_set_power_state(dev->pdev, PCI_D3hot);
	}

	return 0;
}

void intel_console_resume(struct work_struct *work)
{
	struct drm_i915_private *dev_priv =
		container_of(work, struct drm_i915_private,
			     console_resume_work);
	struct drm_device *dev = dev_priv->dev;

	console_lock();
	intel_fbdev_set_suspend(dev, 0);
	console_unlock();
}

static int __i915_drm_thaw(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int error = 0;

	i915_restore_state(dev);
	intel_opregion_setup(dev);

	/* KMS EnterVT equivalent */
	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
		intel_init_pch_refclk(dev);

		mutex_lock(&dev->struct_mutex);
		dev_priv->mm.suspended = 0;

		error = i915_gem_init_hw(dev);
		mutex_unlock(&dev->struct_mutex);

		intel_modeset_init_hw(dev);
		intel_modeset_setup_hw_state(dev, false);
		drm_irq_install(dev);
	}

	intel_opregion_init(dev);

	dev_priv->modeset_on_lid = 0;

	/*
	 * The console lock can be pretty contented on resume due
	 * to all the printk activity.  Try to keep it out of the hot
	 * path of resume if possible.
	 */
	if (console_trylock()) {
		intel_fbdev_set_suspend(dev, 0);
		console_unlock();
	} else {
		schedule_work(&dev_priv->console_resume_work);
	}

	return error;
}

static int i915_drm_thaw(struct drm_device *dev)
{
	int error = 0;

	intel_gt_reset(dev);

	if (drm_core_check_feature(dev, DRIVER_MODESET)) {
		mutex_lock(&dev->struct_mutex);
		i915_gem_restore_gtt_mappings(dev);
		mutex_unlock(&dev->struct_mutex);
	}

	__i915_drm_thaw(dev);

	return error;
}

int i915_resume(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	if (dev->switch_power_state == DRM_SWITCH_POWER_OFF)
		return 0;

	if (pci_enable_device(dev->pdev))
		return -EIO;

	pci_set_master(dev->pdev);

	intel_gt_reset(dev);

	/*
	 * Platforms with opregion should have sane BIOS, older ones (gen3 and
	 * earlier) need this since the BIOS might clear all our scratch PTEs.
	 */
	if (drm_core_check_feature(dev, DRIVER_MODESET) &&
	    !dev_priv->opregion.header) {
		mutex_lock(&dev->struct_mutex);
		i915_gem_restore_gtt_mappings(dev);
		mutex_unlock(&dev->struct_mutex);
	}

	ret = __i915_drm_thaw(dev);
	if (ret)
		return ret;

	drm_kms_helper_poll_enable(dev);
	return 0;
}

static int i8xx_do_reset(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (IS_I85X(dev))
		return -ENODEV;

	I915_WRITE(D_STATE, I915_READ(D_STATE) | DSTATE_GFX_RESET_I830);
	POSTING_READ(D_STATE);

	if (IS_I830(dev) || IS_845G(dev)) {
		I915_WRITE(DEBUG_RESET_I830,
			   DEBUG_RESET_DISPLAY |
			   DEBUG_RESET_RENDER |
			   DEBUG_RESET_FULL);
		POSTING_READ(DEBUG_RESET_I830);
		msleep(1);

		I915_WRITE(DEBUG_RESET_I830, 0);
		POSTING_READ(DEBUG_RESET_I830);
	}

	msleep(1);

	I915_WRITE(D_STATE, I915_READ(D_STATE) & ~DSTATE_GFX_RESET_I830);
	POSTING_READ(D_STATE);

	return 0;
}

static int i965_reset_complete(struct drm_device *dev)
{
	u8 gdrst;
	pci_read_config_byte(dev->pdev, I965_GDRST, &gdrst);
	return (gdrst & GRDOM_RESET_ENABLE) == 0;
}

static int i965_do_reset(struct drm_device *dev)
{
	int ret;
	u8 gdrst;

	/*
	 * Set the domains we want to reset (GRDOM/bits 2 and 3) as
	 * well as the reset bit (GR/bit 0).  Setting the GR bit
	 * triggers the reset; when done, the hardware will clear it.
	 */
	pci_read_config_byte(dev->pdev, I965_GDRST, &gdrst);
	pci_write_config_byte(dev->pdev, I965_GDRST,
			      gdrst | GRDOM_RENDER |
			      GRDOM_RESET_ENABLE);
	ret =  wait_for(i965_reset_complete(dev), 500);
	if (ret)
		return ret;

	/* We can't reset render&media without also resetting display ... */
	pci_read_config_byte(dev->pdev, I965_GDRST, &gdrst);
	pci_write_config_byte(dev->pdev, I965_GDRST,
			      gdrst | GRDOM_MEDIA |
			      GRDOM_RESET_ENABLE);

	return wait_for(i965_reset_complete(dev), 500);
}

static int ironlake_do_reset(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 gdrst;
	int ret;

	gdrst = I915_READ(MCHBAR_MIRROR_BASE + ILK_GDSR);
	I915_WRITE(MCHBAR_MIRROR_BASE + ILK_GDSR,
		   gdrst | GRDOM_RENDER | GRDOM_RESET_ENABLE);
	ret = wait_for(I915_READ(MCHBAR_MIRROR_BASE + ILK_GDSR) & 0x1, 500);
	if (ret)
		return ret;

	/* We can't reset render&media without also resetting display ... */
	gdrst = I915_READ(MCHBAR_MIRROR_BASE + ILK_GDSR);
	I915_WRITE(MCHBAR_MIRROR_BASE + ILK_GDSR,
		   gdrst | GRDOM_MEDIA | GRDOM_RESET_ENABLE);
	return wait_for(I915_READ(MCHBAR_MIRROR_BASE + ILK_GDSR) & 0x1, 500);
}

static int gen6_do_reset(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int	ret;
	unsigned long irqflags;

	/* Hold gt_lock across reset to prevent any register access
	 * with forcewake not set correctly
	 */
	spin_lock_irqsave(&dev_priv->gt_lock, irqflags);

	/* Reset the chip */

	/* GEN6_GDRST is not in the gt power well, no need to check
	 * for fifo space for the write or forcewake the chip for
	 * the read
	 */
	I915_WRITE_NOTRACE(GEN6_GDRST, GEN6_GRDOM_FULL);

	/* Spin waiting for the device to ack the reset request */
	ret = wait_for((I915_READ_NOTRACE(GEN6_GDRST) & GEN6_GRDOM_FULL) == 0, 500);

	/* If reset with a user forcewake, try to restore, otherwise turn it off */
	if (dev_priv->forcewake_count)
		dev_priv->gt.force_wake_get(dev_priv);
	else
		dev_priv->gt.force_wake_put(dev_priv);

	/* Restore fifo count */
	dev_priv->gt_fifo_count = I915_READ_NOTRACE(GT_FIFO_FREE_ENTRIES);

	spin_unlock_irqrestore(&dev_priv->gt_lock, irqflags);
	return ret;
}

int intel_gpu_reset(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret = -ENODEV;

	switch (INTEL_INFO(dev)->gen) {
	case 7:
	case 6:
		ret = gen6_do_reset(dev);
		break;
	case 5:
		ret = ironlake_do_reset(dev);
		break;
	case 4:
		ret = i965_do_reset(dev);
		break;
	case 2:
		ret = i8xx_do_reset(dev);
		break;
	}

	/* Also reset the gpu hangman. */
	if (dev_priv->stop_rings) {
		DRM_DEBUG("Simulated gpu hang, resetting stop_rings\n");
		dev_priv->stop_rings = 0;
		if (ret == -ENODEV) {
			DRM_ERROR("Reset not implemented, but ignoring "
				  "error for simulated gpu hangs\n");
			ret = 0;
		}
	}

	return ret;
}

/**
 * i915_reset - reset chip after a hang
 * @dev: drm device to reset
 *
 * Reset the chip.  Useful if a hang is detected. Returns zero on successful
 * reset or otherwise an error code.
 *
 * Procedure is fairly simple:
 *   - reset the chip using the reset reg
 *   - re-init context state
 *   - re-init hardware status page
 *   - re-init ring buffer
 *   - re-init interrupt state
 *   - re-init display
 */
int i915_reset(struct drm_device *dev)
{
	drm_i915_private_t *dev_priv = dev->dev_private;
	int ret;

	if (!i915_try_reset)
		return 0;

	mutex_lock(&dev->struct_mutex);

	i915_gem_reset(dev);

	ret = -ENODEV;
	if (get_seconds() - dev_priv->last_gpu_reset < 5)
		DRM_ERROR("GPU hanging too fast, declaring wedged!\n");
	else
		ret = intel_gpu_reset(dev);

	dev_priv->last_gpu_reset = get_seconds();
	if (ret) {
		DRM_ERROR("Failed to reset chip.\n");
		mutex_unlock(&dev->struct_mutex);
		return ret;
	}

	/* Ok, now get things going again... */

	/*
	 * Everything depends on having the GTT running, so we need to start
	 * there.  Fortunately we don't need to do this unless we reset the
	 * chip at a PCI level.
	 *
	 * Next we need to restore the context, but we don't use those
	 * yet either...
	 *
	 * Ring buffer needs to be re-initialized in the KMS case, or if X
	 * was running at the time of the reset (i.e. we weren't VT
	 * switched away).
	 */
	if (drm_core_check_feature(dev, DRIVER_MODESET) ||
			!dev_priv->mm.suspended) {
		struct intel_ring_buffer *ring;
		int i;

		dev_priv->mm.suspended = 0;

		i915_gem_init_swizzling(dev);

		for_each_ring(ring, dev_priv, i)
			ring->init(ring);

		i915_gem_context_init(dev);
		i915_gem_init_ppgtt(dev);

		/*
		 * It would make sense to re-init all the other hw state, at
		 * least the rps/rc6/emon init done within modeset_init_hw. For
		 * some unknown reason, this blows up my ilk, so don't.
		 */

		mutex_unlock(&dev->struct_mutex);

		drm_irq_uninstall(dev);
		drm_irq_install(dev);
	} else {
		mutex_unlock(&dev->struct_mutex);
	}

	return 0;
}

static int i915_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct intel_device_info *intel_info =
		(struct intel_device_info *) ent->driver_data;

	if (intel_info->is_valleyview)
		if(!i915_preliminary_hw_support) {
			DRM_ERROR("Preliminary hardware support disabled\n");
			return -ENODEV;
		}

	/* Only bind to function 0 of the device. Early generations
	 * used function 1 as a placeholder for multi-head. This causes
	 * us confusion instead, especially on the systems where both
	 * functions have the same PCI-ID!
	 */
	if (PCI_FUNC(pdev->devfn))
		return -ENODEV;

	/* We've managed to ship a kms-enabled ddx that shipped with an XvMC
	 * implementation for gen3 (and only gen3) that used legacy drm maps
	 * (gasp!) to share buffers between X and the client. Hence we need to
	 * keep around the fake agp stuff for gen3, even when kms is enabled. */
	if (intel_info->gen != 3) {
		driver.driver_features &=
			~(DRIVER_USE_AGP | DRIVER_REQUIRE_AGP);
	} else if (!intel_agp_enabled) {
		DRM_ERROR("drm/i915 can't work without intel_agp module!\n");
		return -ENODEV;
	}

	return drm_get_pci_dev(pdev, ent, &driver);
}

static void
i915_pci_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);

	drm_put_dev(dev);
}

static int i915_pm_suspend(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);
	int error;

	if (!drm_dev || !drm_dev->dev_private) {
		dev_err(dev, "DRM not initialized, aborting suspend.\n");
		return -ENODEV;
	}

	if (drm_dev->switch_power_state == DRM_SWITCH_POWER_OFF)
		return 0;

	error = i915_drm_freeze(drm_dev);
	if (error)
		return error;

	pci_disable_device(pdev);
	pci_set_power_state(pdev, PCI_D3hot);

	return 0;
}

static int i915_pm_resume(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);

	return i915_resume(drm_dev);
}

static int i915_pm_freeze(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);

	if (!drm_dev || !drm_dev->dev_private) {
		dev_err(dev, "DRM not initialized, aborting suspend.\n");
		return -ENODEV;
	}

	return i915_drm_freeze(drm_dev);
}

static int i915_pm_thaw(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);

	return i915_drm_thaw(drm_dev);
}

static int i915_pm_poweroff(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);

	return i915_drm_freeze(drm_dev);
}

static const struct dev_pm_ops i915_pm_ops = {
	.suspend = i915_pm_suspend,
	.resume = i915_pm_resume,
	.freeze = i915_pm_freeze,
	.thaw = i915_pm_thaw,
	.poweroff = i915_pm_poweroff,
	.restore = i915_pm_resume,
};

static const struct vm_operations_struct i915_gem_vm_ops = {
	.fault = i915_gem_fault,
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

static const struct file_operations i915_driver_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.mmap = drm_gem_mmap,
	.poll = drm_poll,
	.fasync = drm_fasync,
	.read = drm_read,
#ifdef CONFIG_COMPAT
	.compat_ioctl = i915_compat_ioctl,
#endif
	.llseek = noop_llseek,
};

static struct drm_driver driver = {
	/* Don't use MTRRs here; the Xserver or userspace app should
	 * deal with them for Intel hardware.
	 */
	.driver_features =
	    DRIVER_USE_AGP | DRIVER_REQUIRE_AGP | /* DRIVER_USE_MTRR |*/
	    DRIVER_HAVE_IRQ | DRIVER_IRQ_SHARED | DRIVER_GEM | DRIVER_PRIME,
	.load = i915_driver_load,
	.unload = i915_driver_unload,
	.open = i915_driver_open,
	.lastclose = i915_driver_lastclose,
	.preclose = i915_driver_preclose,
	.postclose = i915_driver_postclose,

	/* Used in place of i915_pm_ops for non-DRIVER_MODESET */
	.suspend = i915_suspend,
	.resume = i915_resume,

	.device_is_agp = i915_driver_device_is_agp,
	.master_create = i915_master_create,
	.master_destroy = i915_master_destroy,
#if defined(CONFIG_DEBUG_FS)
	.debugfs_init = i915_debugfs_init,
	.debugfs_cleanup = i915_debugfs_cleanup,
#endif
	.gem_init_object = i915_gem_init_object,
	.gem_free_object = i915_gem_free_object,
	.gem_vm_ops = &i915_gem_vm_ops,

	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
	.gem_prime_export = i915_gem_prime_export,
	.gem_prime_import = i915_gem_prime_import,

	.dumb_create = i915_gem_dumb_create,
	.dumb_map_offset = i915_gem_mmap_gtt,
	.dumb_destroy = i915_gem_dumb_destroy,
	.ioctls = i915_ioctls,
	.fops = &i915_driver_fops,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};

static struct pci_driver i915_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = pciidlist,
	.probe = i915_pci_probe,
	.remove = i915_pci_remove,
	.driver.pm = &i915_pm_ops,
};

static int __init i915_init(void)
{
	driver.num_ioctls = i915_max_ioctl;

	/*
	 * If CONFIG_DRM_I915_KMS is set, default to KMS unless
	 * explicitly disabled with the module pararmeter.
	 *
	 * Otherwise, just follow the parameter (defaulting to off).
	 *
	 * Allow optional vga_text_mode_force boot option to override
	 * the default behavior.
	 */
#if defined(CONFIG_DRM_I915_KMS)
	if (i915_modeset != 0)
		driver.driver_features |= DRIVER_MODESET;
#endif
	if (i915_modeset == 1)
		driver.driver_features |= DRIVER_MODESET;

#ifdef CONFIG_VGA_CONSOLE
	if (vgacon_text_force() && i915_modeset == -1)
		driver.driver_features &= ~DRIVER_MODESET;
#endif

	if (!(driver.driver_features & DRIVER_MODESET))
		driver.get_vblank_timestamp = NULL;

	return drm_pci_init(&driver, &i915_pci_driver);
}

static void __exit i915_exit(void)
{
	drm_pci_exit(&driver, &i915_pci_driver);
}

module_init(i915_init);
module_exit(i915_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");

/* We give fast paths for the really cool registers */
#define NEEDS_FORCE_WAKE(dev_priv, reg) \
	((HAS_FORCE_WAKE((dev_priv)->dev)) && \
	 ((reg) < 0x40000) &&            \
	 ((reg) != FORCEWAKE))

static bool IS_DISPLAYREG(u32 reg)
{
	/*
	 * This should make it easier to transition modules over to the
	 * new register block scheme, since we can do it incrementally.
	 */
	if (reg >= VLV_DISPLAY_BASE)
		return false;

	if (reg >= RENDER_RING_BASE &&
	    reg < RENDER_RING_BASE + 0xff)
		return false;
	if (reg >= GEN6_BSD_RING_BASE &&
	    reg < GEN6_BSD_RING_BASE + 0xff)
		return false;
	if (reg >= BLT_RING_BASE &&
	    reg < BLT_RING_BASE + 0xff)
		return false;

	if (reg == PGTBL_ER)
		return false;

	if (reg >= IPEIR_I965 &&
	    reg < HWSTAM)
		return false;

	if (reg == MI_MODE)
		return false;

	if (reg == GFX_MODE_GEN7)
		return false;

	if (reg == RENDER_HWS_PGA_GEN7 ||
	    reg == BSD_HWS_PGA_GEN7 ||
	    reg == BLT_HWS_PGA_GEN7)
		return false;

	if (reg == GEN6_BSD_SLEEP_PSMI_CONTROL ||
	    reg == GEN6_BSD_RNCID)
		return false;

	if (reg == GEN6_BLITTER_ECOSKPD)
		return false;

	if (reg >= 0x4000c &&
	    reg <= 0x4002c)
		return false;

	if (reg >= 0x4f000 &&
	    reg <= 0x4f08f)
		return false;

	if (reg >= 0x4f100 &&
	    reg <= 0x4f11f)
		return false;

	if (reg >= VLV_MASTER_IER &&
	    reg <= GEN6_PMIER)
		return false;

	if (reg >= FENCE_REG_SANDYBRIDGE_0 &&
	    reg < (FENCE_REG_SANDYBRIDGE_0 + (16*8)))
		return false;

	if (reg >= VLV_IIR_RW &&
	    reg <= VLV_ISR)
		return false;

	if (reg == FORCEWAKE_VLV ||
	    reg == FORCEWAKE_ACK_VLV)
		return false;

	if (reg == GEN6_GDRST)
		return false;

	switch (reg) {
	case _3D_CHICKEN3:
	case IVB_CHICKEN3:
	case GEN7_COMMON_SLICE_CHICKEN1:
	case GEN7_L3CNTLREG1:
	case GEN7_L3_CHICKEN_MODE_REGISTER:
	case GEN7_ROW_CHICKEN2:
	case GEN7_L3SQCREG4:
	case GEN7_SQ_CHICKEN_MBCUNIT_CONFIG:
	case GEN7_HALF_SLICE_CHICKEN1:
	case GEN6_MBCTL:
	case GEN6_UCGCTL2:
		return false;
	default:
		break;
	}

	return true;
}

static void
ilk_dummy_write(struct drm_i915_private *dev_priv)
{
	/* WaIssueDummyWriteToWakeupFromRC6: Issue a dummy write to wake up the
	 * chip from rc6 before touching it for real. MI_MODE is masked, hence
	 * harmless to write 0 into. */
	I915_WRITE_NOTRACE(MI_MODE, 0);
}

#define __i915_read(x, y) \
u##x i915_read##x(struct drm_i915_private *dev_priv, u32 reg) { \
	u##x val = 0; \
	if (IS_GEN5(dev_priv->dev)) \
		ilk_dummy_write(dev_priv); \
	if (NEEDS_FORCE_WAKE((dev_priv), (reg))) { \
		unsigned long irqflags; \
		spin_lock_irqsave(&dev_priv->gt_lock, irqflags); \
		if (dev_priv->forcewake_count == 0) \
			dev_priv->gt.force_wake_get(dev_priv); \
		val = read##y(dev_priv->regs + reg); \
		if (dev_priv->forcewake_count == 0) \
			dev_priv->gt.force_wake_put(dev_priv); \
		spin_unlock_irqrestore(&dev_priv->gt_lock, irqflags); \
	} else if (IS_VALLEYVIEW(dev_priv->dev) && IS_DISPLAYREG(reg)) { \
		val = read##y(dev_priv->regs + reg + 0x180000);		\
	} else { \
		val = read##y(dev_priv->regs + reg); \
	} \
	trace_i915_reg_rw(false, reg, val, sizeof(val)); \
	return val; \
}

__i915_read(8, b)
__i915_read(16, w)
__i915_read(32, l)
__i915_read(64, q)
#undef __i915_read

#define __i915_write(x, y) \
void i915_write##x(struct drm_i915_private *dev_priv, u32 reg, u##x val) { \
	u32 __fifo_ret = 0; \
	trace_i915_reg_rw(true, reg, val, sizeof(val)); \
	if (NEEDS_FORCE_WAKE((dev_priv), (reg))) { \
		__fifo_ret = __gen6_gt_wait_for_fifo(dev_priv); \
	} \
	if (IS_GEN5(dev_priv->dev)) \
		ilk_dummy_write(dev_priv); \
	if (IS_HASWELL(dev_priv->dev) && (I915_READ_NOTRACE(GEN7_ERR_INT) & ERR_INT_MMIO_UNCLAIMED)) { \
		DRM_ERROR("Unknown unclaimed register before writing to %x\n", reg); \
		I915_WRITE_NOTRACE(GEN7_ERR_INT, ERR_INT_MMIO_UNCLAIMED); \
	} \
	if (IS_VALLEYVIEW(dev_priv->dev) && IS_DISPLAYREG(reg)) { \
		write##y(val, dev_priv->regs + reg + 0x180000);		\
	} else {							\
		write##y(val, dev_priv->regs + reg);			\
	}								\
	if (unlikely(__fifo_ret)) { \
		gen6_gt_check_fifodbg(dev_priv); \
	} \
	if (IS_HASWELL(dev_priv->dev) && (I915_READ_NOTRACE(GEN7_ERR_INT) & ERR_INT_MMIO_UNCLAIMED)) { \
		DRM_ERROR("Unclaimed write to %x\n", reg); \
		writel(ERR_INT_MMIO_UNCLAIMED, dev_priv->regs + GEN7_ERR_INT);	\
	} \
}
__i915_write(8, b)
__i915_write(16, w)
__i915_write(32, l)
__i915_write(64, q)
#undef __i915_write

static const struct register_whitelist {
	uint64_t offset;
	uint32_t size;
	uint32_t gen_bitmask; /* support gens, 0x10 for 4, 0x30 for 4 and 5, etc. */
} whitelist[] = {
	{ RING_TIMESTAMP(RENDER_RING_BASE), 8, 0xF0 },
};

int i915_reg_read_ioctl(struct drm_device *dev,
			void *data, struct drm_file *file)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_reg_read *reg = data;
	struct register_whitelist const *entry = whitelist;
	int i;

	for (i = 0; i < ARRAY_SIZE(whitelist); i++, entry++) {
		if (entry->offset == reg->offset &&
		    (1 << INTEL_INFO(dev)->gen & entry->gen_bitmask))
			break;
	}

	if (i == ARRAY_SIZE(whitelist))
		return -EINVAL;

	switch (entry->size) {
	case 8:
		reg->val = I915_READ64(reg->offset);
		break;
	case 4:
		reg->val = I915_READ(reg->offset);
		break;
	case 2:
		reg->val = I915_READ16(reg->offset);
		break;
	case 1:
		reg->val = I915_READ8(reg->offset);
		break;
	default:
		WARN_ON(1);
		return -EINVAL;
	}

	return 0;
}
