/*	$NetBSD: emipsreg.h,v 1.1.8.2 2011/06/06 09:05:18 jruoho Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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

/*
 * Reference:
 * TBD MSR techreport by Richard Pittman and Alessandro Forin
 *
 * Definitions for the Xilinx ML40x dev boards with MSR's eMIPS.
 */

#ifndef _MIPS32_EMIPS_EMIPSREG_H_
#define _MIPS32_EMIPS_EMIPSREG_H_ 1

/*
 * Peripheral Mapping Table (PMT) definitions
 *
 * Each entry in this table holds the physical address of a section of peripherals
 * and the tag for the type of peripherals in that section.
 * Peripherals of the same type go into the same section, which is subdivided
 * as appropriate for that peripheral type.
 * Each section is at least 64KB wide, each subdivision is at least 4KB wide.
 *
 * NB: This table grows *down* from the top of the address space
 * The value 0xffffffff for an entry indicates the section is not populated.
 * The tag 0xffff is therefore invalid.  
 * The end-of-table is indicated by the first invalid entry.
 *
 * Entries in the table are (preferably) in the processor's own byteorder.
 * The first entry is for the table itself and has a known tag.
 * From this software can verify the byteorder is correct.
 *
 * For each section we also define here its favorite placement in the address space.
 * Software should verify the table for the ultimate truth, peripherals might
 * or might not be present in the FPGA bitfile being used.
 *
 */
#ifndef __ASSEMBLER__
struct _Pmt {

    volatile uint16_t   TopOfPhysicalAddress;
    volatile uint16_t   Tag;
};
#define ThePmt (((struct _Pmt *)0)-1)

#endif /* !__ASSEMBLER__ */

/*
 * Peripheral tags
 */
# define                PMTTAG_END_OF_TABLE             0xffff /* required, last entry */
# define                PMTTAG_BRAM                     0
# define                PMTTAG_PMT                      1      /* required, first entry */
# define                PMTTAG_SRAM                     2
# define                PMTTAG_DDRAM                    3
# define                PMTTAG_FLASH                    4
# define                PMTTAG_INTERRUPT_CONTROLLER     5
# define                PMTTAG_USART                    6
# define                PMTTAG_TIMER                    7
# define                PMTTAG_WATCHDOG                 8
# define                PMTTAG_GPIO                     9
# define                PMTTAG_SYSTEM_ACE               10
# define                PMTTAG_LCD                      11
# define                PMTTAG_PS2                      12
# define                PMTTAG_VGA                      13
# define                PMTTAG_ETHERNET                 14
# define                PMTTAG_AC97                     15
# define                PMTTAG_POWER_MGR                16
# define                PMTTAG_EXTENSION_CONTROLLER     17
# define                PMTTAG_ICAP                     18
# define                PMTTAG_LAST_TAG_DEFINED         18

/*
 * Preferred addresses (nb: for the control registers of...)
 */
# define                BRAM_DEFAULT_ADDRESS                   (0xffff << 16)
# define                SRAM_0_DEFAULT_ADDRESS                 (0xfffd << 16)
# define                DDRAM_0_DEFAULT_ADDRESS                (0xfffc << 16)
# define                FLASH_0_DEFAULT_ADDRESS                (0xfffb << 16)
# define                INTERRUPT_CONTROLLER_DEFAULT_ADDRESS   (0xfffa << 16)
# define                USART_DEFAULT_ADDRESS                  (0xfff9 << 16)
# define                TIMER_DEFAULT_ADDRESS                  (0xfff8 << 16)
# define                WATCHDOG_DEFAULT_ADDRESS               (0xfff7 << 16)
# define                GPIO_DEFAULT_ADDRESS                   (0xfff6 << 16)
# define                IDE_DEFAULT_ADDRESS                    (0xfff5 << 16)
# define                LCD_DEFAULT_ADDRESS                    (0xfff4 << 16)
# define                PS2_DEFAULT_ADDRESS                    (0xfff3 << 16)
# define                VGA_DEFAULT_ADDRESS                    (0xfff2 << 16)
# define                ETHERNET_DEFAULT_ADDRESS               (0xfff1 << 16)
# define                POWER_MGR_DEFAULT_ADDRESS              (0xfff0 << 16)
# define                AC97_DEFAULT_ADDRESS                   (0xffef << 16)
# define                EXTENSION_CONTROLLER_DEFAULT_ADDRESS   (0xffee << 16)
# define                ICAP_DEFAULT_ADDRESS                   (0xffed << 16)


/*
 * SRAM controller
 */
#ifndef __ASSEMBLER__
struct _Sram {
    volatile uint32_t   BaseAddressAndTag;    /* rw */
    volatile uint32_t   Control;              /* rw */
};
#else
# define                SRAMBT                          0
# define                SRAMST                          4
#endif /* !__ASSEMBLER__ */

# define                SRAMBT_TAG                      0x0000ffff  /* ro */
# define                SRAMBT_BASE                     0xffff0000  /* rw */

