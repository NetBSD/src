/*	$NetBSD: amdgpu_cik.c,v 1.3 2018/08/27 14:24:03 riastradh Exp $	*/

/*
 * Copyright 2012 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Alex Deucher
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amdgpu_cik.c,v 1.3 2018/08/27 14:24:03 riastradh Exp $");

#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/module.h>
#include "drmP.h"
#include "amdgpu.h"
#include "amdgpu_atombios.h"
#include "amdgpu_ih.h"
#include "amdgpu_uvd.h"
#include "amdgpu_vce.h"
#include "cikd.h"
#include "atom.h"

#include "cik.h"
#include "gmc_v7_0.h"
#include "cik_ih.h"
#include "dce_v8_0.h"
#include "gfx_v7_0.h"
#include "cik_sdma.h"
#include "uvd_v4_2.h"
#include "vce_v2_0.h"
#include "cik_dpm.h"

#include "uvd/uvd_4_2_d.h"

#include "smu/smu_7_0_1_d.h"
#include "smu/smu_7_0_1_sh_mask.h"

#include "dce/dce_8_0_d.h"
#include "dce/dce_8_0_sh_mask.h"

#include "bif/bif_4_1_d.h"
#include "bif/bif_4_1_sh_mask.h"

#include "gca/gfx_7_2_d.h"
#include "gca/gfx_7_2_enum.h"
#include "gca/gfx_7_2_sh_mask.h"

#include "gmc/gmc_7_1_d.h"
#include "gmc/gmc_7_1_sh_mask.h"

#include "oss/oss_2_0_d.h"
#include "oss/oss_2_0_sh_mask.h"

#include "amdgpu_amdkfd.h"

/*
 * Indirect registers accessor
 */
static u32 cik_pcie_rreg(struct amdgpu_device *adev, u32 reg)
{
	unsigned long flags;
	u32 r;

	spin_lock_irqsave(&adev->pcie_idx_lock, flags);
	WREG32(mmPCIE_INDEX, reg);
	(void)RREG32(mmPCIE_INDEX);
	r = RREG32(mmPCIE_DATA);
	spin_unlock_irqrestore(&adev->pcie_idx_lock, flags);
	return r;
}

static void cik_pcie_wreg(struct amdgpu_device *adev, u32 reg, u32 v)
{
	unsigned long flags;

	spin_lock_irqsave(&adev->pcie_idx_lock, flags);
	WREG32(mmPCIE_INDEX, reg);
	(void)RREG32(mmPCIE_INDEX);
	WREG32(mmPCIE_DATA, v);
	(void)RREG32(mmPCIE_DATA);
	spin_unlock_irqrestore(&adev->pcie_idx_lock, flags);
}

static u32 cik_smc_rreg(struct amdgpu_device *adev, u32 reg)
{
	unsigned long flags;
	u32 r;

	spin_lock_irqsave(&adev->smc_idx_lock, flags);
	WREG32(mmSMC_IND_INDEX_0, (reg));
	r = RREG32(mmSMC_IND_DATA_0);
	spin_unlock_irqrestore(&adev->smc_idx_lock, flags);
	return r;
}

static void cik_smc_wreg(struct amdgpu_device *adev, u32 reg, u32 v)
{
	unsigned long flags;

	spin_lock_irqsave(&adev->smc_idx_lock, flags);
	WREG32(mmSMC_IND_INDEX_0, (reg));
	WREG32(mmSMC_IND_DATA_0, (v));
	spin_unlock_irqrestore(&adev->smc_idx_lock, flags);
}

static u32 cik_uvd_ctx_rreg(struct amdgpu_device *adev, u32 reg)
{
	unsigned long flags;
	u32 r;

	spin_lock_irqsave(&adev->uvd_ctx_idx_lock, flags);
	WREG32(mmUVD_CTX_INDEX, ((reg) & 0x1ff));
	r = RREG32(mmUVD_CTX_DATA);
	spin_unlock_irqrestore(&adev->uvd_ctx_idx_lock, flags);
	return r;
}

static void cik_uvd_ctx_wreg(struct amdgpu_device *adev, u32 reg, u32 v)
{
	unsigned long flags;

	spin_lock_irqsave(&adev->uvd_ctx_idx_lock, flags);
	WREG32(mmUVD_CTX_INDEX, ((reg) & 0x1ff));
	WREG32(mmUVD_CTX_DATA, (v));
	spin_unlock_irqrestore(&adev->uvd_ctx_idx_lock, flags);
}

static u32 cik_didt_rreg(struct amdgpu_device *adev, u32 reg)
{
	unsigned long flags;
	u32 r;

	spin_lock_irqsave(&adev->didt_idx_lock, flags);
	WREG32(mmDIDT_IND_INDEX, (reg));
	r = RREG32(mmDIDT_IND_DATA);
	spin_unlock_irqrestore(&adev->didt_idx_lock, flags);
	return r;
}

static void cik_didt_wreg(struct amdgpu_device *adev, u32 reg, u32 v)
{
	unsigned long flags;

	spin_lock_irqsave(&adev->didt_idx_lock, flags);
	WREG32(mmDIDT_IND_INDEX, (reg));
	WREG32(mmDIDT_IND_DATA, (v));
	spin_unlock_irqrestore(&adev->didt_idx_lock, flags);
}

static const u32 bonaire_golden_spm_registers[] =
{
	0xc200, 0xe0ffffff, 0xe0000000
};

static const u32 bonaire_golden_common_registers[] =
{
	0x31dc, 0xffffffff, 0x00000800,
	0x31dd, 0xffffffff, 0x00000800,
	0x31e6, 0xffffffff, 0x00007fbf,
	0x31e7, 0xffffffff, 0x00007faf
};

static const u32 bonaire_golden_registers[] =
{
	0xcd5, 0x00000333, 0x00000333,
	0xcd4, 0x000c0fc0, 0x00040200,
	0x2684, 0x00010000, 0x00058208,
	0xf000, 0xffff1fff, 0x00140000,
	0xf080, 0xfdfc0fff, 0x00000100,
	0xf08d, 0x40000000, 0x40000200,
	0x260c, 0xffffffff, 0x00000000,
	0x260d, 0xf00fffff, 0x00000400,
	0x260e, 0x0002021c, 0x00020200,
	0x31e, 0x00000080, 0x00000000,
	0x16ec, 0x000000f0, 0x00000070,
	0x16f0, 0xf0311fff, 0x80300000,
	0x263e, 0x73773777, 0x12010001,
	0xd43, 0x00810000, 0x408af000,
	0x1c0c, 0x31000111, 0x00000011,
	0xbd2, 0x73773777, 0x12010001,
	0x883, 0x00007fb6, 0x0021a1b1,
	0x884, 0x00007fb6, 0x002021b1,
	0x860, 0x00007fb6, 0x00002191,
	0x886, 0x00007fb6, 0x002121b1,
	0x887, 0x00007fb6, 0x002021b1,
	0x877, 0x00007fb6, 0x00002191,
	0x878, 0x00007fb6, 0x00002191,
	0xd8a, 0x0000003f, 0x0000000a,
	0xd8b, 0x0000003f, 0x0000000a,
	0xab9, 0x00073ffe, 0x000022a2,
	0x903, 0x000007ff, 0x00000000,
	0x2285, 0xf000003f, 0x00000007,
	0x22fc, 0x00002001, 0x00000001,
	0x22c9, 0xffffffff, 0x00ffffff,
	0xc281, 0x0000ff0f, 0x00000000,
	0xa293, 0x07ffffff, 0x06000000,
	0x136, 0x00000fff, 0x00000100,
	0xf9e, 0x00000001, 0x00000002,
	0x2440, 0x03000000, 0x0362c688,
	0x2300, 0x000000ff, 0x00000001,
	0x390, 0x00001fff, 0x00001fff,
	0x2418, 0x0000007f, 0x00000020,
	0x2542, 0x00010000, 0x00010000,
	0x2b05, 0x000003ff, 0x000000f3,
	0x2b03, 0xffffffff, 0x00001032
};

static const u32 bonaire_mgcg_cgcg_init[] =
{
	0x3108, 0xffffffff, 0xfffffffc,
	0xc200, 0xffffffff, 0xe0000000,
	0xf0a8, 0xffffffff, 0x00000100,
	0xf082, 0xffffffff, 0x00000100,
	0xf0b0, 0xffffffff, 0xc0000100,
	0xf0b2, 0xffffffff, 0xc0000100,
	0xf0b1, 0xffffffff, 0xc0000100,
	0x1579, 0xffffffff, 0x00600100,
	0xf0a0, 0xffffffff, 0x00000100,
	0xf085, 0xffffffff, 0x06000100,
	0xf088, 0xffffffff, 0x00000100,
	0xf086, 0xffffffff, 0x06000100,
	0xf081, 0xffffffff, 0x00000100,
	0xf0b8, 0xffffffff, 0x00000100,
	0xf089, 0xffffffff, 0x00000100,
	0xf080, 0xffffffff, 0x00000100,
	0xf08c, 0xffffffff, 0x00000100,
	0xf08d, 0xffffffff, 0x00000100,
	0xf094, 0xffffffff, 0x00000100,
	0xf095, 0xffffffff, 0x00000100,
	0xf096, 0xffffffff, 0x00000100,
	0xf097, 0xffffffff, 0x00000100,
	0xf098, 0xffffffff, 0x00000100,
	0xf09f, 0xffffffff, 0x00000100,
	0xf09e, 0xffffffff, 0x00000100,
	0xf084, 0xffffffff, 0x06000100,
	0xf0a4, 0xffffffff, 0x00000100,
	0xf09d, 0xffffffff, 0x00000100,
	0xf0ad, 0xffffffff, 0x00000100,
	0xf0ac, 0xffffffff, 0x00000100,
	0xf09c, 0xffffffff, 0x00000100,
	0xc200, 0xffffffff, 0xe0000000,
	0xf008, 0xffffffff, 0x00010000,
	0xf009, 0xffffffff, 0x00030002,
	0xf00a, 0xffffffff, 0x00040007,
	0xf00b, 0xffffffff, 0x00060005,
	0xf00c, 0xffffffff, 0x00090008,
	0xf00d, 0xffffffff, 0x00010000,
	0xf00e, 0xffffffff, 0x00030002,
	0xf00f, 0xffffffff, 0x00040007,
	0xf010, 0xffffffff, 0x00060005,
	0xf011, 0xffffffff, 0x00090008,
	0xf012, 0xffffffff, 0x00010000,
	0xf013, 0xffffffff, 0x00030002,
	0xf014, 0xffffffff, 0x00040007,
	0xf015, 0xffffffff, 0x00060005,
	0xf016, 0xffffffff, 0x00090008,
	0xf017, 0xffffffff, 0x00010000,
	0xf018, 0xffffffff, 0x00030002,
	0xf019, 0xffffffff, 0x00040007,
	0xf01a, 0xffffffff, 0x00060005,
	0xf01b, 0xffffffff, 0x00090008,
	0xf01c, 0xffffffff, 0x00010000,
	0xf01d, 0xffffffff, 0x00030002,
	0xf01e, 0xffffffff, 0x00040007,
	0xf01f, 0xffffffff, 0x00060005,
	0xf020, 0xffffffff, 0x00090008,
	0xf021, 0xffffffff, 0x00010000,
	0xf022, 0xffffffff, 0x00030002,
	0xf023, 0xffffffff, 0x00040007,
	0xf024, 0xffffffff, 0x00060005,
	0xf025, 0xffffffff, 0x00090008,
	0xf026, 0xffffffff, 0x00010000,
	0xf027, 0xffffffff, 0x00030002,
	0xf028, 0xffffffff, 0x00040007,
	0xf029, 0xffffffff, 0x00060005,
	0xf02a, 0xffffffff, 0x00090008,
	0xf000, 0xffffffff, 0x96e00200,
	0x21c2, 0xffffffff, 0x00900100,
	0x3109, 0xffffffff, 0x0020003f,
	0xe, 0xffffffff, 0x0140001c,
	0xf, 0x000f0000, 0x000f0000,
	0x88, 0xffffffff, 0xc060000c,
	0x89, 0xc0000fff, 0x00000100,
	0x3e4, 0xffffffff, 0x00000100,
	0x3e6, 0x00000101, 0x00000000,
	0x82a, 0xffffffff, 0x00000104,
	0x1579, 0xff000fff, 0x00000100,
	0xc33, 0xc0000fff, 0x00000104,
	0x3079, 0x00000001, 0x00000001,
	0x3403, 0xff000ff0, 0x00000100,
	0x3603, 0xff000ff0, 0x00000100
};

static const u32 spectre_golden_spm_registers[] =
{
	0xc200, 0xe0ffffff, 0xe0000000
};

static const u32 spectre_golden_common_registers[] =
{
	0x31dc, 0xffffffff, 0x00000800,
	0x31dd, 0xffffffff, 0x00000800,
	0x31e6, 0xffffffff, 0x00007fbf,
	0x31e7, 0xffffffff, 0x00007faf
};

