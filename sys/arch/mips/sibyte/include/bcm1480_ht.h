/*  *********************************************************************
    *  BCM1280/BCM1480 Board Support Package
    *
    *  HT Device constants                            File: bcm1480_ht.h
    *
    *  This module contains constants and macros to describe
    *  the HT interface on the BCM1280/BCM1480.
    *
    *  BCM1480 specification level:  1X55_1X80-UM100-R (12/18/03)
    *
    *********************************************************************
    *
    *  Copyright 2000,2001,2002,2003,2004
    *  Broadcom Corporation. All rights reserved.
    *
    *  This software is furnished under license and may be used and
    *  copied only in accordance with the following terms and
    *  conditions.  Subject to these conditions, you may download,
    *  copy, install, use, modify and distribute modified or unmodified
    *  copies of this software in source and/or binary form.  No title
    *  or ownership is transferred hereby.
    *
    *  1) Any source code used, modified or distributed must reproduce
    *     and retain this copyright notice and list of conditions
    *     as they appear in the source file.
    *
    *  2) No right is granted to use any trade name, trademark, or
    *     logo of Broadcom Corporation.  The "Broadcom Corporation"
    *     name may not be used to endorse or promote products derived
    *     from this software without the prior written permission of
    *     Broadcom Corporation.
    *
    *  3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR
    *     IMPLIED WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED
    *     WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
    *     PURPOSE, OR NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT
    *     SHALL BROADCOM BE LIABLE FOR ANY DAMAGES WHATSOEVER, AND IN
    *     PARTICULAR, BROADCOM SHALL NOT BE LIABLE FOR DIRECT, INDIRECT,
    *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    *     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
    *     GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
    *     BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    *     OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
    *     TORT (INCLUDING NEGLIGENCE OR OTHERWISE), EVEN IF ADVISED OF
    *     THE POSSIBILITY OF SUCH DAMAGE.
    ********************************************************************* */


#ifndef _BCM1480_HT_H
#define _BCM1480_HT_H

#include "sb1250_defs.h"


/*
 * The following definitions refer to PCI Configuration Space of the
 * HyperTransport Device Host Bridge (HTD).  All registers are 32 bits.
 */

/*
 * HT Interface Device Configuration Header (Table 130)
 * The first 64 bytes are a standard Type 0 header.  Only
 * device-specific extensions are defined here.
 */

#define R_BCM1480_HTD_ERRORINT           0x0084
#define R_BCM1480_HTD_SPECCMDSTAT        0x0088
#define R_BCM1480_HTD_SUBSYSSET          0x008C
#define R_BCM1480_HTD_TGTDONE            0x0090
#define R_BCM1480_HTD_VENDORIDSET        0x009C
#define R_BCM1480_HTD_CLASSREVSET        0x00A0
#define R_BCM1480_HTD_ISOCBAR            0x00A4
#define R_BCM1480_HTD_ISOCBARMASK        0x00A8
#define R_BCM1480_HTD_MAPBASE            0x00B0	/* 0xB0 through 0xEC - map table */
#define HTD_MAPENTRIES           16	/* 64 bytes, 16 entries */
#define R_BCM1480_HTD_MAP(n)             (R_BCM1480_HTD_MAPBASE + (n)*4)
#define R_BCM1480_HTD_INTBUSCTRL         0x00F0
#define R_BCM1480_HTD_PORTSCTRL          0x00F4


/*
 * HT Device Error and Interrupt Register (Table 133)
 */