# define                SRAMST_DELAY                    0x0000000f  /* rw */
# define                SRAMST_BURST_INTERLEAVED        0x00000010  /* ro */
# define                SRAMST_BURST_LINEAR             0x00000000  /* ro */
# define                SRAMST_BURST_ENABLE             0x00000020  /* rw */
# define                SRAMST_BURST_DISABLE            0x00000000  /* rw */
# define                SRAMST_CLOCK_MASK               0x00000040  /* rw */
# define                SRAMST_SLEEP                    0x00000080  /* rw */
# define                SRAMST_PARITY                   0x00000f00  /* rw */
# define                SRAMST_RESET                    0x00001000  /* rw */
# define                SRAMST_BUS_8                    0x00002000  /* rw */
# define                SRAMST_BUS_16                   0x00004000  /* rw */
# define                SRAMST_BUS_32                   0x00008000  /* rw */
# define                SRAMST_SIZE                     0xffff0000  /* ro
                                                                     * in bytes, masked */

/*
 * DDRAM controller
 */
#ifndef __ASSEMBLER__
struct _Ddram {
    volatile uint32_t   BaseAddressAndTag;    /* rw */
    volatile uint32_t   Control;              /* rw */
    volatile uint32_t   PreCharge;            /* wo */
    volatile uint32_t   Refresh;              /* wo */
};
#else
# define                DDRAMBT                         0
# define                DDRAMST                         4
# define                DDRAMPC                         8
# define                DDRAMRF                         12
# define                DDRAMCTRL_SIZE                  16
#endif /* !__ASSEMBLER__ */

# define                DDRAMBT_TAG                     0x0000ffff  /* ro */
# define                DDRAMBT_BASE                    0xffff0000  /* rw */

# define                DDRAMST_RST                     0x00000001    /* rw */
# define                DDRAMST_CLR                     0x00000002    /* rw */
# define                DDRAMST_TSTEN                   0x00000004    /* rw */
# define                DDRAMST_BUF                     0x00000008    /* rw */
# define                DDRAMST_CALDNE                  0x00000010    /* ro */
# define                DDRAMST_CALFAIL                 0x00000020    /* ro */
# define                DDRAMST_SERR                    0x00000040    /* ro */
# define                DDRAMST_DERR                    0x00000080    /* ro */
# define                DDRAMST_BURST                   0x00000f00    /* ro */
# define                DDRAMST_OVF                     0x00001000    /* ro */
# define                DDRAMST_BUS8                    0x00002000    /* ro */
# define                DDRAMST_BUS16                   0x00004000    /* ro */
# define                DDRAMST_BUS32                   0x00008000    /* ro */
# define                DDRAMST_SIZE                    0xffff0000    /* ro */

/*
 * FLASH controller
 */
#ifndef __ASSEMBLER__
struct _Flash {
    volatile uint32_t   BaseAddressAndTag;    /* rw */
    volatile uint32_t   Control;              /* rw */
};
#else
# define                FLASHBT                         0
# define                FLASHST                         4
#endif /* !__ASSEMBLER__ */

# define                FLASHBT_TAG                     0x0000ffff  /* ro */
# define                FLASHBT_BASE                    0xffff0000  /* rw */

# define                FLASHST_DELAY                   0x0000000f  /* rw */
# define                FLASHST_RESET_PIN               0x00000010  /* rw */
# define                FLASHST_RESET_CONTROLLER        0x00001000  /* rw */
# define                FLASHST_BUS_8                   0x00002000  /* rw */
# define                FLASHST_BUS_16                  0x00004000  /* rw */
# define                FLASHST_BUS_32                  0x00008000  /* rw */
# define                FLASHST_SIZE                    0xffff0000  /* ro
                                                                     * in bytes, masked */

/*
 * ARM RPS Interrupt Controller (AIC)
 */
#ifndef __ASSEMBLER__
struct _Aic {
    volatile uint32_t   Tag;                  /* ro */
    volatile uint32_t   IrqStatus;            /* rw */
    volatile uint32_t   IrqRawStatus;         /* ro */
    volatile uint32_t   IrqEnable;            /* rw */
    volatile uint32_t   IrqEnableClear; 
    volatile uint32_t   IrqSoft; 
};
#else
# define                AICT                            0
# define                AICS                            4
# define                AICRS                           8
# define                AICEN                           12
# define                AICEC                           16
#endif /* !__ASSEMBLER__ */

# define                AIC_TIMER                       0
# define                AIC_SOFTWARE                    1
# define                AIC_GPIO                        2
# define                AIC_WATCHDOG                    3
# define                AIC_SYSTEM_ACE                  4
# define                AIC_ETHERNET                    5
# define                AIC_PS2                         6
# define                AIC_AC97                        7
# define                AIC_USART                       9
# define                AIC_EXTENSION_CONTROLLER        10
# define                AIC_ICAP                        11
# define                AIC_SYSTEM_ACE2                 12
# define                AIC_VGA                         13
# define                AIC_EXTENSION_2                 29
# define                AIC_EXTENSION_1                 30
# define                AIC_EXTENSION_0                 31