static const u32 spectre_golden_registers[] =
{
	0xf000, 0xffff1fff, 0x96940200,
	0xf003, 0xffff0001, 0xff000000,
	0xf080, 0xfffc0fff, 0x00000100,
	0x1bb6, 0x00010101, 0x00010000,
	0x260d, 0xf00fffff, 0x00000400,
	0x260e, 0xfffffffc, 0x00020200,
	0x16ec, 0x000000f0, 0x00000070,
	0x16f0, 0xf0311fff, 0x80300000,
	0x263e, 0x73773777, 0x12010001,
	0x26df, 0x00ff0000, 0x00fc0000,
	0xbd2, 0x73773777, 0x12010001,
	0x2285, 0xf000003f, 0x00000007,
	0x22c9, 0xffffffff, 0x00ffffff,
	0xa0d4, 0x3f3f3fff, 0x00000082,
	0xa0d5, 0x0000003f, 0x00000000,
	0xf9e, 0x00000001, 0x00000002,
	0x244f, 0xffff03df, 0x00000004,
	0x31da, 0x00000008, 0x00000008,
	0x2300, 0x000008ff, 0x00000800,
	0x2542, 0x00010000, 0x00010000,
	0x2b03, 0xffffffff, 0x54763210,
	0x853e, 0x01ff01ff, 0x00000002,
	0x8526, 0x007ff800, 0x00200000,
	0x8057, 0xffffffff, 0x00000f40,
	0xc24d, 0xffffffff, 0x00000001
};

static const u32 spectre_mgcg_cgcg_init[] =
{
	0x3108, 0xffffffff, 0xfffffffc,
	0xc200, 0xffffffff, 0xe0000000,
	0xf0a8, 0xffffffff, 0x00000100,
	0xf082, 0xffffffff, 0x00000100,
	0xf0b0, 0xffffffff, 0x00000100,
	0xf0b2, 0xffffffff, 0x00000100,
	0xf0b1, 0xffffffff, 0x00000100,
	0x1579, 0xffffffff, 0x00600100,
	0xf0a0, 0xffffffff, 0x00000100,
	0xf085, 0xffffffff, 0x06000100,
	0xf088, 0xffffffff, 0x00000100,
	0xf086, 0xffffffff, 0x06000100,
	0xf081, 0xffffffff, 0x00000100,
	0xf0b8, 0xffffffff, 0x00000100,
	0xf089, 0xffffffff, 0x00000100,
	0xf080, 0xffffffff, 0x00000100,
	0xf08c, 0xffffffff, 0x00000100,
	0xf08d, 0xffffffff, 0x00000100,
	0xf094, 0xffffffff, 0x00000100,
	0xf095, 0xffffffff, 0x00000100,
	0xf096, 0xffffffff, 0x00000100,
	0xf097, 0xffffffff, 0x00000100,
	0xf098, 0xffffffff, 0x00000100,
	0xf09f, 0xffffffff, 0x00000100,
	0xf09e, 0xffffffff, 0x00000100,
	0xf084, 0xffffffff, 0x06000100,
	0xf0a4, 0xffffffff, 0x00000100,
	0xf09d, 0xffffffff, 0x00000100,
	0xf0ad, 0xffffffff, 0x00000100,
	0xf0ac, 0xffffffff, 0x00000100,
	0xf09c, 0xffffffff, 0x00000100,
	0xc200, 0xffffffff, 0xe0000000,
	0xf008, 0xffffffff, 0x00010000,
	0xf009, 0xffffffff, 0x00030002,
	0xf00a, 0xffffffff, 0x00040007,
	0xf00b, 0xffffffff, 0x00060005,
	0xf00c, 0xffffffff, 0x00090008,
	0xf00d, 0xffffffff, 0x00010000,
	0xf00e, 0xffffffff, 0x00030002,
	0xf00f, 0xffffffff, 0x00040007,
	0xf010, 0xffffffff, 0x00060005,
	0xf011, 0xffffffff, 0x00090008,
	0xf012, 0xffffffff, 0x00010000,
	0xf013, 0xffffffff, 0x00030002,
	0xf014, 0xffffffff, 0x00040007,
	0xf015, 0xffffffff, 0x00060005,
	0xf016, 0xffffffff, 0x00090008,
	0xf017, 0xffffffff, 0x00010000,
	0xf018, 0xffffffff, 0x00030002,
	0xf019, 0xffffffff, 0x00040007,
	0xf01a, 0xffffffff, 0x00060005,
	0xf01b, 0xffffffff, 0x00090008,
	0xf01c, 0xffffffff, 0x00010000,
	0xf01d, 0xffffffff, 0x00030002,
	0xf01e, 0xffffffff, 0x00040007,
	0xf01f, 0xffffffff, 0x00060005,
	0xf020, 0xffffffff, 0x00090008,
	0xf021, 0xffffffff, 0x00010000,
	0xf022, 0xffffffff, 0x00030002,
	0xf023, 0xffffffff, 0x00040007,
	0xf024, 0xffffffff, 0x00060005,
	0xf025, 0xffffffff, 0x00090008,
	0xf026, 0xffffffff, 0x00010000,
	0xf027, 0xffffffff, 0x00030002,
	0xf028, 0xffffffff, 0x00040007,
	0xf029, 0xffffffff, 0x00060005,
	0xf02a, 0xffffffff, 0x00090008,
	0xf02b, 0xffffffff, 0x00010000,
	0xf02c, 0xffffffff, 0x00030002,
	0xf02d, 0xffffffff, 0x00040007,
	0xf02e, 0xffffffff, 0x00060005,
	0xf02f, 0xffffffff, 0x00090008,
	0xf000, 0xffffffff, 0x96e00200,
	0x21c2, 0xffffffff, 0x00900100,
	0x3109, 0xffffffff, 0x0020003f,
	0xe, 0xffffffff, 0x0140001c,
	0xf, 0x000f0000, 0x000f0000,
	0x88, 0xffffffff, 0xc060000c,
	0x89, 0xc0000fff, 0x00000100,
	0x3e4, 0xffffffff, 0x00000100,
	0x3e6, 0x00000101, 0x00000000,
	0x82a, 0xffffffff, 0x00000104,
	0x1579, 0xff000fff, 0x00000100,
	0xc33, 0xc0000fff, 0x00000104,
	0x3079, 0x00000001, 0x00000001,
	0x3403, 0xff000ff0, 0x00000100,
	0x3603, 0xff000ff0, 0x00000100
};

static const u32 kalindi_golden_spm_registers[] =
{
	0xc200, 0xe0ffffff, 0xe0000000
};

static const u32 kalindi_golden_common_registers[] =
{
	0x31dc, 0xffffffff, 0x00000800,
	0x31dd, 0xffffffff, 0x00000800,
	0x31e6, 0xffffffff, 0x00007fbf,
	0x31e7, 0xffffffff, 0x00007faf
};

static const u32 kalindi_golden_registers[] =
{
	0xf000, 0xffffdfff, 0x6e944040,
	0x1579, 0xff607fff, 0xfc000100,
	0xf088, 0xff000fff, 0x00000100,
	0xf089, 0xff000fff, 0x00000100,
	0xf080, 0xfffc0fff, 0x00000100,
	0x1bb6, 0x00010101, 0x00010000,
	0x260c, 0xffffffff, 0x00000000,
	0x260d, 0xf00fffff, 0x00000400,
	0x16ec, 0x000000f0, 0x00000070,
	0x16f0, 0xf0311fff, 0x80300000,
	0x263e, 0x73773777, 0x12010001,
	0x263f, 0xffffffff, 0x00000010,
	0x26df, 0x00ff0000, 0x00fc0000,
	0x200c, 0x00001f0f, 0x0000100a,
	0xbd2, 0x73773777, 0x12010001,
	0x902, 0x000fffff, 0x000c007f,
	0x2285, 0xf000003f, 0x00000007,
	0x22c9, 0x3fff3fff, 0x00ffcfff,
	0xc281, 0x0000ff0f, 0x00000000,
	0xa293, 0x07ffffff, 0x06000000,
	0x136, 0x00000fff, 0x00000100,
	0xf9e, 0x00000001, 0x00000002,
	0x31da, 0x00000008, 0x00000008,
	0x2300, 0x000000ff, 0x00000003,
	0x853e, 0x01ff01ff, 0x00000002,
	0x8526, 0x007ff800, 0x00200000,
	0x8057, 0xffffffff, 0x00000f40,
	0x2231, 0x001f3ae3, 0x00000082,
	0x2235, 0x0000001f, 0x00000010,
	0xc24d, 0xffffffff, 0x00000000
};

static const u32 kalindi_mgcg_cgcg_init[] =
{
	0x3108, 0xffffffff, 0xfffffffc,
	0xc200, 0xffffffff, 0xe0000000,
	0xf0a8, 0xffffffff, 0x00000100,
	0xf082, 0xffffffff, 0x00000100,
	0xf0b0, 0xffffffff, 0x00000100,
	0xf0b2, 0xffffffff, 0x00000100,
	0xf0b1, 0xffffffff, 0x00000100,
	0x1579, 0xffffffff, 0x00600100,
	0xf0a0, 0xffffffff, 0x00000100,
	0xf085, 0xffffffff, 0x06000100,
	0xf088, 0xffffffff, 0x00000100,
	0xf086, 0xffffffff, 0x06000100,
	0xf081, 0xffffffff, 0x00000100,
	0xf0b8, 0xffffffff, 0x00000100,
	0xf089, 0xffffffff, 0x00000100,
	0xf080, 0xffffffff, 0x00000100,
	0xf08c, 0xffffffff, 0x00000100,
	0xf08d, 0xffffffff, 0x00000100,
	0xf094, 0xffffffff, 0x00000100,
	0xf095, 0xffffffff, 0x00000100,
	0xf096, 0xffffffff, 0x00000100,
	0xf097, 0xffffffff, 0x00000100,
	0xf098, 0xffffffff, 0x00000100,
	0xf09f, 0xffffffff, 0x00000100,
	0xf09e, 0xffffffff, 0x00000100,
	0xf084, 0xffffffff, 0x06000100,
	0xf0a4, 0xffffffff, 0x00000100,
	0xf09d, 0xffffffff, 0x00000100,
	0xf0ad, 0xffffffff, 0x00000100,
	0xf0ac, 0xffffffff, 0x00000100,
	0xf09c, 0xffffffff, 0x00000100,
	0xc200, 0xffffffff, 0xe0000000,
	0xf008, 0xffffffff, 0x00010000,
	0xf009, 0xffffffff, 0x00030002,
	0xf00a, 0xffffffff, 0x00040007,
	0xf00b, 0xffffffff, 0x00060005,
	0xf00c, 0xffffffff, 0x00090008,
	0xf00d, 0xffffffff, 0x00010000,
	0xf00e, 0xffffffff, 0x00030002,
	0xf00f, 0xffffffff, 0x00040007,
	0xf010, 0xffffffff, 0x00060005,
	0xf011, 0xffffffff, 0x00090008,
	0xf000, 0xffffffff, 0x96e00200,
	0x21c2, 0xffffffff, 0x00900100,
	0x3109, 0xffffffff, 0x0020003f,
	0xe, 0xffffffff, 0x0140001c,
	0xf, 0x000f0000, 0x000f0000,
	0x88, 0xffffffff, 0xc060000c,
	0x89, 0xc0000fff, 0x00000100,
	0x82a, 0xffffffff, 0x00000104,
	0x1579, 0xff000fff, 0x00000100,
	0xc33, 0xc0000fff, 0x00000104,
	0x3079, 0x00000001, 0x00000001,
	0x3403, 0xff000ff0, 0x00000100,
	0x3603, 0xff000ff0, 0x00000100
};

static const u32 hawaii_golden_spm_registers[] =
{
	0xc200, 0xe0ffffff, 0xe0000000
};

static const u32 hawaii_golden_common_registers[] =
{
	0xc200, 0xffffffff, 0xe0000000,
	0xa0d4, 0xffffffff, 0x3a00161a,
	0xa0d5, 0xffffffff, 0x0000002e,
	0x2684, 0xffffffff, 0x00018208,
	0x263e, 0xffffffff, 0x12011003
};

static const u32 hawaii_golden_registers[] =
{
	0xcd5, 0x00000333, 0x00000333,
	0x2684, 0x00010000, 0x00058208,
	0x260c, 0xffffffff, 0x00000000,
	0x260d, 0xf00fffff, 0x00000400,
	0x260e, 0x0002021c, 0x00020200,
	0x31e, 0x00000080, 0x00000000,
	0x16ec, 0x000000f0, 0x00000070,
	0x16f0, 0xf0311fff, 0x80300000,
	0xd43, 0x00810000, 0x408af000,
	0x1c0c, 0x31000111, 0x00000011,
	0xbd2, 0x73773777, 0x12010001,
	0x848, 0x0000007f, 0x0000001b,
	0x877, 0x00007fb6, 0x00002191,
	0xd8a, 0x0000003f, 0x0000000a,
	0xd8b, 0x0000003f, 0x0000000a,
	0xab9, 0x00073ffe, 0x000022a2,
	0x903, 0x000007ff, 0x00000000,
	0x22fc, 0x00002001, 0x00000001,
	0x22c9, 0xffffffff, 0x00ffffff,
	0xc281, 0x0000ff0f, 0x00000000,
	0xa293, 0x07ffffff, 0x06000000,
	0xf9e, 0x00000001, 0x00000002,
	0x31da, 0x00000008, 0x00000008,
	0x31dc, 0x00000f00, 0x00000800,
	0x31dd, 0x00000f00, 0x00000800,
	0x31e6, 0x00ffffff, 0x00ff7fbf,
	0x31e7, 0x00ffffff, 0x00ff7faf,
	0x2300, 0x000000ff, 0x00000800,
	0x390, 0x00001fff, 0x00001fff,
	0x2418, 0x0000007f, 0x00000020,
	0x2542, 0x00010000, 0x00010000,
	0x2b80, 0x00100000, 0x000ff07c,
	0x2b05, 0x000003ff, 0x0000000f,
	0x2b04, 0xffffffff, 0x7564fdec,
	0x2b03, 0xffffffff, 0x3120b9a8,
	0x2b02, 0x20000000, 0x0f9c0000
};