#define M_BCM1480_HTD_INT_BUS_ERR            _SB_MAKEMASK1_32(0)
#define M_BCM1480_HTD_PORT0_DOWN_ERR         _SB_MAKEMASK1_32(8)
#define M_BCM1480_HTD_PORT1_DOWN_ERR         _SB_MAKEMASK1_32(9)
#define M_BCM1480_HTD_PORT2_DOWN_ERR         _SB_MAKEMASK1_32(10)
#define M_BCM1480_HTD_INT_BUS_DOWN_ERR       _SB_MAKEMASK1_32(12)
#define M_BCM1480_HTD_INT_BUS_ERR_INT_EN     _SB_MAKEMASK1_32(16)
#define M_BCM1480_HTD_PORT0_DOWN_INT_EN      _SB_MAKEMASK1_32(24)
#define M_BCM1480_HTD_PORT1_DOWN_INT_EN      _SB_MAKEMASK1_32(25)
#define M_BCM1480_HTD_PORT2_DOWN_INT_EN      _SB_MAKEMASK1_32(26)
#define M_BCM1480_HTD_INT_BUS_DOWN_INT_EN    _SB_MAKEMASK1_32(28)

/*
 * HT Device Specific Command and Status Register (Table 134)
 */

#define M_BCM1480_HTD_CMD_LOW_RANGE_EN       _SB_MAKEMASK1_32(0)
#define M_BCM1480_HTD_CMD_LOW_RANGE_SPLIT    _SB_MAKEMASK1_32(1)
#define M_BCM1480_HTD_CMD_FULL_BAR_EN        _SB_MAKEMASK1_32(2)
#define M_BCM1480_HTD_CMD_FULL_BAR_SPLIT     _SB_MAKEMASK1_32(3)
#define M_BCM1480_HTD_CMD_MSTR_ABORT_MODE    _SB_MAKEMASK1_32(5)

/*
 * HT Device Target Done Register (Table 135)
 */

#define S_BCM1480_HTD_TGT_DONE_COUNTER       0
#define M_BCM1480_HTD_TGT_DONE_COUNTER       _SB_MAKEMASK_32(8,S_BCM1480_HTD_TGT_DONE_COUNTER)
#define V_BCM1480_HTD_TGT_DONE_COUNTER(x)    _SB_MAKEVALUE_32(x,S_BCM1480_HTD_TGT_DONE_COUNTER)
#define G_BCM1480_HTD_TGT_DONE_COUNTER(x)    _SB_GETVALUE_32(x,S_BCM1480_HTD_TGT_DONE_COUNTER,M_BCM1480_HTD_TGT_DONE_COUNTER)

/*
 * HT Device Isochronous BAR Register (Table 136)
 */

#define M_BCM1480_HTD_ISOC_BAR_EN            _SB_MAKEMASK1_32(0)

#define S_BCM1480_HTD_ISOC_BAR               1
#define M_BCM1480_HTD_ISOC_BAR               _SB_MAKEMASK_32(31,S_BCM1480_HTD_ISOC_BAR)
#define V_BCM1480_HTD_ISOC_BAR(x)            _SB_MAKEVALUE_32(x,S_BCM1480_HTD_ISOC_BAR)
#define G_BCM1480_HTD_ISOC_BAR(x)            _SB_GETVALUE_32(x,S_BCM1480_HTD_ISOC_BAR,M_BCM1480_HTD_ISOC_BAR)

/*
 * HT Device Isochronous Ignore Mask Register (Table 137)
 */

#define S_BCM1480_HTD_ISOC_IGN_MASK          1
#define M_BCM1480_HTD_ISOC_IGN_MASK          _SB_MAKEMASK_32(31,S_BCM1480_HTD_ISOC_IGN_MASK)
#define V_BCM1480_HTD_ISOC_IGN_MASK(x)       _SB_MAKEVALUE_32(x,S_BCM1480_HTD_ISOC_IGN_MASK)
#define G_BCM1480_HTD_ISOC_IGN_MASK(x)       _SB_GETVALUE_32(x,S_BCM1480_HTD_ISOCBAR,M_BCM1480_HTD_ISOCBAR)

/*
 * HT Device BAR0 Map Table Entry (Table 138)
 */