# define                AIC_TIMER_BIT                   (1 << AIC_TIMER)
# define                AIC_SOFTWARE_BIT                (1 << AIC_SOFTWARE)
# define                AIC_GPIO_BIT                    (1 << AIC_GPIO)
# define                AIC_WATCHDOG_BIT                (1 << AIC_WATCHDOG)
# define                AIC_SYSTEM_ACE_BIT              (1 << AIC_SYSTEM_ACE)
# define                AIC_ETHERNET_BIT                (1 << AIC_ETHERNET)
# define                AIC_PS2_BIT                     (1 << AIC_PS2)
# define                AIC_AC97_BIT                    (1 << AIC_AC97)
# define                AIC_USART_BIT                   (1 << AIC_USART)
# define                AIC_EXTENSION_CONTROLLER_BIT    (1 << AIC_EXTENSION_CONTROLLER)
# define                AIC_ICAP_BIT                    (1 << AIC_ICAP)
# define                AIC_SYSTEM_ACE2_BIT             (1 << AIC_SYSTEM_ACE2)
# define                AIC_VGA_BIT                     (1 << AIC_VGA)
# define                AIC_EXTENSION_2_BIT             (1 << AIC_EXTENSION_2)
# define                AIC_EXTENSION_1_BIT             (1 << AIC_EXTENSION_1)
# define                AIC_EXTENSION_0_BIT             (1 << AIC_EXTENSION_0)

/*
 * General Purpose I/O pads controller (GPIO)
 */
#ifndef __ASSEMBLER__
struct _Pio {
    volatile uint32_t  Tag;                  /* ro value=9 NB: All other registers RESET to 0 */
    volatile uint32_t  Enable;               /* rw READ:  0 => high-z, 1 => In/Out based on DIRECTION
                                              *    WRITE: 0 => no effect, 1 => pin is enabled for I/O */
    volatile uint32_t  Disable;              /* wo 0 => no effect, 1 => disabled, set in high-z */
    volatile uint32_t  Direction;            /* rw READ:  0 => input, 1 => output (if enabled)
                                              *    WRITE: 0 => no-effect, 1 => output */
    volatile uint32_t  OutDisable;           /* wo 0 => no effect, 1 => set for input */

    volatile uint32_t  PinData;              /* rw READ:  0 => LOW, 1 => HIGH
                                              *    WRITE: 0 => no effect, 1 => set pin HIGH */
    volatile uint32_t  ClearData;            /* wo 0 => no effect, 1 => set pin LOW */
    volatile uint32_t  PinStatus;            /* ro 0 => LOW, 1 => HIGH */

    volatile uint32_t  IntrStatus;           /* rw READ:  0 => none 1 => pending (regardless of INTRMASK)
                                              *    WRITE: 0 => no-effect, 1 => clear if pending  */
    volatile uint32_t  IntrEnable;           /* rw READ:  0 => none, 1 => enabled
                                              *    WRITE: 0 => no-effect, 1 => enable */
    volatile uint32_t  IntrDisable;          /* wo 0 => no effect, 1 => disable */
    volatile uint32_t  IntrTrigger;          /* rw 0 => intr on level change, 1 => on transition */
    volatile uint32_t  IntrNotLevel;         /* rw 0 => HIGH, 1 => LOW  -- Combinations:
                                              *    Trig Lev InterruptOn..
                                              *    0    0   level high
                                              *    0    1   level low
                                              *    1    0   low to high transition
                                              *    1    1   high to low transition
                                              */
    volatile uint32_t  reserved[3];          /* ro padding to 64 bytes total */
};
#else
# define                PIOT                            0
# define                PIOEN                           4
# define                PIOD                            8
# define                PIODIR                          12
# define                PIOOD                           16
# define                PIOPD                           20
# define                PIOCD                           24
# define                PIOPS                           28
# define                PIOIS                           32
# define                PIOIE                           36
# define                PIOID                           40
# define                PIOIT                           44
# define                PIOINL                          48
#endif /* !__ASSEMBLER__ */

/* DIP switches on SW1 and their known uses */
# define                PIO_SW1_1                       0
# define                PIO_SW1_1_BIT                   (1 << PIO_SW1_1)
# define                PIO_SW1_2                       1
# define                PIO_SW1_2_BIT                   (1 << PIO_SW1_2)
# define                PIO_SW1_3                       2
# define                PIO_SW1_3_BIT                   (1 << PIO_SW1_3)
# define                PIO_SW1_4                       3
# define                PIO_SW1_4_BIT                   (1 << PIO_SW1_4)
# define                PIO_SW1_5                       4
# define                PIO_SW1_5_BIT                   (1 << PIO_SW1_5)
# define                PIO_SW1_6                       5
# define                PIO_SW1_6_BIT                   (1 << PIO_SW1_6)
# define                PIO_SW1_7                       6
# define                PIO_SW1_7_BIT                   (1 << PIO_SW1_7)
# define                PIO_SW1_8                       7
# define                PIO_SW1_8_BIT                   (1 << PIO_SW1_8)
# define                SW1_BOOT_FROM_FLASH             PIO_SW1_1_BIT  /* else USART */
# define                SW1_BOOT_FS_IN_FLASH            PIO_SW1_2_BIT  /* else serplexd via USART */
# define                SW1_BOOT_FROM_SRAM              PIO_SW1_3_BIT  /* else USART */
/* LEDs */
# define                PIO_LED_NORTH                   8
# define                PIO_LED_NORTH_BIT               (1 << PIO_LED_NORTH)
# define                PIO_LED_EAST                    9
# define                PIO_LED_EAST_BIT                (1 << PIO_LED_EAST)
# define                PIO_LED_SOUTH                   10
# define                PIO_LED_SOUTH_BIT               (1 << PIO_LED_SOUTH)
# define                PIO_LED_WEST                    11
# define                PIO_LED_WEST_BIT                (1 << PIO_LED_WEST)
# define                PIO_LED_CENTER                  12
# define                PIO_LED_CENTER_BIT              (1 << PIO_LED_CENTER)
# define                PIO_LED_GP0                     13
# define                PIO_LED_GP0_BIT                 (1 << PIO_LED_GP0)
# define                PIO_LED_GP1                     14
# define                PIO_LED_GP1_BIT                 (1 << PIO_LED_GP1)
# define                PIO_LED_GP2                     15
# define                PIO_LED_GP2_BIT                 (1 << PIO_LED_GP2)
# define                PIO_LED_GP3                     16
# define                PIO_LED_GP3_BIT                 (1 << PIO_LED_GP3)
# define                PIO_LED_ERROR1                  17
# define                PIO_LED_ERROR1_BIT              (1 << PIO_LED_ERROR1)
# define                PIO_LED_ERROR2                  18
# define                PIO_LED_ERROR2_BIT              (1 << PIO_LED_ERROR2)
/* Buttons */
# define                PIO_BUTTON_NORTH                19
# define                PIO_BUTTON_NORTH_BIT            (1 << PIO_BUTTON_NORTH)
# define                PIO_BUTTON_EAST                 20
# define                PIO_BUTTON_EAST_BIT             (1 << PIO_BUTTON_EAST)
# define                PIO_BUTTON_SOUTH                21
# define                PIO_BUTTON_SOUTH_BIT            (1 << PIO_BUTTON_SOUTH)
# define                PIO_BUTTON_WEST                 22
# define                PIO_BUTTON_WEST_BIT             (1 << PIO_BUTTON_WEST)
# define                PIO_BUTTON_CENTER               23
# define                PIO_BUTTON_CENTER_BIT           (1 << PIO_BUTTON_CENTER)