static const u32 hawaii_mgcg_cgcg_init[] =
{
	0x3108, 0xffffffff, 0xfffffffd,
	0xc200, 0xffffffff, 0xe0000000,
	0xf0a8, 0xffffffff, 0x00000100,
	0xf082, 0xffffffff, 0x00000100,
	0xf0b0, 0xffffffff, 0x00000100,
	0xf0b2, 0xffffffff, 0x00000100,
	0xf0b1, 0xffffffff, 0x00000100,
	0x1579, 0xffffffff, 0x00200100,
	0xf0a0, 0xffffffff, 0x00000100,
	0xf085, 0xffffffff, 0x06000100,
	0xf088, 0xffffffff, 0x00000100,
	0xf086, 0xffffffff, 0x06000100,
	0xf081, 0xffffffff, 0x00000100,
	0xf0b8, 0xffffffff, 0x00000100,
	0xf089, 0xffffffff, 0x00000100,
	0xf080, 0xffffffff, 0x00000100,
	0xf08c, 0xffffffff, 0x00000100,
	0xf08d, 0xffffffff, 0x00000100,
	0xf094, 0xffffffff, 0x00000100,
	0xf095, 0xffffffff, 0x00000100,
	0xf096, 0xffffffff, 0x00000100,
	0xf097, 0xffffffff, 0x00000100,
	0xf098, 0xffffffff, 0x00000100,
	0xf09f, 0xffffffff, 0x00000100,
	0xf09e, 0xffffffff, 0x00000100,
	0xf084, 0xffffffff, 0x06000100,
	0xf0a4, 0xffffffff, 0x00000100,
	0xf09d, 0xffffffff, 0x00000100,
	0xf0ad, 0xffffffff, 0x00000100,
	0xf0ac, 0xffffffff, 0x00000100,
	0xf09c, 0xffffffff, 0x00000100,
	0xc200, 0xffffffff, 0xe0000000,
	0xf008, 0xffffffff, 0x00010000,
	0xf009, 0xffffffff, 0x00030002,
	0xf00a, 0xffffffff, 0x00040007,
	0xf00b, 0xffffffff, 0x00060005,
	0xf00c, 0xffffffff, 0x00090008,
	0xf00d, 0xffffffff, 0x00010000,
	0xf00e, 0xffffffff, 0x00030002,
	0xf00f, 0xffffffff, 0x00040007,
	0xf010, 0xffffffff, 0x00060005,
	0xf011, 0xffffffff, 0x00090008,
	0xf012, 0xffffffff, 0x00010000,
	0xf013, 0xffffffff, 0x00030002,
	0xf014, 0xffffffff, 0x00040007,
	0xf015, 0xffffffff, 0x00060005,
	0xf016, 0xffffffff, 0x00090008,
	0xf017, 0xffffffff, 0x00010000,
	0xf018, 0xffffffff, 0x00030002,
	0xf019, 0xffffffff, 0x00040007,
	0xf01a, 0xffffffff, 0x00060005,
	0xf01b, 0xffffffff, 0x00090008,
	0xf01c, 0xffffffff, 0x00010000,
	0xf01d, 0xffffffff, 0x00030002,
	0xf01e, 0xffffffff, 0x00040007,
	0xf01f, 0xffffffff, 0x00060005,
	0xf020, 0xffffffff, 0x00090008,
	0xf021, 0xffffffff, 0x00010000,
	0xf022, 0xffffffff, 0x00030002,
	0xf023, 0xffffffff, 0x00040007,
	0xf024, 0xffffffff, 0x00060005,
	0xf025, 0xffffffff, 0x00090008,
	0xf026, 0xffffffff, 0x00010000,
	0xf027, 0xffffffff, 0x00030002,
	0xf028, 0xffffffff, 0x00040007,
	0xf029, 0xffffffff, 0x00060005,
	0xf02a, 0xffffffff, 0x00090008,
	0xf02b, 0xffffffff, 0x00010000,
	0xf02c, 0xffffffff, 0x00030002,
	0xf02d, 0xffffffff, 0x00040007,
	0xf02e, 0xffffffff, 0x00060005,
	0xf02f, 0xffffffff, 0x00090008,
	0xf030, 0xffffffff, 0x00010000,
	0xf031, 0xffffffff, 0x00030002,
	0xf032, 0xffffffff, 0x00040007,
	0xf033, 0xffffffff, 0x00060005,
	0xf034, 0xffffffff, 0x00090008,
	0xf035, 0xffffffff, 0x00010000,
	0xf036, 0xffffffff, 0x00030002,
	0xf037, 0xffffffff, 0x00040007,
	0xf038, 0xffffffff, 0x00060005,
	0xf039, 0xffffffff, 0x00090008,
	0xf03a, 0xffffffff, 0x00010000,
	0xf03b, 0xffffffff, 0x00030002,
	0xf03c, 0xffffffff, 0x00040007,
	0xf03d, 0xffffffff, 0x00060005,
	0xf03e, 0xffffffff, 0x00090008,
	0x30c6, 0xffffffff, 0x00020200,
	0xcd4, 0xffffffff, 0x00000200,
	0x570, 0xffffffff, 0x00000400,
	0x157a, 0xffffffff, 0x00000000,
	0xbd4, 0xffffffff, 0x00000902,
	0xf000, 0xffffffff, 0x96940200,
	0x21c2, 0xffffffff, 0x00900100,
	0x3109, 0xffffffff, 0x0020003f,
	0xe, 0xffffffff, 0x0140001c,
	0xf, 0x000f0000, 0x000f0000,
	0x88, 0xffffffff, 0xc060000c,
	0x89, 0xc0000fff, 0x00000100,
	0x3e4, 0xffffffff, 0x00000100,
	0x3e6, 0x00000101, 0x00000000,
	0x82a, 0xffffffff, 0x00000104,
	0x1579, 0xff000fff, 0x00000100,
	0xc33, 0xc0000fff, 0x00000104,
	0x3079, 0x00000001, 0x00000001,
	0x3403, 0xff000ff0, 0x00000100,
	0x3603, 0xff000ff0, 0x00000100
};

static const u32 godavari_golden_registers[] =
{
	0x1579, 0xff607fff, 0xfc000100,
	0x1bb6, 0x00010101, 0x00010000,
	0x260c, 0xffffffff, 0x00000000,
	0x260c0, 0xf00fffff, 0x00000400,
	0x184c, 0xffffffff, 0x00010000,
	0x16ec, 0x000000f0, 0x00000070,
	0x16f0, 0xf0311fff, 0x80300000,
	0x263e, 0x73773777, 0x12010001,
	0x263f, 0xffffffff, 0x00000010,
	0x200c, 0x00001f0f, 0x0000100a,
	0xbd2, 0x73773777, 0x12010001,
	0x902, 0x000fffff, 0x000c007f,
	0x2285, 0xf000003f, 0x00000007,
	0x22c9, 0xffffffff, 0x00ff0fff,
	0xc281, 0x0000ff0f, 0x00000000,
	0xa293, 0x07ffffff, 0x06000000,
	0x136, 0x00000fff, 0x00000100,
	0x3405, 0x00010000, 0x00810001,
	0x3605, 0x00010000, 0x00810001,
	0xf9e, 0x00000001, 0x00000002,
	0x31da, 0x00000008, 0x00000008,
	0x31dc, 0x00000f00, 0x00000800,
	0x31dd, 0x00000f00, 0x00000800,
	0x31e6, 0x00ffffff, 0x00ff7fbf,
	0x31e7, 0x00ffffff, 0x00ff7faf,
	0x2300, 0x000000ff, 0x00000001,
	0x853e, 0x01ff01ff, 0x00000002,
	0x8526, 0x007ff800, 0x00200000,
	0x8057, 0xffffffff, 0x00000f40,
	0x2231, 0x001f3ae3, 0x00000082,
	0x2235, 0x0000001f, 0x00000010,
	0xc24d, 0xffffffff, 0x00000000
};

static void cik_init_golden_registers(struct amdgpu_device *adev)
{
	/* Some of the registers might be dependent on GRBM_GFX_INDEX */
	mutex_lock(&adev->grbm_idx_mutex);

	switch (adev->asic_type) {
	case CHIP_BONAIRE:
		amdgpu_program_register_sequence(adev,
						 bonaire_mgcg_cgcg_init,
						 (const u32)ARRAY_SIZE(bonaire_mgcg_cgcg_init));
		amdgpu_program_register_sequence(adev,
						 bonaire_golden_registers,
						 (const u32)ARRAY_SIZE(bonaire_golden_registers));
		amdgpu_program_register_sequence(adev,
						 bonaire_golden_common_registers,
						 (const u32)ARRAY_SIZE(bonaire_golden_common_registers));
		amdgpu_program_register_sequence(adev,
						 bonaire_golden_spm_registers,
						 (const u32)ARRAY_SIZE(bonaire_golden_spm_registers));
		break;
	case CHIP_KABINI:
		amdgpu_program_register_sequence(adev,
						 kalindi_mgcg_cgcg_init,
						 (const u32)ARRAY_SIZE(kalindi_mgcg_cgcg_init));
		amdgpu_program_register_sequence(adev,
						 kalindi_golden_registers,
						 (const u32)ARRAY_SIZE(kalindi_golden_registers));
		amdgpu_program_register_sequence(adev,
						 kalindi_golden_common_registers,
						 (const u32)ARRAY_SIZE(kalindi_golden_common_registers));
		amdgpu_program_register_sequence(adev,
						 kalindi_golden_spm_registers,
						 (const u32)ARRAY_SIZE(kalindi_golden_spm_registers));
		break;
	case CHIP_MULLINS:
		amdgpu_program_register_sequence(adev,
						 kalindi_mgcg_cgcg_init,
						 (const u32)ARRAY_SIZE(kalindi_mgcg_cgcg_init));
		amdgpu_program_register_sequence(adev,
						 godavari_golden_registers,
						 (const u32)ARRAY_SIZE(godavari_golden_registers));
		amdgpu_program_register_sequence(adev,
						 kalindi_golden_common_registers,
						 (const u32)ARRAY_SIZE(kalindi_golden_common_registers));
		amdgpu_program_register_sequence(adev,
						 kalindi_golden_spm_registers,
						 (const u32)ARRAY_SIZE(kalindi_golden_spm_registers));
		break;
	case CHIP_KAVERI:
		amdgpu_program_register_sequence(adev,
						 spectre_mgcg_cgcg_init,
						 (const u32)ARRAY_SIZE(spectre_mgcg_cgcg_init));
		amdgpu_program_register_sequence(adev,
						 spectre_golden_registers,
						 (const u32)ARRAY_SIZE(spectre_golden_registers));
		amdgpu_program_register_sequence(adev,
						 spectre_golden_common_registers,
						 (const u32)ARRAY_SIZE(spectre_golden_common_registers));
		amdgpu_program_register_sequence(adev,
						 spectre_golden_spm_registers,
						 (const u32)ARRAY_SIZE(spectre_golden_spm_registers));
		break;
	case CHIP_HAWAII:
		amdgpu_program_register_sequence(adev,
						 hawaii_mgcg_cgcg_init,
						 (const u32)ARRAY_SIZE(hawaii_mgcg_cgcg_init));
		amdgpu_program_register_sequence(adev,
						 hawaii_golden_registers,
						 (const u32)ARRAY_SIZE(hawaii_golden_registers));
		amdgpu_program_register_sequence(adev,
						 hawaii_golden_common_registers,
						 (const u32)ARRAY_SIZE(hawaii_golden_common_registers));
		amdgpu_program_register_sequence(adev,
						 hawaii_golden_spm_registers,
						 (const u32)ARRAY_SIZE(hawaii_golden_spm_registers));
		break;
	default:
		break;
	}
	mutex_unlock(&adev->grbm_idx_mutex);
}

/**
 * cik_get_xclk - get the xclk
 *
 * @adev: amdgpu_device pointer
 *
 * Returns the reference clock used by the gfx engine
 * (CIK).
 */
static u32 cik_get_xclk(struct amdgpu_device *adev)
{
	u32 reference_clock = adev->clock.spll.reference_freq;

	if (adev->flags & AMD_IS_APU) {
		if (RREG32_SMC(ixGENERAL_PWRMGT) & GENERAL_PWRMGT__GPU_COUNTER_CLK_MASK)
			return reference_clock / 2;
	} else {
		if (RREG32_SMC(ixCG_CLKPIN_CNTL) & CG_CLKPIN_CNTL__XTALIN_DIVIDE_MASK)
			return reference_clock / 4;
	}
	return reference_clock;
}

/**
 * cik_srbm_select - select specific register instances
 *
 * @adev: amdgpu_device pointer
 * @me: selected ME (micro engine)
 * @pipe: pipe
 * @queue: queue
 * @vmid: VMID
 *
 * Switches the currently active registers instances.  Some
 * registers are instanced per VMID, others are instanced per
 * me/pipe/queue combination.
 */
void cik_srbm_select(struct amdgpu_device *adev,
		     u32 me, u32 pipe, u32 queue, u32 vmid)
{
	u32 srbm_gfx_cntl =
		(((pipe << SRBM_GFX_CNTL__PIPEID__SHIFT) & SRBM_GFX_CNTL__PIPEID_MASK)|
		((me << SRBM_GFX_CNTL__MEID__SHIFT) & SRBM_GFX_CNTL__MEID_MASK)|
		((vmid << SRBM_GFX_CNTL__VMID__SHIFT) & SRBM_GFX_CNTL__VMID_MASK)|
		((queue << SRBM_GFX_CNTL__QUEUEID__SHIFT) & SRBM_GFX_CNTL__QUEUEID_MASK));
	WREG32(mmSRBM_GFX_CNTL, srbm_gfx_cntl);
}

