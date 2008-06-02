/* $NetBSD: pic16lcreg.h,v 1.3.42.1 2008/06/02 13:23:17 mjf Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _DEV_I2C_PIC16LCREG_H
#define _DEV_I2C_PIC16LCREG_H

#define PIC16LC_REG_VER                 0x01
#define PIC16LC_REG_POWER               0x02
#define         PIC16LC_REG_POWER_RESET         0x01
#define         PIC16LC_REG_POWER_CYCLE         0x40
#define         PIC16LC_REG_POWER_SHUTDOWN      0x80
#define PIC16LC_REG_TRAYSTATE           0x03
#define PIC16LC_REG_AVPACK              0x04
#define         PIC16LC_REG_AVPACK_SCART        0x00
#define         PIC16LC_REG_AVPACK_HDTV         0x01
#define         PIC16LC_REG_AVPACK_VGA_SOG      0x02
#define         PIC16LC_REG_AVPACK_SVIDEO       0x04
#define         PIC16LC_REG_AVPACK_COMPOSITE    0x06
#define         PIC16LC_REG_AVPACK_VGA          0x07
#define PIC16LC_REG_FANMODE             0x05
#define PIC16LC_REG_FANSPEED            0x06
#define PIC16LC_REG_LEDMODE             0x07
#define PIC16LC_REG_LEDSEQ              0x08
#define PIC16LC_REG_CPUTEMP             0x09
#define PIC16LC_REG_BOARDTEMP           0x0a
#define PIC16LC_REG_TRAYEJECT           0x0c
#define PIC16LC_REG_INTACK		0x0d
#define PIC16LC_REG_INTSTATUS           0x11
#define		PIC16LC_REG_INTSTATUS_POWER		0x01
#define		PIC16LC_REG_INTSTATUS_TRAYCLOSED	0x02
#define		PIC16LC_REG_INTSTATUS_TRAYOPENING	0x04
#define		PIC16LC_REG_INTSTATUS_AVPACK_PLUG	0x08
#define		PIC16LC_REG_INTSTATUS_AVPACK_UNPLUG	0x10
#define		PIC16LC_REG_INTSTATUS_EJECT_BUTTON	0x20
#define		PIC16LC_REG_INTSTATUS_TRAYCLOSING	0x40
#define PIC16LC_REG_RESETONEJECT        0x19
#define PIC16LC_REG_INTEN               0x1a

#endif /* !_DEV_I2C_PIC16LCREG_H */