/*
 * Universal Synch/Asynch Receiver/Transmitter (USART)
 */
#ifndef __ASSEMBLER__
struct _Usart {
    volatile uint32_t  Tag;                  /* ro */
    volatile uint32_t  Control;              /* rw */
    volatile uint32_t  IntrEnable;
    volatile uint32_t  IntrDisable;
    volatile uint32_t  IntrMask;
    volatile uint32_t  ChannelStatus; /* all these with.. */
    volatile uint32_t  RxData;
    volatile uint32_t  TxData;
    volatile uint32_t  Baud;
    volatile uint32_t  Timeout;
    volatile uint32_t  reserved[6];          /* ro padding to 64 bytes total */
};
#else
# define                USARTT                          0
# define                USARTC                          4
# define                USARTIE                         8
# define                USARTID                         12
# define                USARTM                          16
# define                USARTST                         20
# define                USARTRX                         24
# define                USARTTX                         28
# define                USARTBD                         32
# define                USARTTO                         36
#endif /* !__ASSEMBLER__ */

# define                USC_RESET                       0x00000001
# define                USC_RSTRX                       0x00000004
# define                USC_RSTTX                       0x00000008
# define                USC_RXEN                        0x00000010
# define                USC_RXDIS                       0x00000020
# define                USC_TXEN                        0x00000040
# define                USC_TXDIS                       0x00000080
# define                USC_RSTSTA                      0x00000100
# define                USC_STTBRK                      0x00000200
# define                USC_STPBRK                      0x00000400
# define                USC_STTO                        0x00000800
# define                USC_CLK_SENDA                   0x00010000
# define                USC_BPC_9                       0x00020000
# define                USC_CLKO                        0x00040000
# define                USC_EVEN                        0x00000000
# define                USC_ODD                         0x00080000
# define                USC_SPACE                       0x00100000 /* forced 0 */
# define                USC_MARK                        0x00180000 /* forced 1 */
# define                USC_NONE                        0x00200000
# define                USC_MDROP                       0x00300000
# define                USC_BPC_5                       0x00000000
# define                USC_BPC_6                       0x00400000
# define                USC_BPC_7                       0x00800000
# define                USC_BPC_8                       0x00c00000
# define                USC_CLKDIV                      0x0f000000
# define                USC_CLKDIV_1                    0x00000000
# define                USC_CLKDIV_2                    0x01000000
# define                USC_CLKDIV_4                    0x02000000
# define                USC_CLKDIV_8                    0x03000000
# define                USC_CLKDIV_16                   0x04000000
# define                USC_CLKDIV_32                   0x05000000
# define                USC_CLKDIV_64                   0x06000000
# define                USC_CLKDIV_128                  0x07000000
# define                USC_CLKDIV_EXT                  0x08000000
# define                USC_1STOP                       0x00000000
# define                USC_1_5STOP                     0x10000000
# define                USC_2STOP                       0x20000000
# define                USC_ECHO                        0x40000000 /* rx->tx, tx disabled */
# define                USC_LOOPBACK                    0x80000000 /* tx->rx, rx/tx disabled */
# define                USC_ECHO2                       0xc0000000 /* rx->tx, rx disabled */

# define                USI_RXRDY                       0x00000001
# define                USI_TXRDY                       0x00000002
# define                USI_RXBRK                       0x00000004
# define                USI_ENDRX                       0x00000008
# define                USI_ENDTX                       0x00000010
# define                USI_OVRE                        0x00000020
# define                USI_FRAME                       0x00000040
# define                USI_PARE                        0x00000080
# define                USI_TIMEOUT                     0x00000100
# define                USI_TXEMPTY                     0x00000200