static void cik_vga_set_state(struct amdgpu_device *adev, bool state)
{
	uint32_t tmp;

	tmp = RREG32(mmCONFIG_CNTL);
	if (state == false)
		tmp |= CONFIG_CNTL__VGA_DIS_MASK;
	else
		tmp &= ~CONFIG_CNTL__VGA_DIS_MASK;
	WREG32(mmCONFIG_CNTL, tmp);
}

static bool cik_read_disabled_bios(struct amdgpu_device *adev)
{
	u32 bus_cntl;
	u32 d1vga_control = 0;
	u32 d2vga_control = 0;
	u32 vga_render_control = 0;
	u32 rom_cntl;
	bool r;

	bus_cntl = RREG32(mmBUS_CNTL);
	if (adev->mode_info.num_crtc) {
		d1vga_control = RREG32(mmD1VGA_CONTROL);
		d2vga_control = RREG32(mmD2VGA_CONTROL);
		vga_render_control = RREG32(mmVGA_RENDER_CONTROL);
	}
	rom_cntl = RREG32_SMC(ixROM_CNTL);

	/* enable the rom */
	WREG32(mmBUS_CNTL, (bus_cntl & ~BUS_CNTL__BIOS_ROM_DIS_MASK));
	if (adev->mode_info.num_crtc) {
		/* Disable VGA mode */
		WREG32(mmD1VGA_CONTROL,
		       (d1vga_control & ~(D1VGA_CONTROL__D1VGA_MODE_ENABLE_MASK |
					  D1VGA_CONTROL__D1VGA_TIMING_SELECT_MASK)));
		WREG32(mmD2VGA_CONTROL,
		       (d2vga_control & ~(D1VGA_CONTROL__D1VGA_MODE_ENABLE_MASK |
					  D1VGA_CONTROL__D1VGA_TIMING_SELECT_MASK)));
		WREG32(mmVGA_RENDER_CONTROL,
		       (vga_render_control & ~VGA_RENDER_CONTROL__VGA_VSTATUS_CNTL_MASK));
	}
	WREG32_SMC(ixROM_CNTL, rom_cntl | ROM_CNTL__SCK_OVERWRITE_MASK);

	r = amdgpu_read_bios(adev);

	/* restore regs */
	WREG32(mmBUS_CNTL, bus_cntl);
	if (adev->mode_info.num_crtc) {
		WREG32(mmD1VGA_CONTROL, d1vga_control);
		WREG32(mmD2VGA_CONTROL, d2vga_control);
		WREG32(mmVGA_RENDER_CONTROL, vga_render_control);
	}
	WREG32_SMC(ixROM_CNTL, rom_cntl);
	return r;
}

static struct amdgpu_allowed_register_entry cik_allowed_read_registers[] = {
	{mmGRBM_STATUS, false},
	{mmGB_ADDR_CONFIG, false},
	{mmMC_ARB_RAMCFG, false},
	{mmGB_TILE_MODE0, false},
	{mmGB_TILE_MODE1, false},
	{mmGB_TILE_MODE2, false},
	{mmGB_TILE_MODE3, false},
	{mmGB_TILE_MODE4, false},
	{mmGB_TILE_MODE5, false},
	{mmGB_TILE_MODE6, false},
	{mmGB_TILE_MODE7, false},
	{mmGB_TILE_MODE8, false},
	{mmGB_TILE_MODE9, false},
	{mmGB_TILE_MODE10, false},
	{mmGB_TILE_MODE11, false},
	{mmGB_TILE_MODE12, false},
	{mmGB_TILE_MODE13, false},
	{mmGB_TILE_MODE14, false},
	{mmGB_TILE_MODE15, false},
	{mmGB_TILE_MODE16, false},
	{mmGB_TILE_MODE17, false},
	{mmGB_TILE_MODE18, false},
	{mmGB_TILE_MODE19, false},
	{mmGB_TILE_MODE20, false},
	{mmGB_TILE_MODE21, false},
	{mmGB_TILE_MODE22, false},
	{mmGB_TILE_MODE23, false},
	{mmGB_TILE_MODE24, false},
	{mmGB_TILE_MODE25, false},
	{mmGB_TILE_MODE26, false},
	{mmGB_TILE_MODE27, false},
	{mmGB_TILE_MODE28, false},
	{mmGB_TILE_MODE29, false},
	{mmGB_TILE_MODE30, false},
	{mmGB_TILE_MODE31, false},
	{mmGB_MACROTILE_MODE0, false},
	{mmGB_MACROTILE_MODE1, false},
	{mmGB_MACROTILE_MODE2, false},
	{mmGB_MACROTILE_MODE3, false},
	{mmGB_MACROTILE_MODE4, false},
	{mmGB_MACROTILE_MODE5, false},
	{mmGB_MACROTILE_MODE6, false},
	{mmGB_MACROTILE_MODE7, false},
	{mmGB_MACROTILE_MODE8, false},
	{mmGB_MACROTILE_MODE9, false},
	{mmGB_MACROTILE_MODE10, false},
	{mmGB_MACROTILE_MODE11, false},
	{mmGB_MACROTILE_MODE12, false},
	{mmGB_MACROTILE_MODE13, false},
	{mmGB_MACROTILE_MODE14, false},
	{mmGB_MACROTILE_MODE15, false},
	{mmCC_RB_BACKEND_DISABLE, false, true},
	{mmGC_USER_RB_BACKEND_DISABLE, false, true},
	{mmGB_BACKEND_MAP, false, false},
	{mmPA_SC_RASTER_CONFIG, false, true},
	{mmPA_SC_RASTER_CONFIG_1, false, true},
};

static uint32_t cik_read_indexed_register(struct amdgpu_device *adev,
					  u32 se_num, u32 sh_num,
					  u32 reg_offset)
{
	uint32_t val;

	mutex_lock(&adev->grbm_idx_mutex);
	if (se_num != 0xffffffff || sh_num != 0xffffffff)
		gfx_v7_0_select_se_sh(adev, se_num, sh_num);

	val = RREG32(reg_offset);

	if (se_num != 0xffffffff || sh_num != 0xffffffff)
		gfx_v7_0_select_se_sh(adev, 0xffffffff, 0xffffffff);
	mutex_unlock(&adev->grbm_idx_mutex);
	return val;
}

static int cik_read_register(struct amdgpu_device *adev, u32 se_num,
			     u32 sh_num, u32 reg_offset, u32 *value)
{
	uint32_t i;

	*value = 0;
	for (i = 0; i < ARRAY_SIZE(cik_allowed_read_registers); i++) {
		if (reg_offset != cik_allowed_read_registers[i].reg_offset)
			continue;

		if (!cik_allowed_read_registers[i].untouched)
			*value = cik_allowed_read_registers[i].grbm_indexed ?
				 cik_read_indexed_register(adev, se_num,
							   sh_num, reg_offset) :
				 RREG32(reg_offset);
		return 0;
	}
	return -EINVAL;
}

static void cik_print_gpu_status_regs(struct amdgpu_device *adev)
{
	dev_info(adev->dev, "  GRBM_STATUS=0x%08X\n",
		RREG32(mmGRBM_STATUS));
	dev_info(adev->dev, "  GRBM_STATUS2=0x%08X\n",
		RREG32(mmGRBM_STATUS2));
	dev_info(adev->dev, "  GRBM_STATUS_SE0=0x%08X\n",
		RREG32(mmGRBM_STATUS_SE0));
	dev_info(adev->dev, "  GRBM_STATUS_SE1=0x%08X\n",
		RREG32(mmGRBM_STATUS_SE1));
	dev_info(adev->dev, "  GRBM_STATUS_SE2=0x%08X\n",
		RREG32(mmGRBM_STATUS_SE2));
	dev_info(adev->dev, "  GRBM_STATUS_SE3=0x%08X\n",
		RREG32(mmGRBM_STATUS_SE3));
	dev_info(adev->dev, "  SRBM_STATUS=0x%08X\n",
		RREG32(mmSRBM_STATUS));
	dev_info(adev->dev, "  SRBM_STATUS2=0x%08X\n",
		RREG32(mmSRBM_STATUS2));
	dev_info(adev->dev, "  SDMA0_STATUS_REG   = 0x%08X\n",
		RREG32(mmSDMA0_STATUS_REG + SDMA0_REGISTER_OFFSET));
	dev_info(adev->dev, "  SDMA1_STATUS_REG   = 0x%08X\n",
		 RREG32(mmSDMA0_STATUS_REG + SDMA1_REGISTER_OFFSET));
	dev_info(adev->dev, "  CP_STAT = 0x%08x\n", RREG32(mmCP_STAT));
	dev_info(adev->dev, "  CP_STALLED_STAT1 = 0x%08x\n",
		 RREG32(mmCP_STALLED_STAT1));
	dev_info(adev->dev, "  CP_STALLED_STAT2 = 0x%08x\n",
		 RREG32(mmCP_STALLED_STAT2));
	dev_info(adev->dev, "  CP_STALLED_STAT3 = 0x%08x\n",
		 RREG32(mmCP_STALLED_STAT3));
	dev_info(adev->dev, "  CP_CPF_BUSY_STAT = 0x%08x\n",
		 RREG32(mmCP_CPF_BUSY_STAT));
	dev_info(adev->dev, "  CP_CPF_STALLED_STAT1 = 0x%08x\n",
		 RREG32(mmCP_CPF_STALLED_STAT1));
	dev_info(adev->dev, "  CP_CPF_STATUS = 0x%08x\n", RREG32(mmCP_CPF_STATUS));
	dev_info(adev->dev, "  CP_CPC_BUSY_STAT = 0x%08x\n", RREG32(mmCP_CPC_BUSY_STAT));
	dev_info(adev->dev, "  CP_CPC_STALLED_STAT1 = 0x%08x\n",
		 RREG32(mmCP_CPC_STALLED_STAT1));
	dev_info(adev->dev, "  CP_CPC_STATUS = 0x%08x\n", RREG32(mmCP_CPC_STATUS));
}

/**
 * cik_gpu_check_soft_reset - check which blocks are busy
 *
 * @adev: amdgpu_device pointer
 *
 * Check which blocks are busy and return the relevant reset
 * mask to be used by cik_gpu_soft_reset().
 * Returns a mask of the blocks to be reset.
 */
u32 amdgpu_cik_gpu_check_soft_reset(struct amdgpu_device *adev); /* XXX put this in a header file */
u32 amdgpu_cik_gpu_check_soft_reset(struct amdgpu_device *adev)
{
	u32 reset_mask = 0;
	u32 tmp;

	/* GRBM_STATUS */
	tmp = RREG32(mmGRBM_STATUS);
	if (tmp & (GRBM_STATUS__PA_BUSY_MASK | GRBM_STATUS__SC_BUSY_MASK |
		   GRBM_STATUS__BCI_BUSY_MASK | GRBM_STATUS__SX_BUSY_MASK |
		   GRBM_STATUS__TA_BUSY_MASK | GRBM_STATUS__VGT_BUSY_MASK |
		   GRBM_STATUS__DB_BUSY_MASK | GRBM_STATUS__CB_BUSY_MASK |
		   GRBM_STATUS__GDS_BUSY_MASK | GRBM_STATUS__SPI_BUSY_MASK |
		   GRBM_STATUS__IA_BUSY_MASK | GRBM_STATUS__IA_BUSY_NO_DMA_MASK))
		reset_mask |= AMDGPU_RESET_GFX;

	if (tmp & (GRBM_STATUS__CP_BUSY_MASK | GRBM_STATUS__CP_COHERENCY_BUSY_MASK))
		reset_mask |= AMDGPU_RESET_CP;

	/* GRBM_STATUS2 */
	tmp = RREG32(mmGRBM_STATUS2);
	if (tmp & GRBM_STATUS2__RLC_BUSY_MASK)
		reset_mask |= AMDGPU_RESET_RLC;

	/* SDMA0_STATUS_REG */
	tmp = RREG32(mmSDMA0_STATUS_REG + SDMA0_REGISTER_OFFSET);
	if (!(tmp & SDMA0_STATUS_REG__IDLE_MASK))
		reset_mask |= AMDGPU_RESET_DMA;

	/* SDMA1_STATUS_REG */
	tmp = RREG32(mmSDMA0_STATUS_REG + SDMA1_REGISTER_OFFSET);
	if (!(tmp & SDMA0_STATUS_REG__IDLE_MASK))
		reset_mask |= AMDGPU_RESET_DMA1;

	/* SRBM_STATUS2 */
	tmp = RREG32(mmSRBM_STATUS2);
	if (tmp & SRBM_STATUS2__SDMA_BUSY_MASK)
		reset_mask |= AMDGPU_RESET_DMA;

	if (tmp & SRBM_STATUS2__SDMA1_BUSY_MASK)
		reset_mask |= AMDGPU_RESET_DMA1;

	/* SRBM_STATUS */
	tmp = RREG32(mmSRBM_STATUS);

	if (tmp & SRBM_STATUS__IH_BUSY_MASK)
		reset_mask |= AMDGPU_RESET_IH;

	if (tmp & SRBM_STATUS__SEM_BUSY_MASK)
		reset_mask |= AMDGPU_RESET_SEM;

	if (tmp & SRBM_STATUS__GRBM_RQ_PENDING_MASK)
		reset_mask |= AMDGPU_RESET_GRBM;

	if (tmp & SRBM_STATUS__VMC_BUSY_MASK)
		reset_mask |= AMDGPU_RESET_VMC;

	if (tmp & (SRBM_STATUS__MCB_BUSY_MASK | SRBM_STATUS__MCB_NON_DISPLAY_BUSY_MASK |
		   SRBM_STATUS__MCC_BUSY_MASK | SRBM_STATUS__MCD_BUSY_MASK))
		reset_mask |= AMDGPU_RESET_MC;

	if (amdgpu_display_is_display_hung(adev))
		reset_mask |= AMDGPU_RESET_DISPLAY;

	/* Skip MC reset as it's mostly likely not hung, just busy */
	if (reset_mask & AMDGPU_RESET_MC) {
		DRM_DEBUG("MC busy: 0x%08X, clearing.\n", reset_mask);
		reset_mask &= ~AMDGPU_RESET_MC;
	}

	return reset_mask;
}