#define M_BCM1480_HTD_BAR0MAP_ENABLE         _SB_MAKEMASK1_32(0)
#define M_BCM1480_HTD_BAR0MAP_L2CA           _SB_MAKEMASK1_32(2)
#define M_BCM1480_HTD_BAR0MAP_ENDIAN         _SB_MAKEMASK1_32(3)

#define S_BCM1480_HTD_BAR0MAP_ADDR           12
#define M_BCM1480_HTD_BAR0MAP_ADDR           _SB_MAKEMASK_32(20,S_BCM1480_HTD_BAR0MAP_ADDR)
#define V_BCM1480_HTD_BAR0MAP_ADDR(x)        _SB_MAKEVALUE_32(x,S_BCM1480_HTD_BAR0MAP_ADDR)
#define G_BCM1480_HTD_BAR0MAP_ADDR(x)        _SB_GETVALUE_32(x,S_BCM1480_HTD_BAR0MAP_ADDR,M_BCM1480_HTD_BAR0MAP_ADDR)

/*
 * HT Device Internal Bus Control and Status Register (Table 139)
 */

#define M_BCM1480_HTD_INTBUS_WARM_R          _SB_MAKEMASK1_32(2)
#define M_BCM1480_HTD_INTBUS_RESET           _SB_MAKEMASK1_32(3)
#define M_BCM1480_HTD_INTBUS_WARM_R_STATUS   _SB_MAKEMASK1_32(4)
#define M_BCM1480_HTD_INTBUS_RESET_STATUS    _SB_MAKEMASK1_32(5)
#define M_BCM1480_HTD_INTBUS_DET_SERR        _SB_MAKEMASK1_32(8)

/*
 * HT Device Ports Control and Status Register (Table 140)
 */

#define S_BCM1480_HTD_PORTCTRL_PORT0        0
#define M_BCM1480_HTD_PORTCTRL_PORT0        _SB_MAKEMASK_32(8,S_BCM1480_HTD_PORTCTRL_PORT0)
#define V_BCM1480_HTD_PORTCTRL_PORT0(x)     _SB_MAKEVALUE_32(x,S_BCM1480_HTD_PORTCTRL_PORT0)
#define G_BCM1480_HTD_PORTCTRL_PORT0(x)     _SB_GETVALUE_32(x,S_BCM1480_HTD_PORTCTRL_PORT0,M_BCM1480_HTD_PORTCTRL_PORT0)

#define S_BCM1480_HTD_PORTCTRL_PORT1        8
#define M_BCM1480_HTD_PORTCTRL_PORT1        _SB_MAKEMASK_32(8,S_BCM1480_HTD_PORTCTRL_PORT1)
#define V_BCM1480_HTD_PORTCTRL_PORT1(x)     _SB_MAKEVALUE_32(x,S_BCM1480_HTD_PORTCTRL_PORT1)
#define G_BCM1480_HTD_PORTCTRL_PORT1(x)     _SB_GETVALUE_32(x,S_BCM1480_HTD_PORTCTRL_PORT1,M_BCM1480_HTD_PORTCTRL_PORT1)

#define S_BCM1480_HTD_PORTCTRL_PORT2        16
#define M_BCM1480_HTD_PORTCTRL_PORT2        _SB_MAKEMASK_32(8,S_BCM1480_HTD_PORTCTRL_PORT2)
#define V_BCM1480_HTD_PORTCTRL_PORT2(x)     _SB_MAKEVALUE_32(x,S_BCM1480_HTD_PORTCTRL_PORT2)
#define G_BCM1480_HTD_PORTCTRL_PORT2(x)     _SB_GETVALUE_32(x,S_BCM1480_HTD_PORTCTRL_PORT2,M_BCM1480_HTD_PORTCTRL_PORT2)