/*
 * Timer/Counter (TC)
 */
#ifndef __ASSEMBLER__
struct _Tc {
    volatile uint32_t   Tag;                  /* ro */
    volatile uint32_t   Control;              /* rw */
    volatile uint64_t   FreeRunning;
    volatile uint32_t   DownCounterHigh;      /* rw */
    volatile uint32_t   DownCounter;          /* rw */
    volatile uint32_t   reserved[2];          /* ro padding to 32 bytes total */
};
#else
# define                TCT                             0
# define                TCC                             4
# define                TCFH                            8
# define                TCFL                            12
# define                TCCH                            16
# define                TCCL                            20
#endif /* !__ASSEMBLER__ */

# define                TCCT_ENABLE                     0x00000001
# define                TCCT_INT_ENABLE                 0x00000002
# define                TCCT_CLKO                       0x00000008
# define                TCCT_RESET                      0x00000010
# define                TCCT_FINTEN                     0x00000020
# define                TCCT_CLOCK                      0x000000c0
# define                TCCT_INTERRUPT                  0x00000100
# define                TCCT_FINT                       0x00000200
# define                TCCT_OVERFLOW                   0x00000400
# define                TCCT_OVERRUN                    0x00000800

/*
 * LCD controller
 */
#ifndef __ASSEMBLER__
struct _Lcd {
    volatile uint32_t   TypeAndTag;           /* ro */
    volatile uint32_t   Control;              /* rw */
    volatile uint32_t   Data;                 /* wo */
    volatile uint32_t   Refresh;              /* wo */
};
#else
# define                LCDBT                            0
# define                LCDST                            4
# define                LCDPC                            8
# define                LCDRF                            12
#endif /* !__ASSEMBLER__ */

# define                LCDBT_TAG                       0x0000ffff  /* ro */
# define                LCDBT_TYPE                      0xffff0000  /* ro */

# define                LCDST_RST                       0x00000001  /* rw */
/* other bits are type-specific */

/*
 * Watchdog Timer (WD)
 */
#ifndef __ASSEMBLER__
struct _Wd {
    volatile uint32_t   Tag;                  /* ro */
    volatile uint32_t   OvflMode;
    volatile uint32_t   ClockMode;
    volatile uint32_t   Control;
    volatile uint32_t   Status;
    volatile uint32_t   reserved[3];          /* ro padding to 32 bytes total */
};
#else
# define                WDT                             0
# define                WDO                             4
# define                WDM                             8
# define                WDC                             12
# define                WDS                             16
#endif /* !__ASSEMBLER__ */

/*
 * Power Management Controller (PMC)
 */
#ifndef __ASSEMBLER__
struct _Pmc {
    volatile uint32_t   Tag;                            /* ro */
    volatile uint32_t   SystemPowerEnable;              /* rw */
    volatile uint32_t   SystemPowerDisable;             /* wo */
    volatile uint32_t   PeripheralPowerEnable;          /* rw */
    volatile uint32_t   PeripheralPowerDisable;         /* wo */
    volatile uint32_t   reserved[3];                    /* ro padding to 32 bytes total */
};
#else
# define                PMCT                            0
# define                PMCSE                           4
# define                PMCSD                           8
# define                PMCPE                           12
# define                PMCPD                           16
#endif /* !__ASSEMBLER__ */

# define                PMCSC_CPU       0x00000001

/* more as we get more.. */
# define                PMCPC_USART     0x00000001


/*
 * System ACE Controller (SAC)
 */

#ifndef __ASSEMBLER__
struct _Sac {
    volatile uint32_t        Tag;                       /*0x00000000 */
    volatile uint32_t        Control;                   /*0x00000004 */
    volatile uint32_t        reserved0[30];
    volatile uint32_t        BUSMODEREG;                /*0x00000080 */
    volatile uint32_t        STATUS;                    /*0x00000084 */
    volatile uint32_t        ERRORREG;                  /*0x00000088 */
    volatile uint32_t        CFGLBAREG;                 /*0x0000008c */
    volatile uint32_t        MPULBAREG;                 /*0x00000090 */
    volatile uint16_t        VERSIONREG;                /*0x00000094 */
    volatile uint16_t        SECCNTCMDREG;              /*0x00000096 */
    volatile uint32_t        CONTROLREG;                /*0x00000098 */
    volatile uint16_t        reserved1[1];    
    volatile uint16_t        FATSTATREG;                /*0x0000009e */
    volatile uint32_t        reserved2[8];
    volatile uint32_t        DATABUFREG[16];            /*0x000000c0 */
};
#endif /* !__ASSEMBLER__ */

/*    volatile uint32_t        Tag;                      0x00000000 */
# define                SAC_TAG                          0x0000ffff

/*    volatile uint32_t        Control;                  0x00000004 */
# define                SAC_SIZE                         0x0000ffff
# define                SAC_RST                          0x00010000
# define                SAC_BUS8                         0x00020000
# define                SAC_BUS16                        0x00040000
# define                SAC_BUS32                        0x00080000
# define                SAC_IRQ                          0x00100000
# define                SAC_BRDY                         0x00200000
# define                SAC_INTMASK                      0x00c00000
# define                SAC_TDELAY                       0x0f000000
# define                SAC_BUFW8                        0x10000000
# define                SAC_BUFW16                       0x20000000
# define                SAC_BUFW32                       0x40000000
# define                SAC_DEBUG                        0x80000000