/**
 * cik_gpu_soft_reset - soft reset GPU
 *
 * @adev: amdgpu_device pointer
 * @reset_mask: mask of which blocks to reset
 *
 * Soft reset the blocks specified in @reset_mask.
 */
static void cik_gpu_soft_reset(struct amdgpu_device *adev, u32 reset_mask)
{
	struct amdgpu_mode_mc_save save;
	u32 grbm_soft_reset = 0, srbm_soft_reset = 0;
	u32 tmp;

	if (reset_mask == 0)
		return;

	dev_info(adev->dev, "GPU softreset: 0x%08X\n", reset_mask);

	cik_print_gpu_status_regs(adev);
	dev_info(adev->dev, "  VM_CONTEXT1_PROTECTION_FAULT_ADDR   0x%08X\n",
		 RREG32(mmVM_CONTEXT1_PROTECTION_FAULT_ADDR));
	dev_info(adev->dev, "  VM_CONTEXT1_PROTECTION_FAULT_STATUS 0x%08X\n",
		 RREG32(mmVM_CONTEXT1_PROTECTION_FAULT_STATUS));

	/* disable CG/PG */

	/* stop the rlc */
	gfx_v7_0_rlc_stop(adev);

	/* Disable GFX parsing/prefetching */
	WREG32(mmCP_ME_CNTL, CP_ME_CNTL__ME_HALT_MASK | CP_ME_CNTL__PFP_HALT_MASK | CP_ME_CNTL__CE_HALT_MASK);

	/* Disable MEC parsing/prefetching */
	WREG32(mmCP_MEC_CNTL, CP_MEC_CNTL__MEC_ME1_HALT_MASK | CP_MEC_CNTL__MEC_ME2_HALT_MASK);

	if (reset_mask & AMDGPU_RESET_DMA) {
		/* sdma0 */
		tmp = RREG32(mmSDMA0_F32_CNTL + SDMA0_REGISTER_OFFSET);
		tmp |= SDMA0_F32_CNTL__HALT_MASK;
		WREG32(mmSDMA0_F32_CNTL + SDMA0_REGISTER_OFFSET, tmp);
	}
	if (reset_mask & AMDGPU_RESET_DMA1) {
		/* sdma1 */
		tmp = RREG32(mmSDMA0_F32_CNTL + SDMA1_REGISTER_OFFSET);
		tmp |= SDMA0_F32_CNTL__HALT_MASK;
		WREG32(mmSDMA0_F32_CNTL + SDMA1_REGISTER_OFFSET, tmp);
	}

	gmc_v7_0_mc_stop(adev, &save);
	if (amdgpu_asic_wait_for_mc_idle(adev)) {
		dev_warn(adev->dev, "Wait for MC idle timedout !\n");
	}

	if (reset_mask & (AMDGPU_RESET_GFX | AMDGPU_RESET_COMPUTE | AMDGPU_RESET_CP))
		grbm_soft_reset = GRBM_SOFT_RESET__SOFT_RESET_CP_MASK |
			GRBM_SOFT_RESET__SOFT_RESET_GFX_MASK;

	if (reset_mask & AMDGPU_RESET_CP) {
		grbm_soft_reset |= GRBM_SOFT_RESET__SOFT_RESET_CP_MASK;

		srbm_soft_reset |= SRBM_SOFT_RESET__SOFT_RESET_GRBM_MASK;
	}

	if (reset_mask & AMDGPU_RESET_DMA)
		srbm_soft_reset |= SRBM_SOFT_RESET__SOFT_RESET_SDMA_MASK;

	if (reset_mask & AMDGPU_RESET_DMA1)
		srbm_soft_reset |= SRBM_SOFT_RESET__SOFT_RESET_SDMA1_MASK;

	if (reset_mask & AMDGPU_RESET_DISPLAY)
		srbm_soft_reset |= SRBM_SOFT_RESET__SOFT_RESET_DC_MASK;

	if (reset_mask & AMDGPU_RESET_RLC)
		grbm_soft_reset |= GRBM_SOFT_RESET__SOFT_RESET_RLC_MASK;

	if (reset_mask & AMDGPU_RESET_SEM)
		srbm_soft_reset |= SRBM_SOFT_RESET__SOFT_RESET_SEM_MASK;

	if (reset_mask & AMDGPU_RESET_IH)
		srbm_soft_reset |= SRBM_SOFT_RESET__SOFT_RESET_IH_MASK;

	if (reset_mask & AMDGPU_RESET_GRBM)
		srbm_soft_reset |= SRBM_SOFT_RESET__SOFT_RESET_GRBM_MASK;

	if (reset_mask & AMDGPU_RESET_VMC)
		srbm_soft_reset |= SRBM_SOFT_RESET__SOFT_RESET_VMC_MASK;

	if (!(adev->flags & AMD_IS_APU)) {
		if (reset_mask & AMDGPU_RESET_MC)
			srbm_soft_reset |= SRBM_SOFT_RESET__SOFT_RESET_MC_MASK;
	}

	if (grbm_soft_reset) {
		tmp = RREG32(mmGRBM_SOFT_RESET);
		tmp |= grbm_soft_reset;
		dev_info(adev->dev, "GRBM_SOFT_RESET=0x%08X\n", tmp);
		WREG32(mmGRBM_SOFT_RESET, tmp);
		tmp = RREG32(mmGRBM_SOFT_RESET);

		udelay(50);

		tmp &= ~grbm_soft_reset;
		WREG32(mmGRBM_SOFT_RESET, tmp);
		tmp = RREG32(mmGRBM_SOFT_RESET);
	}

	if (srbm_soft_reset) {
		tmp = RREG32(mmSRBM_SOFT_RESET);
		tmp |= srbm_soft_reset;
		dev_info(adev->dev, "SRBM_SOFT_RESET=0x%08X\n", tmp);
		WREG32(mmSRBM_SOFT_RESET, tmp);
		tmp = RREG32(mmSRBM_SOFT_RESET);

		udelay(50);

		tmp &= ~srbm_soft_reset;
		WREG32(mmSRBM_SOFT_RESET, tmp);
		tmp = RREG32(mmSRBM_SOFT_RESET);
	}

	/* Wait a little for things to settle down */
	udelay(50);

	gmc_v7_0_mc_resume(adev, &save);
	udelay(50);

	cik_print_gpu_status_regs(adev);
}

struct kv_reset_save_regs {
	u32 gmcon_reng_execute;
	u32 gmcon_misc;
	u32 gmcon_misc3;
};

static void kv_save_regs_for_reset(struct amdgpu_device *adev,
				   struct kv_reset_save_regs *save)
{
	save->gmcon_reng_execute = RREG32(mmGMCON_RENG_EXECUTE);
	save->gmcon_misc = RREG32(mmGMCON_MISC);
	save->gmcon_misc3 = RREG32(mmGMCON_MISC3);

	WREG32(mmGMCON_RENG_EXECUTE, save->gmcon_reng_execute &
		~GMCON_RENG_EXECUTE__RENG_EXECUTE_ON_PWR_UP_MASK);
	WREG32(mmGMCON_MISC, save->gmcon_misc &
		~(GMCON_MISC__RENG_EXECUTE_ON_REG_UPDATE_MASK |
			GMCON_MISC__STCTRL_STUTTER_EN_MASK));
}

static void kv_restore_regs_for_reset(struct amdgpu_device *adev,
				      struct kv_reset_save_regs *save)
{
	int i;

	WREG32(mmGMCON_PGFSM_WRITE, 0);
	WREG32(mmGMCON_PGFSM_CONFIG, 0x200010ff);

	for (i = 0; i < 5; i++)
		WREG32(mmGMCON_PGFSM_WRITE, 0);

	WREG32(mmGMCON_PGFSM_WRITE, 0);
	WREG32(mmGMCON_PGFSM_CONFIG, 0x300010ff);

	for (i = 0; i < 5; i++)
		WREG32(mmGMCON_PGFSM_WRITE, 0);

	WREG32(mmGMCON_PGFSM_WRITE, 0x210000);
	WREG32(mmGMCON_PGFSM_CONFIG, 0xa00010ff);

	for (i = 0; i < 5; i++)
		WREG32(mmGMCON_PGFSM_WRITE, 0);

	WREG32(mmGMCON_PGFSM_WRITE, 0x21003);
	WREG32(mmGMCON_PGFSM_CONFIG, 0xb00010ff);

	for (i = 0; i < 5; i++)
		WREG32(mmGMCON_PGFSM_WRITE, 0);

	WREG32(mmGMCON_PGFSM_WRITE, 0x2b00);
	WREG32(mmGMCON_PGFSM_CONFIG, 0xc00010ff);

	for (i = 0; i < 5; i++)
		WREG32(mmGMCON_PGFSM_WRITE, 0);

	WREG32(mmGMCON_PGFSM_WRITE, 0);
	WREG32(mmGMCON_PGFSM_CONFIG, 0xd00010ff);

	for (i = 0; i < 5; i++)
		WREG32(mmGMCON_PGFSM_WRITE, 0);

	WREG32(mmGMCON_PGFSM_WRITE, 0x420000);
	WREG32(mmGMCON_PGFSM_CONFIG, 0x100010ff);

	for (i = 0; i < 5; i++)
		WREG32(mmGMCON_PGFSM_WRITE, 0);

	WREG32(mmGMCON_PGFSM_WRITE, 0x120202);
	WREG32(mmGMCON_PGFSM_CONFIG, 0x500010ff);

	for (i = 0; i < 5; i++)
		WREG32(mmGMCON_PGFSM_WRITE, 0);

	WREG32(mmGMCON_PGFSM_WRITE, 0x3e3e36);
	WREG32(mmGMCON_PGFSM_CONFIG, 0x600010ff);

	for (i = 0; i < 5; i++)
		WREG32(mmGMCON_PGFSM_WRITE, 0);

	WREG32(mmGMCON_PGFSM_WRITE, 0x373f3e);
	WREG32(mmGMCON_PGFSM_CONFIG, 0x700010ff);

	for (i = 0; i < 5; i++)
		WREG32(mmGMCON_PGFSM_WRITE, 0);

	WREG32(mmGMCON_PGFSM_WRITE, 0x3e1332);
	WREG32(mmGMCON_PGFSM_CONFIG, 0xe00010ff);

	WREG32(mmGMCON_MISC3, save->gmcon_misc3);
	WREG32(mmGMCON_MISC, save->gmcon_misc);
	WREG32(mmGMCON_RENG_EXECUTE, save->gmcon_reng_execute);
}

static void cik_gpu_pci_config_reset(struct amdgpu_device *adev)
{
	struct amdgpu_mode_mc_save save;
	struct kv_reset_save_regs kv_save = { 0 };
	u32 tmp, i;

	dev_info(adev->dev, "GPU pci config reset\n");

	/* disable dpm? */

	/* disable cg/pg */

	/* Disable GFX parsing/prefetching */
	WREG32(mmCP_ME_CNTL, CP_ME_CNTL__ME_HALT_MASK |
		CP_ME_CNTL__PFP_HALT_MASK | CP_ME_CNTL__CE_HALT_MASK);

	/* Disable MEC parsing/prefetching */
	WREG32(mmCP_MEC_CNTL,
			CP_MEC_CNTL__MEC_ME1_HALT_MASK | CP_MEC_CNTL__MEC_ME2_HALT_MASK);

	/* sdma0 */
	tmp = RREG32(mmSDMA0_F32_CNTL + SDMA0_REGISTER_OFFSET);
	tmp |= SDMA0_F32_CNTL__HALT_MASK;
	WREG32(mmSDMA0_F32_CNTL + SDMA0_REGISTER_OFFSET, tmp);
	/* sdma1 */
	tmp = RREG32(mmSDMA0_F32_CNTL + SDMA1_REGISTER_OFFSET);
	tmp |= SDMA0_F32_CNTL__HALT_MASK;
	WREG32(mmSDMA0_F32_CNTL + SDMA1_REGISTER_OFFSET, tmp);
	/* XXX other engines? */

	/* halt the rlc, disable cp internal ints */
	gfx_v7_0_rlc_stop(adev);

	udelay(50);

	/* disable mem access */
	gmc_v7_0_mc_stop(adev, &save);
	if (amdgpu_asic_wait_for_mc_idle(adev)) {
		dev_warn(adev->dev, "Wait for MC idle timed out !\n");
	}

	if (adev->flags & AMD_IS_APU)
		kv_save_regs_for_reset(adev, &kv_save);

	/* disable BM */
	pci_clear_master(adev->pdev);
	/* reset */
	amdgpu_pci_config_reset(adev);

	udelay(100);

	/* wait for asic to come out of reset */
	for (i = 0; i < adev->usec_timeout; i++) {
		if (RREG32(mmCONFIG_MEMSIZE) != 0xffffffff)
			break;
		udelay(1);
	}

	/* does asic init need to be run first??? */
	if (adev->flags & AMD_IS_APU)
		kv_restore_regs_for_reset(adev, &kv_save);
}