#define S_BCM1480_HTD_PORTCTRL_PORTX(p)     ((p)*8)
#define M_BCM1480_HTD_PORTCTRL_PORTX(p)     _SB_MAKEMASK_32(8,S_BCM1480_HTD_PORTCTRL_PORTX(p))
#define V_BCM1480_HTD_PORTCTRL_PORTX(p,x)   _SB_MAKEVALUE_32(x,S_BCM1480_HTD_PORTCTRL_PORTX(p))
#define G_BCM1480_HTD_PORTCTRL_PORTX(p,x)   _SB_GETVALUE_32(x,S_BCM1480_HTD_PORTCTRL_PORTX(p),M_BCM1480_HTD_PORTCTRL_PORTX(p))

/* The following fields are per port */
#define M_BCM1480_HTD_PORT_ACTIVE           _SB_MAKEMASK1_32(0)
#define M_BCM1480_HTD_PORT_IS_PRIMARY       _SB_MAKEMASK1_32(1)
#define M_BCM1480_HTD_PORT_LINK_WARM_R      _SB_MAKEMASK1_32(2)
#define M_BCM1480_HTD_PORT_LINK_RESET       _SB_MAKEMASK1_32(3)
#define M_BCM1480_HTD_PORT_LINKWARMR_STATUS _SB_MAKEMASK1_32(4)
#define M_BCM1480_HTD_PORT_LINKRESET_STATUS _SB_MAKEMASK1_32(5)


/*
 * HT Internal Bridge HTB Configuration Header (Tables 141
 * and 163).  The first 64 bytes are a standard Type 1 header.
 * Only device-specific extensions are defined here.
 * Note that Primary (Table 141) and Secondary (Table 163) formats
 * are identical except for the HT Link Capability registers.
 */

#define R_BCM1480_HTB_LINKCAP            0x0040
#define R_BCM1480_HTB_LINKCTRL           0x0044
#define R_BCM1480_HTB_LINKFREQERR        0x0048
#define R_BCM1480_HTB_ERRHNDL            0x0050
#define R_BCM1480_HTB_SWITCHCAP          0x005C
#define R_BCM1480_HTB_SWITCHINFO         0x0064
#define R_BCM1480_HTB_VCSETCAP           0x0074

#define R_BCM1480_HTB_SPECBRCTRL         0x0088
#define R_BCM1480_HTB_SPECLINKFREQ       0x0090
#define R_BCM1480_HTB_VENDORIDSET        0x009C
#define R_BCM1480_HTB_NODEROUTING0       0x00B0
#define R_BCM1480_HTB_NODEROUTING1       0x00B4


/*
 * HT Bridge Specific Bridge Link Control Register (Tables 156 and 178)
 */

#define M_BCM1480_HTB_LINKCTRL_CRCFLEN   _SB_MAKEMASK1_32(1)
#define M_BCM1480_HTB_LINKCTRL_LINKFAIL  _SB_MAKEMASK1_32(4)
#define S_BCM1480_HTB_LINKCTRL_CRCERR    8
#define M_BCM1480_HTB_LINKCTRL_CRCERR    _SB_MAKEMASK_32(4,S_BCM1480_HTB_LINKCTRL_CRCERR)

/*
 * HT Bridge Specific Bridge Link Frequency / Error Register (Tables 158 and 180)
 */
#define M_BCM1480_HTB_LINKFQERR_PROTERR   _SB_MAKEMASK1_32(12)
#define M_BCM1480_HTB_LINKFQERR_OVFLERR   _SB_MAKEMASK1_32(13)
#define M_BCM1480_HTB_LINKFQERR_EOCERR    _SB_MAKEMASK1_32(14)


/*
 * HT Bridge Specific Bridge Error Handling Register (Tables 161 and 182)
 */
#define M_BCM1480_HTB_ERRHNDL_PROFLEN     _SB_MAKEMASK1_32(16)
#define M_BCM1480_HTB_ERRHNDL_OVFFLEN     _SB_MAKEMASK1_32(17)
#define M_BCM1480_HTB_ERRHNDL_CHNFAIL     _SB_MAKEMASK1_32(24)
#define M_BCM1480_HTB_ERRHNDL_RSPERR      _SB_MAKEMASK1_32(25)