/*    volatile uint32_t        BUSMODEREG;               0x00000080 */
# define                SAC_MODE16                       0x00000001

/*    volatile uint32_t        STATUS;                   0x00000084 */
# define                SAC_CFGLOCK                      0x00000001
# define                SAC_MPULOCK                      0x00000002
# define                SAC_CFGERROR                     0x00000004
# define                SAC_CFCERROR                     0x00000008
# define                SAC_CFDETECT                     0x00000010
# define                SAC_DATABUFRDY                   0x00000020
# define                SAC_DATABUFMODE                  0x00000040
# define                SAC_CFGDONE                      0x00000080
# define                SAC_RDYFORCFCMD                  0x00000100
# define                SAC_CFGMODEPIN                   0x00000200
# define                SAC_CFGADDRPINS                  0x0000e000
# define                SAC_CFBSY                        0x00020000
# define                SAC_CFRDY                        0x00040000
# define                SAC_CFDWF                        0x00080000
# define                SAC_CFDSC                        0x00100000
# define                SAC_CFDRQ                        0x00200000
# define                SAC_CFCORR                       0x00400000
# define                SAC_CFERR                        0x00800000

/*    volatile uint32_t        ERRORREG;                 0x00000088 */
# define                SAC_CARDRESETERR                 0x00000001
# define                SAC_CARDRDYERR                   0x00000002
# define                SAC_CARDREADERR                  0x00000004
# define                SAC_CARDWRITEERR                 0x00000008
# define                SAC_SECTORRDYERR                 0x00000010
# define                SAC_CFGADDRERR                   0x00000020
# define                SAC_CFGFAILED                    0x00000040
# define                SAC_CFGREADERR                   0x00000080
# define                SAC_CFGINSTRERR                  0x00000100
# define                SAC_CFGINITERR                   0x00000200
# define                SAC_CFBBK                        0x00000800
# define                SAC_CFUNC                        0x00001000
# define                SAC_CFIDNF                       0x00002000
# define                SAC_CFABORT                      0x00004000
# define                SAC_CFAMNF                       0x00008000

/*    volatile uint16_t        VERSIONREG;               0x00000094 */
# define                SAC_VERREV                       0x000000ff
# define                SAC_VERMINOR                     0x00000f00
# define                SAC_VERMAJOR                     0x0000f000

/*    volatile uint16_t        SECCNTCMDREG;             0x00000096 */
# define                SAC_SECCCNT                      0x000000ff
# define                SAC_CMD                          0x00000700
# define                SAC_CMD_RESETMEMCARD             0x00000100
# define                SAC_CMD_IDENTIFYMEMCARD          0x00000200
# define                SAC_CMD_READMEMCARDDATA          0x00000300
# define                SAC_CMD_WRITEMEMCARDDATA         0x00000400
# define                SAC_CMD_ABORT                    0x00000600

/*    volatile uint32_t        CONTROLREG;               0x00000098 */
# define                SAC_FORCELOCKREQ                 0x00000001
# define                SAC_LOCKREQ                      0x00000002
# define                SAC_FORCECFGADDR                 0x00000004
# define                SAC_FORCECFGMODE                 0x00000008
# define                SAC_CFGMODE                      0x00000010
# define                SAC_CFGSTART                     0x00000020
# define                SAC_CFGSEL                       0x00000040
# define                SAC_CFGRESET                     0x00000080
# define                SAC_DATABUFRDYIRQ                0x00000100
# define                SAC_ERRORIRQ                     0x00000200
# define                SAC_CFGDONEIRQ                   0x00000400
# define                SAC_RESETIRQ                     0x00000800
# define                SAC_CFGPROG                      0x00001000
# define                SAC_CFGADDRBIT                   0x0000e000
# define                SAC_CFGADDR_0                    0x00000000
# define                SAC_CFGADDR_1                    0x00002000
# define                SAC_CFGADDR_2                    0x00004000
# define                SAC_CFGADDR_3                    0x00006000
# define                SAC_CFGADDR_4                    0x00008000
# define                SAC_CFGADDR_5                    0x0000a000
# define                SAC_CFGADDR_6                    0x0000c000
# define                SAC_CFGADDR_7                    0x0000e000
# define                SAC_CFGRSVD                      0x00070000

/*    volatile uint16_t        FATSTATREG;               0x0000009e */
# define                SAC_MBRVALID                     0x00000001
# define                SAC_PBRVALID                     0x00000002
# define                SAC_MBRFAT12                     0x00000004
# define                SAC_PBRFAT12                     0x00000008
# define                SAC_MBRFAT16                     0x00000010
# define                SAC_PBRFAT16                     0x00000020
# define                SAC_CALCFAT12                    0x00000040
# define                SAC_CALCFAT16                    0x00000080

/*
 * Extension Controller (EC)
 */

#define EC_MAX_BATS 5
#ifndef __ASSEMBLER__
struct _Ec {
    volatile uint32_t        Tag;                       /*0x00000000 */
    volatile uint32_t        Control;                   /*0x00000004 */
    volatile uint32_t        SlotStatusAndTag;          /*0x00000008 */
    volatile uint32_t        BatOrSize[EC_MAX_BATS];    /*0x0000000c */
};
#endif /* !__ASSEMBLER__ */