static void cik_set_bios_scratch_engine_hung(struct amdgpu_device *adev, bool hung)
{
	u32 tmp = RREG32(mmBIOS_SCRATCH_3);

	if (hung)
		tmp |= ATOM_S3_ASIC_GUI_ENGINE_HUNG;
	else
		tmp &= ~ATOM_S3_ASIC_GUI_ENGINE_HUNG;

	WREG32(mmBIOS_SCRATCH_3, tmp);
}

/**
 * cik_asic_reset - soft reset GPU
 *
 * @adev: amdgpu_device pointer
 *
 * Look up which blocks are hung and attempt
 * to reset them.
 * Returns 0 for success.
 */
static int cik_asic_reset(struct amdgpu_device *adev)
{
	u32 reset_mask;

	reset_mask = amdgpu_cik_gpu_check_soft_reset(adev);

	if (reset_mask)
		cik_set_bios_scratch_engine_hung(adev, true);

	/* try soft reset */
	cik_gpu_soft_reset(adev, reset_mask);

	reset_mask = amdgpu_cik_gpu_check_soft_reset(adev);

	/* try pci config reset */
	if (reset_mask && amdgpu_hard_reset)
		cik_gpu_pci_config_reset(adev);

	reset_mask = amdgpu_cik_gpu_check_soft_reset(adev);

	if (!reset_mask)
		cik_set_bios_scratch_engine_hung(adev, false);

	return 0;
}

static int cik_set_uvd_clock(struct amdgpu_device *adev, u32 clock,
			      u32 cntl_reg, u32 status_reg)
{
	int r, i;
	struct atom_clock_dividers dividers;
	uint32_t tmp;

	r = amdgpu_atombios_get_clock_dividers(adev,
					       COMPUTE_GPUCLK_INPUT_FLAG_DEFAULT_GPUCLK,
					       clock, false, &dividers);
	if (r)
		return r;

	tmp = RREG32_SMC(cntl_reg);
	tmp &= ~(CG_DCLK_CNTL__DCLK_DIR_CNTL_EN_MASK |
		CG_DCLK_CNTL__DCLK_DIVIDER_MASK);
	tmp |= dividers.post_divider;
	WREG32_SMC(cntl_reg, tmp);

	for (i = 0; i < 100; i++) {
		if (RREG32_SMC(status_reg) & CG_DCLK_STATUS__DCLK_STATUS_MASK)
			break;
		mdelay(10);
	}
	if (i == 100)
		return -ETIMEDOUT;

	return 0;
}

static int cik_set_uvd_clocks(struct amdgpu_device *adev, u32 vclk, u32 dclk)
{
	int r = 0;

	r = cik_set_uvd_clock(adev, vclk, ixCG_VCLK_CNTL, ixCG_VCLK_STATUS);
	if (r)
		return r;

	r = cik_set_uvd_clock(adev, dclk, ixCG_DCLK_CNTL, ixCG_DCLK_STATUS);
	return r;
}

static int cik_set_vce_clocks(struct amdgpu_device *adev, u32 evclk, u32 ecclk)
{
	int r, i;
	struct atom_clock_dividers dividers;
	u32 tmp;

	r = amdgpu_atombios_get_clock_dividers(adev,
					       COMPUTE_GPUCLK_INPUT_FLAG_DEFAULT_GPUCLK,
					       ecclk, false, &dividers);
	if (r)
		return r;

	for (i = 0; i < 100; i++) {
		if (RREG32_SMC(ixCG_ECLK_STATUS) & CG_ECLK_STATUS__ECLK_STATUS_MASK)
			break;
		mdelay(10);
	}
	if (i == 100)
		return -ETIMEDOUT;

	tmp = RREG32_SMC(ixCG_ECLK_CNTL);
	tmp &= ~(CG_ECLK_CNTL__ECLK_DIR_CNTL_EN_MASK |
		CG_ECLK_CNTL__ECLK_DIVIDER_MASK);
	tmp |= dividers.post_divider;
	WREG32_SMC(ixCG_ECLK_CNTL, tmp);

	for (i = 0; i < 100; i++) {
		if (RREG32_SMC(ixCG_ECLK_STATUS) & CG_ECLK_STATUS__ECLK_STATUS_MASK)
			break;
		mdelay(10);
	}
	if (i == 100)
		return -ETIMEDOUT;

	return 0;
}

static void cik_pcie_gen3_enable(struct amdgpu_device *adev)
{
#ifndef __NetBSD__		/* XXX amdgpu pcie */
	struct pci_dev *root = adev->pdev->bus->self;
	int bridge_pos, gpu_pos;
	u32 speed_cntl, mask, current_data_rate;
	int ret, i;
	u16 tmp16;

	if (pci_is_root_bus(adev->pdev->bus))
		return;

	if (amdgpu_pcie_gen2 == 0)
		return;

	if (adev->flags & AMD_IS_APU)
		return;

	ret = drm_pcie_get_speed_cap_mask(adev->ddev, &mask);
	if (ret != 0)
		return;

	if (!(mask & (DRM_PCIE_SPEED_50 | DRM_PCIE_SPEED_80)))
		return;

	speed_cntl = RREG32_PCIE(ixPCIE_LC_SPEED_CNTL);
	current_data_rate = (speed_cntl & PCIE_LC_SPEED_CNTL__LC_CURRENT_DATA_RATE_MASK) >>
		PCIE_LC_SPEED_CNTL__LC_CURRENT_DATA_RATE__SHIFT;
	if (mask & DRM_PCIE_SPEED_80) {
		if (current_data_rate == 2) {
			DRM_INFO("PCIE gen 3 link speeds already enabled\n");
			return;
		}
		DRM_INFO("enabling PCIE gen 3 link speeds, disable with amdgpu.pcie_gen2=0\n");
	} else if (mask & DRM_PCIE_SPEED_50) {
		if (current_data_rate == 1) {
			DRM_INFO("PCIE gen 2 link speeds already enabled\n");
			return;
		}
		DRM_INFO("enabling PCIE gen 2 link speeds, disable with amdgpu.pcie_gen2=0\n");
	}

	bridge_pos = pci_pcie_cap(root);
	if (!bridge_pos)
		return;

	gpu_pos = pci_pcie_cap(adev->pdev);
	if (!gpu_pos)
		return;

	if (mask & DRM_PCIE_SPEED_80) {
		/* re-try equalization if gen3 is not already enabled */
		if (current_data_rate != 2) {
			u16 bridge_cfg, gpu_cfg;
			u16 bridge_cfg2, gpu_cfg2;
			u32 max_lw, current_lw, tmp;

			pci_read_config_word(root, bridge_pos + PCI_EXP_LNKCTL, &bridge_cfg);
			pci_read_config_word(adev->pdev, gpu_pos + PCI_EXP_LNKCTL, &gpu_cfg);

			tmp16 = bridge_cfg | PCI_EXP_LNKCTL_HAWD;
			pci_write_config_word(root, bridge_pos + PCI_EXP_LNKCTL, tmp16);

			tmp16 = gpu_cfg | PCI_EXP_LNKCTL_HAWD;
			pci_write_config_word(adev->pdev, gpu_pos + PCI_EXP_LNKCTL, tmp16);

			tmp = RREG32_PCIE(ixPCIE_LC_STATUS1);
			max_lw = (tmp & PCIE_LC_STATUS1__LC_DETECTED_LINK_WIDTH_MASK) >>
				PCIE_LC_STATUS1__LC_DETECTED_LINK_WIDTH__SHIFT;
			current_lw = (tmp & PCIE_LC_STATUS1__LC_OPERATING_LINK_WIDTH_MASK)
				>> PCIE_LC_STATUS1__LC_OPERATING_LINK_WIDTH__SHIFT;

			if (current_lw < max_lw) {
				tmp = RREG32_PCIE(ixPCIE_LC_LINK_WIDTH_CNTL);
				if (tmp & PCIE_LC_LINK_WIDTH_CNTL__LC_RENEGOTIATION_SUPPORT_MASK) {
					tmp &= ~(PCIE_LC_LINK_WIDTH_CNTL__LC_LINK_WIDTH_MASK |
						PCIE_LC_LINK_WIDTH_CNTL__LC_UPCONFIGURE_DIS_MASK);
					tmp |= (max_lw <<
						PCIE_LC_LINK_WIDTH_CNTL__LC_LINK_WIDTH__SHIFT);
					tmp |= PCIE_LC_LINK_WIDTH_CNTL__LC_UPCONFIGURE_SUPPORT_MASK |
					PCIE_LC_LINK_WIDTH_CNTL__LC_RENEGOTIATE_EN_MASK |
					PCIE_LC_LINK_WIDTH_CNTL__LC_RECONFIG_NOW_MASK;
					WREG32_PCIE(ixPCIE_LC_LINK_WIDTH_CNTL, tmp);
				}
			}

			for (i = 0; i < 10; i++) {
				/* check status */
				pci_read_config_word(adev->pdev, gpu_pos + PCI_EXP_DEVSTA, &tmp16);
				if (tmp16 & PCI_EXP_DEVSTA_TRPND)
					break;

				pci_read_config_word(root, bridge_pos + PCI_EXP_LNKCTL, &bridge_cfg);
				pci_read_config_word(adev->pdev, gpu_pos + PCI_EXP_LNKCTL, &gpu_cfg);

				pci_read_config_word(root, bridge_pos + PCI_EXP_LNKCTL2, &bridge_cfg2);
				pci_read_config_word(adev->pdev, gpu_pos + PCI_EXP_LNKCTL2, &gpu_cfg2);

				tmp = RREG32_PCIE(ixPCIE_LC_CNTL4);
				tmp |= PCIE_LC_CNTL4__LC_SET_QUIESCE_MASK;
				WREG32_PCIE(ixPCIE_LC_CNTL4, tmp);

				tmp = RREG32_PCIE(ixPCIE_LC_CNTL4);
				tmp |= PCIE_LC_CNTL4__LC_REDO_EQ_MASK;
				WREG32_PCIE(ixPCIE_LC_CNTL4, tmp);

				mdelay(100);

				/* linkctl */
				pci_read_config_word(root, bridge_pos + PCI_EXP_LNKCTL, &tmp16);
				tmp16 &= ~PCI_EXP_LNKCTL_HAWD;
				tmp16 |= (bridge_cfg & PCI_EXP_LNKCTL_HAWD);
				pci_write_config_word(root, bridge_pos + PCI_EXP_LNKCTL, tmp16);

				pci_read_config_word(adev->pdev, gpu_pos + PCI_EXP_LNKCTL, &tmp16);
				tmp16 &= ~PCI_EXP_LNKCTL_HAWD;
				tmp16 |= (gpu_cfg & PCI_EXP_LNKCTL_HAWD);
				pci_write_config_word(adev->pdev, gpu_pos + PCI_EXP_LNKCTL, tmp16);

				/* linkctl2 */
				pci_read_config_word(root, bridge_pos + PCI_EXP_LNKCTL2, &tmp16);
				tmp16 &= ~((1 << 4) | (7 << 9));
				tmp16 |= (bridge_cfg2 & ((1 << 4) | (7 << 9)));
				pci_write_config_word(root, bridge_pos + PCI_EXP_LNKCTL2, tmp16);

				pci_read_config_word(adev->pdev, gpu_pos + PCI_EXP_LNKCTL2, &tmp16);
				tmp16 &= ~((1 << 4) | (7 << 9));
				tmp16 |= (gpu_cfg2 & ((1 << 4) | (7 << 9)));
				pci_write_config_word(adev->pdev, gpu_pos + PCI_EXP_LNKCTL2, tmp16);

				tmp = RREG32_PCIE(ixPCIE_LC_CNTL4);
				tmp &= ~PCIE_LC_CNTL4__LC_SET_QUIESCE_MASK;
				WREG32_PCIE(ixPCIE_LC_CNTL4, tmp);
			}
		}
	}

	/* set the link speed */
	speed_cntl |= PCIE_LC_SPEED_CNTL__LC_FORCE_EN_SW_SPEED_CHANGE_MASK |
		PCIE_LC_SPEED_CNTL__LC_FORCE_DIS_HW_SPEED_CHANGE_MASK;
	speed_cntl &= ~PCIE_LC_SPEED_CNTL__LC_FORCE_DIS_SW_SPEED_CHANGE_MASK;
	WREG32_PCIE(ixPCIE_LC_SPEED_CNTL, speed_cntl);

	pci_read_config_word(adev->pdev, gpu_pos + PCI_EXP_LNKCTL2, &tmp16);
	tmp16 &= ~0xf;
	if (mask & DRM_PCIE_SPEED_80)
		tmp16 |= 3; /* gen3 */
	else if (mask & DRM_PCIE_SPEED_50)
		tmp16 |= 2; /* gen2 */
	else
		tmp16 |= 1; /* gen1 */
	pci_write_config_word(adev->pdev, gpu_pos + PCI_EXP_LNKCTL2, tmp16);

	speed_cntl = RREG32_PCIE(ixPCIE_LC_SPEED_CNTL);
	speed_cntl |= PCIE_LC_SPEED_CNTL__LC_INITIATE_LINK_SPEED_CHANGE_MASK;
	WREG32_PCIE(ixPCIE_LC_SPEED_CNTL, speed_cntl);

	for (i = 0; i < adev->usec_timeout; i++) {
		speed_cntl = RREG32_PCIE(ixPCIE_LC_SPEED_CNTL);
		if ((speed_cntl & PCIE_LC_SPEED_CNTL__LC_INITIATE_LINK_SPEED_CHANGE_MASK) == 0)
			break;
		udelay(1);
	}
#endif
}

