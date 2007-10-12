#ifndef _ARM_OMAP_OMAP_GPTMRREG_H_
#define _ARM_OMAP_OMAP_GPTMRREG_H_

/* Registers */
#define TIDR		0x00
#define TIOCP_CFG	0x10
#define TISTAT		0x14
#define TISR		0x18
#define TIER		0x1C
#define TWER		0x20
#define TCLR		0x24
#define TCRR		0x28
#define TLDR		0x2C
#define TTGR		0x30
#define TWPS		0x34
#define TMAR		0x38
#define TCAR		0x3C
#define TSICR		0x40


#define TIDR_TID_REV_MASK		0xF

#define TIOCP_CFG_AUTOIDLE		(1<<0)
#define TIOCP_CFG_SOFTRESET		(1<<1)
#define TIOCP_CFG_ENAWAKEUP		(1<<2)
#define TIOCP_CFG_IDLEMODE_MASK	(3<<3)
#define TIOCP_CFG_IDLEMODE(n)	(((n)&0x3)<<3)
#define TIOCP_CFG_EMUFREE		(1<<5)

#define TISTAT_RESETDONE		(1<<0)

#define TISR_MAT_IT_FLAG		(1<<0)
#define TISR_OVF_IT_FLAG		(1<<1)
#define TISR_TCAR_IT_FLAG		(1<<2)

#define TIER_MAT_IT_ENA			(1<<0)
#define TIER_OVF_IT_ENA			(1<<1)
#define TIER_TCAR_IT_ENA		(1<<2)

#define TWER_MAT_WUP_ENA		(1<<0)
#define TWER_OVF_WUP_ENA		(1<<2)
#define TWER_TCAR_WUP_ENA		(1<<3)

#define TCLR_ST					(1<<0)
#define TCLR_AR					(1<<1)
#define TCLR_PTV_MASK			(7<<2)
#define TCLR_PTV(n)				((n)<<2)
#define TCLR_PRE(n)				((n)<<5)
#define TCLR_CE					(1<<6)
#define TCLR_SCPWM				(1<<7)
#define TCLR_TCM(n)				((n)<<8)
#define TCLR_TCM_MASK			(3<<8)
#define TCLR_TRG(n)				((n)<<10)
#define TCLR_TRG_MASK			(3<<10)
#define TCLR_PT					(1<<12)

#define TCLR_TCM_NONE			0
#define TCLR_TCM_RISING			1
#define TCLR_TCM_FALLING		2
#define TCLR_TCM_BOTH			3

#define TCLR_TRG_NONE			0
#define TCLR_TRG_OVERFLOW		1
#define TCLR_TRG_OVERFLOW_AND_MATCH 2

#define TWPS_W_PEND__TCLR		(1<<0)
#define TWPS_W_PEND__TCRR		(1<<1)
#define TWPS_W_PEND__TLDR		(1<<2)
#define TWPS_W_PEND__TTGR		(1<<3)
#define TWPS_W_PEND__TMAR		(1<<4)

#define TSICR_POSTED			(1<<2)
#define TSICR_SFT				(1<<1)

#endif