/*    volatile uint32_t        Tag;                      0x00000000 */
# define                ECT_TAG                          0x0000ffff

/*    volatile uint32_t        Control;                  0x00000004 */
# define                ECC_SIZE                         0x0000ffff
# define                ECC_RST                          0x00010000
# define                ECC_IRQ                          0x00020000  /* write-one-to-clear */
# define                ECC_IRQ_ENABLE                   0x00040000
# define                ECC_INT_LOAD                     0x00100000
# define                ECC_INT_UNLOAD                   0x00200000
# define                ECC_SLOTNO                       0xff000000
# define                ECC_SLOTNO_SHIFT                 24
# define                ECC_WANTS_INTERRUPT              0x00400000  /* extension needs interrupt */
# define                ECC_PRIVILEDGED                  0x00800000  /* extension needs priv interface */

/*    volatile uint32_t        SlotStatusAndTag;         0x00000008 */
# define                ECS_TAG                          0x0000ffff  /* of the selected slot */
# define                ECS_STATUS                       0x00ff0000
# define                ECS_ABSENT                       0x00000000  /* not loaded */
# define                ECS_CONFIG                       0x00010000  /* needs configuration */
# define                ECS_RUN                          0x00020000  /* running ok */
# define                ECS_PM0                          0x00030000  /* power management state 0 */
# define                ECS_PM1                          0x00040000  /* power management state 1 */
# define                ECS_PM2                          0x00050000  /* power management state 2 */

/*    volatile uint32_t        BatOrSize[5];             0x0000000c */
/* In the CONFIG state reads the size (flipped decode mask) for the corresponding bat */
# define                ECB_SIZE_NONE                    0x00000000
# define                ECB_SIZE_4                       0x00000003
# define                ECB_SIZE_8                       0x00000007 /* and so on, 2^N */
/* In non-CONFIG states reads the value of the corresponding Base Address Translation */
/* In all states, writes back to the corresponding BAT */
# define                ECB_BAT_VALID                    0x00000001
# define                ECB_BAT                          0xfffffff8

/*
 * Common interface for packet-based device interfaces (CPBDI)
 */

#ifndef __ASSEMBLER__
#define CPBDI_STRUCT_DECLARATION {                                                  \
    volatile uint32_t   Tag;                  /* ro */                              \
    volatile uint32_t   Control;              /* rw */                              \
    /* FIFO interface. Write-> input FIFO; Read-> output FIFO */                    \
    volatile uint32_t  SizeAndFlags;                                                \
    volatile uint32_t  BufferAddressHi32;                                           \
    volatile uint32_t  BufferAddressLo32;  /* write/read of this word acts */       \
    volatile uint32_t  Pad[3];             /* round to 32bytes */                   \
}

struct _Cpbdi    CPBDI_STRUCT_DECLARATION;
#else
# define                CPBDIT                           0
# define                CPBDIC                           4
# define                CPBDIS                           8
# define                CPBDIH                           12
# define                CPBDIL                           16
# define                CPBDI_SIZE                       32
#endif /* !__ASSEMBLER__ */

/* Common defines for Control register */
# define                CPBDI_RESET                     0x00000001   /* autoclear */
# define                CPBDI_INTEN                     0x00000002   /* interrupt enable */
# define                CPBDI_DONE                      0x00000004   /* interrupt pending aka done */
# define                CPBDI_IF_FULL                   0x00000010   /* input fifo full */
# define                CPBDI_OF_EMPTY                  0x00000020   /* output fifo empty */
# define                CPBDI_URUN                      0x00000040   /* recvr ran out of buffers */
# define                CPBDI_ERROR                     0x80000000   /* unrecoverable error */

/* Common defines for SizeAndFlags register */
# define                CPBDI_F_MASK                    0xf0000000
# define                CPBDI_F_DONE                    0x80000000
# define                CPBDI_F_XMIT                    0x00000000
# define                CPBDI_F_RECV                    0x10000000
# define                CPBDI_F_CMD                     0x20000000

/*
 * Ethernet interface (ENIC)
 */

#ifndef __ASSEMBLER__
struct _Enic    CPBDI_STRUCT_DECLARATION;
#else
# define                ENICT                           CPBDIT
# define                ENICC                           CPBDIC
# define                ENICS                           CPBDIS
# define                ENICH                           CPBDIH
# define                ENICL                           CPBDIL
# define                ENIC_SIZE                       CPBDI_SIZE
#endif /* !__ASSEMBLER__ */

# define                EC_RESET                        CPBDI_RESET
# define                EC_INTEN                        CPBDI_INTEN
# define                EC_DONE                         CPBDI_DONE
# define                EC_RXDIS                        0x00000008   /* recv disabled */
# define                EC_IF_FULL                      CPBDI_IF_FULL
# define                EC_OF_EMPTY                     CPBDI_OF_EMPTY
# define                EC_URUN                         CPBDI_URUN
# define                EC_ERROR                        CPBDI_ERROR
# define                EC_WMASK                        0x0000000b   /* user-writeable bits */

# define                ES_F_MASK                       CPBDI_F_MASK
# define                ES_F_DONE                       CPBDI_F_DONE
# define                ES_F_XMIT                       CPBDI_F_XMIT
# define                ES_F_RECV                       CPBDI_F_RECV
# define                ES_F_CMD                        CPBDI_F_CMD
# define                ES_S_MASK                       0xFFFF