static void cik_program_aspm(struct amdgpu_device *adev)
{
	u32 data, orig;
	bool disable_l0s = false, disable_l1 = false, disable_plloff_in_l1 = false;
	bool disable_clkreq = false;

	if (amdgpu_aspm == 0)
		return;

	/* XXX double check APUs */
	if (adev->flags & AMD_IS_APU)
		return;

	orig = data = RREG32_PCIE(ixPCIE_LC_N_FTS_CNTL);
	data &= ~PCIE_LC_N_FTS_CNTL__LC_XMIT_N_FTS_MASK;
	data |= (0x24 << PCIE_LC_N_FTS_CNTL__LC_XMIT_N_FTS__SHIFT) |
		PCIE_LC_N_FTS_CNTL__LC_XMIT_N_FTS_OVERRIDE_EN_MASK;
	if (orig != data)
		WREG32_PCIE(ixPCIE_LC_N_FTS_CNTL, data);

	orig = data = RREG32_PCIE(ixPCIE_LC_CNTL3);
	data |= PCIE_LC_CNTL3__LC_GO_TO_RECOVERY_MASK;
	if (orig != data)
		WREG32_PCIE(ixPCIE_LC_CNTL3, data);

	orig = data = RREG32_PCIE(ixPCIE_P_CNTL);
	data |= PCIE_P_CNTL__P_IGNORE_EDB_ERR_MASK;
	if (orig != data)
		WREG32_PCIE(ixPCIE_P_CNTL, data);

	orig = data = RREG32_PCIE(ixPCIE_LC_CNTL);
	data &= ~(PCIE_LC_CNTL__LC_L0S_INACTIVITY_MASK |
		PCIE_LC_CNTL__LC_L1_INACTIVITY_MASK);
	data |= PCIE_LC_CNTL__LC_PMI_TO_L1_DIS_MASK;
	if (!disable_l0s)
		data |= (7 << PCIE_LC_CNTL__LC_L0S_INACTIVITY__SHIFT);

	if (!disable_l1) {
		data |= (7 << PCIE_LC_CNTL__LC_L1_INACTIVITY__SHIFT);
		data &= ~PCIE_LC_CNTL__LC_PMI_TO_L1_DIS_MASK;
		if (orig != data)
			WREG32_PCIE(ixPCIE_LC_CNTL, data);

		if (!disable_plloff_in_l1) {
			bool clk_req_support;

			orig = data = RREG32_PCIE(ixPB0_PIF_PWRDOWN_0);
			data &= ~(PB0_PIF_PWRDOWN_0__PLL_POWER_STATE_IN_OFF_0_MASK |
				PB0_PIF_PWRDOWN_0__PLL_POWER_STATE_IN_TXS2_0_MASK);
			data |= (7 << PB0_PIF_PWRDOWN_0__PLL_POWER_STATE_IN_OFF_0__SHIFT) |
				(7 << PB0_PIF_PWRDOWN_0__PLL_POWER_STATE_IN_TXS2_0__SHIFT);
			if (orig != data)
				WREG32_PCIE(ixPB0_PIF_PWRDOWN_0, data);

			orig = data = RREG32_PCIE(ixPB0_PIF_PWRDOWN_1);
			data &= ~(PB0_PIF_PWRDOWN_1__PLL_POWER_STATE_IN_OFF_1_MASK |
				PB0_PIF_PWRDOWN_1__PLL_POWER_STATE_IN_TXS2_1_MASK);
			data |= (7 << PB0_PIF_PWRDOWN_1__PLL_POWER_STATE_IN_OFF_1__SHIFT) |
				(7 << PB0_PIF_PWRDOWN_1__PLL_POWER_STATE_IN_TXS2_1__SHIFT);
			if (orig != data)
				WREG32_PCIE(ixPB0_PIF_PWRDOWN_1, data);

			orig = data = RREG32_PCIE(ixPB1_PIF_PWRDOWN_0);
			data &= ~(PB1_PIF_PWRDOWN_0__PLL_POWER_STATE_IN_OFF_0_MASK |
				PB1_PIF_PWRDOWN_0__PLL_POWER_STATE_IN_TXS2_0_MASK);
			data |= (7 << PB1_PIF_PWRDOWN_0__PLL_POWER_STATE_IN_OFF_0__SHIFT) |
				(7 << PB1_PIF_PWRDOWN_0__PLL_POWER_STATE_IN_TXS2_0__SHIFT);
			if (orig != data)
				WREG32_PCIE(ixPB1_PIF_PWRDOWN_0, data);

			orig = data = RREG32_PCIE(ixPB1_PIF_PWRDOWN_1);
			data &= ~(PB1_PIF_PWRDOWN_1__PLL_POWER_STATE_IN_OFF_1_MASK |
				PB1_PIF_PWRDOWN_1__PLL_POWER_STATE_IN_TXS2_1_MASK);
			data |= (7 << PB1_PIF_PWRDOWN_1__PLL_POWER_STATE_IN_OFF_1__SHIFT) |
				(7 << PB1_PIF_PWRDOWN_1__PLL_POWER_STATE_IN_TXS2_1__SHIFT);
			if (orig != data)
				WREG32_PCIE(ixPB1_PIF_PWRDOWN_1, data);

			orig = data = RREG32_PCIE(ixPCIE_LC_LINK_WIDTH_CNTL);
			data &= ~PCIE_LC_LINK_WIDTH_CNTL__LC_DYN_LANES_PWR_STATE_MASK;
			data |= ~(3 << PCIE_LC_LINK_WIDTH_CNTL__LC_DYN_LANES_PWR_STATE__SHIFT);
			if (orig != data)
				WREG32_PCIE(ixPCIE_LC_LINK_WIDTH_CNTL, data);

			if (!disable_clkreq) {
#ifdef __NetBSD__		/* XXX amdgpu pcie */
				clk_req_support = false;
#else
				struct pci_dev *root = adev->pdev->bus->self;
				u32 lnkcap;

				clk_req_support = false;
				pcie_capability_read_dword(root, PCI_EXP_LNKCAP, &lnkcap);
				if (lnkcap & PCI_EXP_LNKCAP_CLKPM)
					clk_req_support = true;
#endif
			} else {
				clk_req_support = false;
			}

			if (clk_req_support) {
				orig = data = RREG32_PCIE(ixPCIE_LC_CNTL2);
				data |= PCIE_LC_CNTL2__LC_ALLOW_PDWN_IN_L1_MASK |
					PCIE_LC_CNTL2__LC_ALLOW_PDWN_IN_L23_MASK;
				if (orig != data)
					WREG32_PCIE(ixPCIE_LC_CNTL2, data);

				orig = data = RREG32_SMC(ixTHM_CLK_CNTL);
				data &= ~(THM_CLK_CNTL__CMON_CLK_SEL_MASK |
					THM_CLK_CNTL__TMON_CLK_SEL_MASK);
				data |= (1 << THM_CLK_CNTL__CMON_CLK_SEL__SHIFT) |
					(1 << THM_CLK_CNTL__TMON_CLK_SEL__SHIFT);
				if (orig != data)
					WREG32_SMC(ixTHM_CLK_CNTL, data);

				orig = data = RREG32_SMC(ixMISC_CLK_CTRL);
				data &= ~(MISC_CLK_CTRL__DEEP_SLEEP_CLK_SEL_MASK |
					MISC_CLK_CTRL__ZCLK_SEL_MASK);
				data |= (1 << MISC_CLK_CTRL__DEEP_SLEEP_CLK_SEL__SHIFT) |
					(1 << MISC_CLK_CTRL__ZCLK_SEL__SHIFT);
				if (orig != data)
					WREG32_SMC(ixMISC_CLK_CTRL, data);

				orig = data = RREG32_SMC(ixCG_CLKPIN_CNTL);
				data &= ~CG_CLKPIN_CNTL__BCLK_AS_XCLK_MASK;
				if (orig != data)
					WREG32_SMC(ixCG_CLKPIN_CNTL, data);

				orig = data = RREG32_SMC(ixCG_CLKPIN_CNTL_2);
				data &= ~CG_CLKPIN_CNTL_2__FORCE_BIF_REFCLK_EN_MASK;
				if (orig != data)
					WREG32_SMC(ixCG_CLKPIN_CNTL_2, data);

				orig = data = RREG32_SMC(ixMPLL_BYPASSCLK_SEL);
				data &= ~MPLL_BYPASSCLK_SEL__MPLL_CLKOUT_SEL_MASK;
				data |= (4 << MPLL_BYPASSCLK_SEL__MPLL_CLKOUT_SEL__SHIFT);
				if (orig != data)
					WREG32_SMC(ixMPLL_BYPASSCLK_SEL, data);
			}
		}
	} else {
		if (orig != data)
			WREG32_PCIE(ixPCIE_LC_CNTL, data);
	}

	orig = data = RREG32_PCIE(ixPCIE_CNTL2);
	data |= PCIE_CNTL2__SLV_MEM_LS_EN_MASK |
		PCIE_CNTL2__MST_MEM_LS_EN_MASK |
		PCIE_CNTL2__REPLAY_MEM_LS_EN_MASK;
	if (orig != data)
		WREG32_PCIE(ixPCIE_CNTL2, data);

	if (!disable_l0s) {
		data = RREG32_PCIE(ixPCIE_LC_N_FTS_CNTL);
		if ((data & PCIE_LC_N_FTS_CNTL__LC_N_FTS_MASK) ==
				PCIE_LC_N_FTS_CNTL__LC_N_FTS_MASK) {
			data = RREG32_PCIE(ixPCIE_LC_STATUS1);
			if ((data & PCIE_LC_STATUS1__LC_REVERSE_XMIT_MASK) &&
			(data & PCIE_LC_STATUS1__LC_REVERSE_RCVR_MASK)) {
				orig = data = RREG32_PCIE(ixPCIE_LC_CNTL);
				data &= ~PCIE_LC_CNTL__LC_L0S_INACTIVITY_MASK;
				if (orig != data)
					WREG32_PCIE(ixPCIE_LC_CNTL, data);
			}
		}
	}
}

static uint32_t cik_get_rev_id(struct amdgpu_device *adev)
{
	return (RREG32(mmCC_DRM_ID_STRAPS) & CC_DRM_ID_STRAPS__ATI_REV_ID_MASK)
		>> CC_DRM_ID_STRAPS__ATI_REV_ID__SHIFT;
}

static const struct amdgpu_ip_block_version bonaire_ip_blocks[] =
{
	/* ORDER MATTERS! */
	{
		.type = AMD_IP_BLOCK_TYPE_COMMON,
		.major = 1,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_common_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_GMC,
		.major = 7,
		.minor = 0,
		.rev = 0,
		.funcs = &gmc_v7_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_IH,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_ih_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_SMC,
		.major = 7,
		.minor = 0,
		.rev = 0,
		.funcs = &ci_dpm_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_DCE,
		.major = 8,
		.minor = 2,
		.rev = 0,
		.funcs = &dce_v8_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_GFX,
		.major = 7,
		.minor = 2,
		.rev = 0,
		.funcs = &gfx_v7_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_SDMA,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_sdma_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_UVD,
		.major = 4,
		.minor = 2,
		.rev = 0,
		.funcs = &uvd_v4_2_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_VCE,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &vce_v2_0_ip_funcs,
	},
};

static const struct amdgpu_ip_block_version hawaii_ip_blocks[] =
{
	/* ORDER MATTERS! */
	{
		.type = AMD_IP_BLOCK_TYPE_COMMON,
		.major = 1,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_common_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_GMC,
		.major = 7,
		.minor = 0,
		.rev = 0,
		.funcs = &gmc_v7_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_IH,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_ih_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_SMC,
		.major = 7,
		.minor = 0,
		.rev = 0,
		.funcs = &ci_dpm_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_DCE,
		.major = 8,
		.minor = 5,
		.rev = 0,
		.funcs = &dce_v8_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_GFX,
		.major = 7,
		.minor = 3,
		.rev = 0,
		.funcs = &gfx_v7_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_SDMA,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_sdma_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_UVD,
		.major = 4,
		.minor = 2,
		.rev = 0,
		.funcs = &uvd_v4_2_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_VCE,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &vce_v2_0_ip_funcs,
	},
};

static const struct amdgpu_ip_block_version kabini_ip_blocks[] =
{
	/* ORDER MATTERS! */
	{
		.type = AMD_IP_BLOCK_TYPE_COMMON,
		.major = 1,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_common_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_GMC,
		.major = 7,
		.minor = 0,
		.rev = 0,
		.funcs = &gmc_v7_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_IH,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_ih_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_SMC,
		.major = 7,
		.minor = 0,
		.rev = 0,
		.funcs = &kv_dpm_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_DCE,
		.major = 8,
		.minor = 3,
		.rev = 0,
		.funcs = &dce_v8_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_GFX,
		.major = 7,
		.minor = 2,
		.rev = 0,
		.funcs = &gfx_v7_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_SDMA,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_sdma_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_UVD,
		.major = 4,
		.minor = 2,
		.rev = 0,
		.funcs = &uvd_v4_2_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_VCE,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &vce_v2_0_ip_funcs,
	},
};