/*
 * HT Bridge Specific Bridge Control Register (Tables 160 and 181)
 */

#define M_BCM1480_HTB_SPBRCTRL_SOUTH         _SB_MAKEMASK1_32(0)
#define M_BCM1480_HTB_SPBRCTRL_NO_INTR_FORWARD _SB_MAKEMASK1_32(8)

/*
 * HT Bridge Specific Link Frequency Control Register (Table 160)
 */

#define S_BCM1480_HTB_SPLINKFREQ_PLLFREQ     0
#define M_BCM1480_HTB_SPLINKFREQ_PLLFREQ     _SB_MAKEMASK_32(5,S_BCM1480_HTB_SPLINKFREQ_PLLFREQ)
#define V_BCM1480_HTB_SPLINKFREQ_PLLFREQ(x)  _SB_MAKEVALUE_32(x,S_BCM1480_HTB_SPLINKFREQ_PLLFREQ)
#define G_BCM1480_HTB_SPLINKFREQ_PLLFREQ(x)  _SB_GETVALUE_32(x,S_BCM1480_HTB_SPLINKFREQ_PLLFREQ,M_BCM1480_HTB_SPLINKFREQ_PLLFREQ)

#define M_BCM1480_HTB_SPLINKFREQ_PLLDIV4     _SB_MAKEMASK1_32(5)
#define M_BCM1480_HTB_SPLINKFREQ_PLLCOMPAT   _SB_MAKEMASK1_32(6)
#define M_BCM1480_HTB_SPLINKFREQ_SEQSTEP     _SB_MAKEMASK1_32(7)

#define S_BCM1480_HTB_SPLINKFREQ_FREQCAPSET  16
#define M_BCM1480_HTB_SPLINKFREQ_FREQCAPSET  _SB_MAKEMASK_32(16,S_BCM1480_HTB_SPLINKFREQ_FREQCAPSET)
#define V_BCM1480_HTB_SPLINKFREQ_FREQCAPSET(x) _SB_MAKEVALUE_32(x,S_BCM1480_HTB_SPLINKFREQ_FREQCAPSET)
#define G_BCM1480_HTB_SPLINKFREQ_FREQCAPSET(x) _SB_GETVALUE_32(x,S_BCM1480_HTB_SPLINKFREQ_FREQCAPSET,M_BCM1480_HTB_SPLINKFREQ_FREQCAPSET)

/*
 * HT Bridge Node Routing Registers (Table 161)
 */

#define S_BCM1480_HTB_NODEROUTE(n)           (4*(n))
#define M_BCM1480_HTB_NODEROUTE(n)           _SB_MAKEMASK_32(4,S_BCM1480_HTB_NODEROUTE(n))
#define V_BCM1480_HTB_NODEROUTE(x,n)         _SB_MAKEVALUE_32(x,S_BCM1480_HTB_NODEROUTE(n))
#define G_BCM1480_HTB_NODEROUTE(x,n)         _SB_GETVALUE_32(x,S_BCM1480_HTB_NODEROUTE(n),M_BCM1480_HTB_NODEROUTE(n))

/* The following fields are per nibble */
#define M_BCM1480_HTB_ROUTE_IS_ON_SEC_FOR_IO _SB_MAKEMASK1_32(0)
#define M_BCM1480_HTB_ROUTE_OVERRIDE_FOR_IO  _SB_MAKEMASK1_32(1)
#define M_BCM1480_HTB_ROUTE_IS_ON_SEC_FOR_CC _SB_MAKEMASK1_32(2)
#define M_BCM1480_HTB_ROUTE_OVERRIDE_FOR_CC  _SB_MAKEMASK1_32(3)


/*
 * HT Bridge Switch Info Register Bits (Table 164)
 */
#define M_BCM1480_HTB_SWITCHINFO_HIDEPORT     _SB_MAKEMASK1_32(23)

#endif /* _BCM1480_HT_H */