/* Command codes in a command buffer (first byte)
 */
#define ENIC_CMD_NOP           0x00
#define ENIC_CMD_GET_INFO      0x01
#ifndef __ASSEMBLER__
typedef struct {
    uint8_t  InputFifoSize;
    uint8_t  OutputFifoSize;
    uint8_t  CompletionFifoSize;
    uint8_t  ErrorCount;
    uint16_t FramesDropped;
    uint16_t Reserved;
} ENIC_INFO, *PENIC_INFO;
#endif /* !__ASSEMBLER__ */

#define ENIC_CMD_GET_ADDRESS   0x02
#ifndef __ASSEMBLER__
typedef struct {
    uint8_t  Mac[6];
} ENIC_MAC, *PENIC_MAC;
#endif /* !__ASSEMBLER__ */

/*
 * Internal Configuration Access Point (ICAP) interface
 */

#ifndef __ASSEMBLER__
struct _Icap    CPBDI_STRUCT_DECLARATION;
#else
# define                ICAPT                           CPBDIT
# define                ICAPC                           CPBDIC
# define                ICAPS                           CPBDIS
# define                ICAPH                           CPBDIH
# define                ICAPL                           CPBDIL
# define                ICAP_SIZE                       CPBDI_SIZE
#endif /* !__ASSEMBLER__ */

# define                ICAPC_RESET                     CPBDI_RESET
# define                ICAPC_INTEN                     CPBDI_INTEN
# define                ICAPC_DONE                      CPBDI_DONE
# define                ICAPC_IF_FULL                   CPBDI_IF_FULL
# define                ICAPC_OF_EMPTY                  CPBDI_OF_EMPTY
# define                ICAPC_ERROR                     CPBDI_ERROR
# define                ICAPC_WMASK                     0x00000007   /* user-writeable bits */

# define                ICAPS_F_MASK                    CPBDI_F_MASK
# define                ICAPS_F_DONE                    CPBDI_F_DONE
# define                ICAPS_F_XMIT                    CPBDI_F_XMIT
# define                ICAPS_F_RECV                    CPBDI_F_RECV
# define                ICAPS_F_CMD                     CPBDI_F_CMD  /* TBD */
# define                ICAPS_S_MASK                    0x0FFFFFFF

/*
 * Extensible Video Graphic Array (EVGA) interface
 */

#ifndef __ASSEMBLER__
struct _Evga    CPBDI_STRUCT_DECLARATION;
#else
# define                EVGAT                           CPBDIT
# define                EVGAC                           CPBDIC
# define                EVGAS                           CPBDIS
# define                EVGAH                           CPBDIH
# define                EVGAL                           CPBDIL
# define                EVGA_SIZE                       CPBDI_SIZE
#endif /* !__ASSEMBLER__ */

# define                EVGAC_RESET                     CPBDI_RESET
# define                EVGAC_INTEN                     CPBDI_INTEN
# define                EVGAC_DONE                      CPBDI_DONE
# define                EVGAC_IF_FULL                   CPBDI_IF_FULL
# define                EVGAC_OF_EMPTY                  CPBDI_OF_EMPTY
# define                EVGAC_ERROR                     CPBDI_ERROR
# define                EVGAC_WMASK                     0x00000007   /* user-writeable bits */

# define                EVGAS_F_MASK                    CPBDI_F_MASK
# define                EVGAS_F_DONE                    CPBDI_F_DONE
# define                EVGAS_F_XMIT                    CPBDI_F_XMIT
# define                EVGAS_F_RECV                    CPBDI_F_RECV
# define                EVGAS_F_CMD                     CPBDI_F_CMD
# define                EVGAS_S_MASK                    0x0FFFFFFF

/* Command codes in a command buffer (first byte) */
#define EVGA_CMD_NOP           0x00
#define EVGA_CMD_IDENTIFY      0x01
#ifndef __ASSEMBLER__
typedef struct {
    uint8_t  CommandEcho;
    uint8_t  InterfaceVersion;
    uint16_t Size;                        /* of this structure, all bytes counted */
    uint16_t PciVendorId;                 /* See PCI catalog */
    uint16_t PciProductId;                /* See PCI catalog */
    uint32_t StandardCaps;                /* TBD */
    uint8_t  InputFifoSize;
    uint8_t  OutputFifoSize;
    uint8_t  CompletionFifoSize;
    uint8_t  ErrorCount;
    /* More as needed */
} EVGA_IDENTIFY, *PEVGA_IDENTIFY;
#endif /* !__ASSEMBLER__ */
#define EVGA_IDENTIFY_BIG_ENDIAN                        0x00
#define EVGA_IDENTIFY_LITTLE_ENDIAN                     0x80
#define EVGA_IDENTIFY_VERSION_1                         0x01


#define EVGA_CMD_2D_SET_BASE   0x02
#define EVGA_CMD_2D_GET_BASE   0x03
#ifndef __ASSEMBLER__
typedef struct {
    uint8_t  CommandEcho;
    uint8_t  Pad[3];
    uint32_t AddressLow32;
    uint32_t AddressHigh32;
} EVGA_2D_BASE, *PEVGA_2D_BASE;

#endif /* !__ASSEMBLER__ */



#endif /* _MIPS32_EMIPS_EMIPSREG_H_ */