static const struct amdgpu_ip_block_version mullins_ip_blocks[] =
{
	/* ORDER MATTERS! */
	{
		.type = AMD_IP_BLOCK_TYPE_COMMON,
		.major = 1,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_common_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_GMC,
		.major = 7,
		.minor = 0,
		.rev = 0,
		.funcs = &gmc_v7_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_IH,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_ih_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_SMC,
		.major = 7,
		.minor = 0,
		.rev = 0,
		.funcs = &kv_dpm_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_DCE,
		.major = 8,
		.minor = 3,
		.rev = 0,
		.funcs = &dce_v8_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_GFX,
		.major = 7,
		.minor = 2,
		.rev = 0,
		.funcs = &gfx_v7_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_SDMA,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_sdma_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_UVD,
		.major = 4,
		.minor = 2,
		.rev = 0,
		.funcs = &uvd_v4_2_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_VCE,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &vce_v2_0_ip_funcs,
	},
};

static const struct amdgpu_ip_block_version kaveri_ip_blocks[] =
{
	/* ORDER MATTERS! */
	{
		.type = AMD_IP_BLOCK_TYPE_COMMON,
		.major = 1,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_common_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_GMC,
		.major = 7,
		.minor = 0,
		.rev = 0,
		.funcs = &gmc_v7_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_IH,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_ih_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_SMC,
		.major = 7,
		.minor = 0,
		.rev = 0,
		.funcs = &kv_dpm_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_DCE,
		.major = 8,
		.minor = 1,
		.rev = 0,
		.funcs = &dce_v8_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_GFX,
		.major = 7,
		.minor = 1,
		.rev = 0,
		.funcs = &gfx_v7_0_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_SDMA,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &cik_sdma_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_UVD,
		.major = 4,
		.minor = 2,
		.rev = 0,
		.funcs = &uvd_v4_2_ip_funcs,
	},
	{
		.type = AMD_IP_BLOCK_TYPE_VCE,
		.major = 2,
		.minor = 0,
		.rev = 0,
		.funcs = &vce_v2_0_ip_funcs,
	},
};

int cik_set_ip_blocks(struct amdgpu_device *adev)
{
	switch (adev->asic_type) {
	case CHIP_BONAIRE:
		adev->ip_blocks = bonaire_ip_blocks;
		adev->num_ip_blocks = ARRAY_SIZE(bonaire_ip_blocks);
		break;
	case CHIP_HAWAII:
		adev->ip_blocks = hawaii_ip_blocks;
		adev->num_ip_blocks = ARRAY_SIZE(hawaii_ip_blocks);
		break;
	case CHIP_KAVERI:
		adev->ip_blocks = kaveri_ip_blocks;
		adev->num_ip_blocks = ARRAY_SIZE(kaveri_ip_blocks);
		break;
	case CHIP_KABINI:
		adev->ip_blocks = kabini_ip_blocks;
		adev->num_ip_blocks = ARRAY_SIZE(kabini_ip_blocks);
		break;
	case CHIP_MULLINS:
		adev->ip_blocks = mullins_ip_blocks;
		adev->num_ip_blocks = ARRAY_SIZE(mullins_ip_blocks);
		break;
	default:
		/* FIXME: not supported yet */
		return -EINVAL;
	}

	return 0;
}

static const struct amdgpu_asic_funcs cik_asic_funcs =
{
	.read_disabled_bios = &cik_read_disabled_bios,
	.read_register = &cik_read_register,
	.reset = &cik_asic_reset,
	.set_vga_state = &cik_vga_set_state,
	.get_xclk = &cik_get_xclk,
	.set_uvd_clocks = &cik_set_uvd_clocks,
	.set_vce_clocks = &cik_set_vce_clocks,
	.get_cu_info = &gfx_v7_0_get_cu_info,
	/* these should be moved to their own ip modules */
	.get_gpu_clock_counter = &gfx_v7_0_get_gpu_clock_counter,
	.wait_for_mc_idle = &gmc_v7_0_mc_wait_for_idle,
};

static int cik_common_early_init(void *handle)
{
	struct amdgpu_device *adev = (struct amdgpu_device *)handle;

	adev->smc_rreg = &cik_smc_rreg;
	adev->smc_wreg = &cik_smc_wreg;
	adev->pcie_rreg = &cik_pcie_rreg;
	adev->pcie_wreg = &cik_pcie_wreg;
	adev->uvd_ctx_rreg = &cik_uvd_ctx_rreg;
	adev->uvd_ctx_wreg = &cik_uvd_ctx_wreg;
	adev->didt_rreg = &cik_didt_rreg;
	adev->didt_wreg = &cik_didt_wreg;

	adev->asic_funcs = &cik_asic_funcs;

	adev->has_uvd = true;

	adev->rev_id = cik_get_rev_id(adev);
	adev->external_rev_id = 0xFF;
	switch (adev->asic_type) {
	case CHIP_BONAIRE:
		adev->cg_flags =
			AMDGPU_CG_SUPPORT_GFX_MGCG |
			AMDGPU_CG_SUPPORT_GFX_MGLS |
			/*AMDGPU_CG_SUPPORT_GFX_CGCG |*/
			AMDGPU_CG_SUPPORT_GFX_CGLS |
			AMDGPU_CG_SUPPORT_GFX_CGTS |
			AMDGPU_CG_SUPPORT_GFX_CGTS_LS |
			AMDGPU_CG_SUPPORT_GFX_CP_LS |
			AMDGPU_CG_SUPPORT_MC_LS |
			AMDGPU_CG_SUPPORT_MC_MGCG |
			AMDGPU_CG_SUPPORT_SDMA_MGCG |
			AMDGPU_CG_SUPPORT_SDMA_LS |
			AMDGPU_CG_SUPPORT_BIF_LS |
			AMDGPU_CG_SUPPORT_VCE_MGCG |
			AMDGPU_CG_SUPPORT_UVD_MGCG |
			AMDGPU_CG_SUPPORT_HDP_LS |
			AMDGPU_CG_SUPPORT_HDP_MGCG;
		adev->pg_flags = 0;
		adev->external_rev_id = adev->rev_id + 0x14;
		break;
	case CHIP_HAWAII:
		adev->cg_flags =
			AMDGPU_CG_SUPPORT_GFX_MGCG |
			AMDGPU_CG_SUPPORT_GFX_MGLS |
			/*AMDGPU_CG_SUPPORT_GFX_CGCG |*/
			AMDGPU_CG_SUPPORT_GFX_CGLS |
			AMDGPU_CG_SUPPORT_GFX_CGTS |
			AMDGPU_CG_SUPPORT_GFX_CP_LS |
			AMDGPU_CG_SUPPORT_MC_LS |
			AMDGPU_CG_SUPPORT_MC_MGCG |
			AMDGPU_CG_SUPPORT_SDMA_MGCG |
			AMDGPU_CG_SUPPORT_SDMA_LS |
			AMDGPU_CG_SUPPORT_BIF_LS |
			AMDGPU_CG_SUPPORT_VCE_MGCG |
			AMDGPU_CG_SUPPORT_UVD_MGCG |
			AMDGPU_CG_SUPPORT_HDP_LS |
			AMDGPU_CG_SUPPORT_HDP_MGCG;
		adev->pg_flags = 0;
		adev->external_rev_id = 0x28;
		break;
	case CHIP_KAVERI:
		adev->cg_flags =
			AMDGPU_CG_SUPPORT_GFX_MGCG |
			AMDGPU_CG_SUPPORT_GFX_MGLS |
			/*AMDGPU_CG_SUPPORT_GFX_CGCG |*/
			AMDGPU_CG_SUPPORT_GFX_CGLS |
			AMDGPU_CG_SUPPORT_GFX_CGTS |
			AMDGPU_CG_SUPPORT_GFX_CGTS_LS |
			AMDGPU_CG_SUPPORT_GFX_CP_LS |
			AMDGPU_CG_SUPPORT_SDMA_MGCG |
			AMDGPU_CG_SUPPORT_SDMA_LS |
			AMDGPU_CG_SUPPORT_BIF_LS |
			AMDGPU_CG_SUPPORT_VCE_MGCG |
			AMDGPU_CG_SUPPORT_UVD_MGCG |
			AMDGPU_CG_SUPPORT_HDP_LS |
			AMDGPU_CG_SUPPORT_HDP_MGCG;
		adev->pg_flags =
			/*AMDGPU_PG_SUPPORT_GFX_PG |
			  AMDGPU_PG_SUPPORT_GFX_SMG |
			  AMDGPU_PG_SUPPORT_GFX_DMG |*/
			AMDGPU_PG_SUPPORT_UVD |
			/*AMDGPU_PG_SUPPORT_VCE |
			  AMDGPU_PG_SUPPORT_CP |
			  AMDGPU_PG_SUPPORT_GDS |
			  AMDGPU_PG_SUPPORT_RLC_SMU_HS |
			  AMDGPU_PG_SUPPORT_ACP |
			  AMDGPU_PG_SUPPORT_SAMU |*/
			0;
		if (adev->pdev->device == 0x1312 ||
			adev->pdev->device == 0x1316 ||
			adev->pdev->device == 0x1317)
			adev->external_rev_id = 0x41;
		else
			adev->external_rev_id = 0x1;
		break;
	case CHIP_KABINI:
	case CHIP_MULLINS:
		adev->cg_flags =
			AMDGPU_CG_SUPPORT_GFX_MGCG |
			AMDGPU_CG_SUPPORT_GFX_MGLS |
			/*AMDGPU_CG_SUPPORT_GFX_CGCG |*/
			AMDGPU_CG_SUPPORT_GFX_CGLS |
			AMDGPU_CG_SUPPORT_GFX_CGTS |
			AMDGPU_CG_SUPPORT_GFX_CGTS_LS |
			AMDGPU_CG_SUPPORT_GFX_CP_LS |
			AMDGPU_CG_SUPPORT_SDMA_MGCG |
			AMDGPU_CG_SUPPORT_SDMA_LS |
			AMDGPU_CG_SUPPORT_BIF_LS |
			AMDGPU_CG_SUPPORT_VCE_MGCG |
			AMDGPU_CG_SUPPORT_UVD_MGCG |
			AMDGPU_CG_SUPPORT_HDP_LS |
			AMDGPU_CG_SUPPORT_HDP_MGCG;
		adev->pg_flags =
			/*AMDGPU_PG_SUPPORT_GFX_PG |
			  AMDGPU_PG_SUPPORT_GFX_SMG | */
			AMDGPU_PG_SUPPORT_UVD |
			/*AMDGPU_PG_SUPPORT_VCE |
			  AMDGPU_PG_SUPPORT_CP |
			  AMDGPU_PG_SUPPORT_GDS |
			  AMDGPU_PG_SUPPORT_RLC_SMU_HS |
			  AMDGPU_PG_SUPPORT_SAMU |*/
			0;
		if (adev->asic_type == CHIP_KABINI) {
			if (adev->rev_id == 0)
				adev->external_rev_id = 0x81;
			else if (adev->rev_id == 1)
				adev->external_rev_id = 0x82;
			else if (adev->rev_id == 2)
				adev->external_rev_id = 0x85;
		} else
			adev->external_rev_id = adev->rev_id + 0xa1;
		break;
	default:
		/* FIXME: not supported yet */
		return -EINVAL;
	}

	return 0;
}

static int cik_common_sw_init(void *handle)
{
	return 0;
}

static int cik_common_sw_fini(void *handle)
{
	return 0;
}

static int cik_common_hw_init(void *handle)
{
	struct amdgpu_device *adev = (struct amdgpu_device *)handle;

	/* move the golden regs per IP block */
	cik_init_golden_registers(adev);
	/* enable pcie gen2/3 link */
	cik_pcie_gen3_enable(adev);
	/* enable aspm */
	cik_program_aspm(adev);

	return 0;
}

static int cik_common_hw_fini(void *handle)
{
	return 0;
}

static int cik_common_suspend(void *handle)
{
	struct amdgpu_device *adev = (struct amdgpu_device *)handle;

	amdgpu_amdkfd_suspend(adev);

	return cik_common_hw_fini(adev);
}

static int cik_common_resume(void *handle)
{
	int r;
	struct amdgpu_device *adev = (struct amdgpu_device *)handle;

	r = cik_common_hw_init(adev);
	if (r)
		return r;

	return amdgpu_amdkfd_resume(adev);
}

static bool cik_common_is_idle(void *handle)
{
	return true;
}

static int cik_common_wait_for_idle(void *handle)
{
	return 0;
}

static void cik_common_print_status(void *handle)
{

}

static int cik_common_soft_reset(void *handle)
{
	/* XXX hard reset?? */
	return 0;
}

static int cik_common_set_clockgating_state(void *handle,
					    enum amd_clockgating_state state)
{
	return 0;
}

static int cik_common_set_powergating_state(void *handle,
					    enum amd_powergating_state state)
{
	return 0;
}

const struct amd_ip_funcs cik_common_ip_funcs = {
	.early_init = cik_common_early_init,
	.late_init = NULL,
	.sw_init = cik_common_sw_init,
	.sw_fini = cik_common_sw_fini,
	.hw_init = cik_common_hw_init,
	.hw_fini = cik_common_hw_fini,
	.suspend = cik_common_suspend,
	.resume = cik_common_resume,
	.is_idle = cik_common_is_idle,
	.wait_for_idle = cik_common_wait_for_idle,
	.soft_reset = cik_common_soft_reset,
	.print_status = cik_common_print_status,
	.set_clockgating_state = cik_common_set_clockgating_state,
	.set_powergating_state = cik_common_set_powergating_state,
};
