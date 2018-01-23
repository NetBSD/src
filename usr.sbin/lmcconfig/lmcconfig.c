/*-
 * $NetBSD: lmcconfig.c,v 1.12 2018/01/23 22:12:52 sevan Exp $
 *
 * First author: Michael Graff.
 * Copyright (c) 1997-2000 Lan Media Corp.
 * All rights reserved.
 *
 * Second author: Andrew Stanley-Jones.
 * Copyright (c) 2000-2002 SBE Corp.
 * All rights reserved.
 *
 * Third author: David Boggs.
 * Copyright (c) 2002-2006 David Boggs.
 * All rights reserved.
 *
 * BSD License:
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * GNU General Public License:
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Description:
 *
 * This program configures the Unix/Linux device driver
 *  for LMC wide area network interface cards.
 * A complete man page for this program exists.
 * This is a total rewrite of the program 'lmcctl' by
 *  Michael Graff, Rob Braun and Andrew Stanley-Jones.
 *
 * If Netgraph is present (FreeBSD only):
 *    cc -o lmcconfig -l netgraph -D NETGRAPH lmcconfig.c
 * If Netgraph is NOT present:
 *    cc -o lmcconfig lmcconfig.c
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#if defined(NETGRAPH)
# include <netgraph.h>
#endif
#include <sys/cdefs.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <net/if.h>
/* and finally... */
# include "if_lmc.h"

/* procedure prototypes */

void usage(void);

void call_driver(unsigned long, struct iohdr *);

u_int32_t read_pci_config(u_int8_t);
void     write_pci_config(u_int8_t, u_int32_t);
u_int32_t read_csr(u_int8_t);
void     write_csr(u_int8_t, u_int32_t);
u_int16_t read_srom(u_int8_t);
void     write_srom(u_int8_t, u_int16_t);
u_int8_t  read_bios_rom(u_int32_t);
void     write_bios_rom(u_int32_t, u_int8_t);
u_int16_t read_mii(u_int8_t);
void     write_mii(u_int8_t, u_int16_t);
u_int8_t  read_framer(u_int16_t);
void     write_framer(u_int16_t, u_int8_t);

void write_synth(struct synth);
void write_dac(u_int16_t);

void reset_xilinx(void);
void load_xilinx_from_rom(void);
void load_xilinx_from_file(char *, int);

void ioctl_snmp_send(u_int32_t);
void ioctl_snmp_loop(u_int32_t);
void ioctl_reset_cntrs(void);
void ioctl_read_config(void);
void ioctl_write_config(void);
void ioctl_read_status(void);

void print_card_name(void);
void print_card_type(void);
void print_status(void);
void print_tx_speed(void);
void print_debug(void);
void print_line_prot(void);
void print_crc_len(void);
void print_loop_back(int);
void print_tx_clk_src(void);
void print_format(void);
void print_dte_dce(void);
void print_synth_freq(void);
void synth_freq(unsigned long);
void print_cable_len(void);
void print_cable_type(void);
void print_time_slots(void);
void print_scrambler(void);
double vga_dbs(u_int8_t);
void print_rx_gain_max(void);
void print_tx_lbo(void);
void print_tx_pulse(int);
void print_ssi_sigs(void);
void print_hssi_sigs(void);
void print_events(void);
void print_summary(void);

const char *print_t3_bop(int);
void print_t3_snmp(void);
void print_t3_dsu(void);
void t3_cmd(int, char **);

const char *print_t1_bop(int);
void print_t1_test_pattern(int);
void print_t1_far_report(int);
void print_t1_snmp(void);
void print_t1_dsu(void);
void t1_cmd(int, char **);

unsigned char read_hex(FILE *);
void load_xilinx(char *);

u_int32_t crc32(u_int8_t *, int);
u_int8_t  crc8(u_int16_t *, int);

void main_cmd(int, char **);

/* program global variables */

char *		progname;	/* name of this program */
const char *	ifname;		/* interface name */
int		fdcs;		/* ifnet File Desc or ng Ctl Socket */
struct status	status;		/* card status (read only) */
struct config	config;		/* card configuration (read/write) */
int		netgraph = 0;	/* non-zero if netgraph present */
int		summary  = 0;	/* print summary at end */
int		update   = 0;	/* update driver config */
int		verbose  = 0;	/* verbose output */
unsigned int	waittime = 0;	/* time in seconds between status prints */
u_int8_t	checksum;	/* gate array ucode file checksum */

void usage()
  {
  fprintf(stderr, "Usage: %s interface [-abBcCdDeEfgGhiLmMpPsStTuUvVwxXyY?]\n", progname);
  fprintf(stderr, "or\n");
  fprintf(stderr, "Usage: %s interface -1 [-aABcdeEfFgiIlLpPstTuUvxX]\n", progname);
  fprintf(stderr, "or\n");
  fprintf(stderr, "Usage: %s interface -3 [-aABcdefFlLsSvV]\n\n", progname);
  fprintf(stderr, "\tInterface is the interface name, e.g. '%s'\n", ifname);
#if defined(NETGRAPH)
  fprintf(stderr, "\tIf interface name ends with ':' then use netgraph\n");
#endif
  fprintf(stderr, "\t-1 following parameters apply to T1E1 cards\n");
  fprintf(stderr, "\t-3 following parameters apply to T3 cards\n");
  fprintf(stderr, "\t-a <number> Set Tx clock source, where:\n");
  fprintf(stderr, "\t   1:modem Tx clk 2:xtal osc 3:modem Rx Clk 4:ext conn\n");
  fprintf(stderr, "\t-b Read and print bios rom addrs 0-255\n");
  fprintf(stderr, "\t-B Write bios rom with address pattern\n");
  fprintf(stderr, "\t-c Set 16-bit CRC (default)\n");
  fprintf(stderr, "\t-C Set 32-bit CRC\n");
  fprintf(stderr, "\t-d Clear driver DEBUG flag\n");
  fprintf(stderr, "\t-D Set driver DEBUG flag (more log msgs)\n");
  fprintf(stderr, "\t-e Set DTE mode (default)\n");
  fprintf(stderr, "\t-E Set DCE mode\n");
  fprintf(stderr, "\t-f <number> Set synth osc freq in bits/sec\n");
  fprintf(stderr, "\t-g Load gate array from ROM\n");
  fprintf(stderr, "\t-G <filename> Load gate array from file\n");
  fprintf(stderr, "\t-h Help: this usage message\n");
  fprintf(stderr, "\t-i Interface name (eg, lmc0)\n");
  fprintf(stderr, "\t-L <number> Set loopback, where:\n");
  fprintf(stderr, "\t   1:none 2:payload 3:line 4:other 5: inward\n");
  fprintf(stderr, "\t   6:dual 16:Tulip 17:pins 18:LA/LL 19:LB/RL\n");
  fprintf(stderr, "\t-m Read and print MII regs\n");
  fprintf(stderr, "\t-M <addr> <data> Write MII reg\n");
  fprintf(stderr, "\t-p Read and print PCI config regs\n");
  fprintf(stderr, "\t-P <addr> <data> Write PCI config reg\n");
  fprintf(stderr, "\t-s Read and print Tulip SROM\n");
  fprintf(stderr, "\t-S <number> Initialize Tulip SROM\n");
  fprintf(stderr, "\t-t Read and print Tulip Control/Status regs\n");
  fprintf(stderr, "\t-T <addr> <data> Write Tulip Control/status reg\n");
  fprintf(stderr, "\t-u Reset event counters\n");
  fprintf(stderr, "\t-U Reset gate array\n");
  fprintf(stderr, "\t-v Set verbose printout mode\n");
  fprintf(stderr, "\t-V Print card configuration\n");
  fprintf(stderr, "\t-w <number> Seconds between status prints\n");
  fprintf(stderr, "\t-x <number> Set line protocol where:\n");
  fprintf(stderr, "\t   1:RAW-IP 2:PPP 3:C-HDLC 4:FRM-RLY 5:RAW-ETH\n");
  fprintf(stderr, "\t-X <number> Set line package where:\n");
  fprintf(stderr, "\t   1:RAW-IP 2:SPPP 3:P2P 4:GEN-HDLC 5:SYNC-PPP\n");
  fprintf(stderr, "\t-y Disable SPPP keep-alive packets\n");
  fprintf(stderr, "\t-Y  Enable SPPP keep-alive packets\n");

  fprintf(stderr, "The -1 switch precedes T1/E1 commands.\n");
  fprintf(stderr, "\t-a <y|b|a> Stop  sending Yellow|Blue|AIS signal\n");
  fprintf(stderr, "\t-A <y|b|a> Start sending Yellow|Blue}AIS signal\n");
  fprintf(stderr, "\t-B <number> Send BOP msg 25 times\n");
  fprintf(stderr, "\t-c <number> Set cable length in meters\n");
  fprintf(stderr, "\t-d Print status of T1 DSU/CSU\n");
  fprintf(stderr, "\t-e <number> Set framing format, where:\n");
  fprintf(stderr, "\t   27:T1-ESF 9:T1-SF 0:E1-FAS 8:E1-FAS+CRC\n");
  fprintf(stderr, "\t   16:E1-FAS+CAS 24:E1-FAS+CRC+CAS 32:E1-NO-FRAMING\n");
  fprintf(stderr, "\t-E <32-bit hex number> 1 activates a channel and 0 deactivates it.\n");
  fprintf(stderr, "\t   Use this to config a link in fractional T1/E1 mode\n");
  fprintf(stderr, "\t-f Read and print Framer/LIU registers\n");
  fprintf(stderr, "\t-F <addr> <data> Write Framer/LIU register\n");
  fprintf(stderr, "\t-g <number> Set receiver gain, where:\n");
  fprintf(stderr, "\t   0:short range  1:medium range\n");
  fprintf(stderr, "\t   2:long range   3:extended range\n");
  fprintf(stderr, "\t   4:auto-set based on cable length\n");
  fprintf(stderr, "\t-i Send 'CSU Loop Down' inband msg\n");
  fprintf(stderr, "\t-I Send 'CSU Loop Up' inband msg\n");
  fprintf(stderr, "\t-l Send 'Line Loop Down' BOP msg\n");
  fprintf(stderr, "\t-L Send 'Line Loop Up' BOP msg\n");
  fprintf(stderr, "\t-p Send 'Payload Loop Down' BOP msg\n");
  fprintf(stderr, "\t-P Send 'Payload Loop Up' BOP msg\n");
  fprintf(stderr, "\t-s Print status of T1 DSU/CSU\n");
  fprintf(stderr, "\t-t Stop sending test pattern\n");
  fprintf(stderr, "\t-T <number> Start sending test pattern, where:\n");
  fprintf(stderr, "\t    0:unframed 2^11       1:unframed 2^15\n");
  fprintf(stderr, "\t    2:unframed 2^20       3:unframed 2^23\n");
  fprintf(stderr, "\t    4:unframed 2^11 w/ZS  5:unframed 2^15 w/ZS\n");
  fprintf(stderr, "\t    6:unframed QRSS       7:unframed 2^23 w/ZS\n");
  fprintf(stderr, "\t    8:  framed 2^11       9:  framed 2^15\n");
  fprintf(stderr, "\t   10:  framed 2^20      11:  framed 2^23\n");
  fprintf(stderr, "\t   12:  framed 2^11 w/ZS 13:  framed 2^15 w/ZS\n");
  fprintf(stderr, "\t   14:  framed QRSS      15:  framed 2^23 w/ZS\n");
  fprintf(stderr, "\t-u <number> Set transmitter pulse shape, where:\n");
  fprintf(stderr, "\t   0:T1-DSX   0-40m       1:T1-DSX  40-80m\n");
  fprintf(stderr, "\t   2:T1-DSX  80-120m      3:T1-DSX 120-160m\n");
  fprintf(stderr, "\t   4:T1-DSX 160-200m      5:E1-G.703 75ohm coax\n");
  fprintf(stderr, "\t   6:E1-G.703 120ohm TP   7:T1-CSU Long range\n");
  fprintf(stderr, "\t   8:auto-set based on cable length (T1 only)\n");
  fprintf(stderr, "\t-U <number> Set line build out where:\n");
  fprintf(stderr, "\t   0:0dB 1:7.5dB 2:15dB 3:22.5dB\n");
  fprintf(stderr, "\t   4:auto-set based on cable length\n");
  fprintf(stderr, "\t-x Disable Transmitter outputs\n");
  fprintf(stderr, "\t-X  Enable Transmitter outputs\n");

  fprintf(stderr, "The -3 switch precedes T3 commands.\n");
  fprintf(stderr, "\t-a <y|b|a|i> Stop  sending Yellow|Blue|AIS|Idle signal\n");
  fprintf(stderr, "\t-A <y|b|a|i> Start sending Yellow|Blue|AIS|Idle signal\n");
  fprintf(stderr, "\t-B <bopcode> Send BOP msg 10 times\n");
  fprintf(stderr, "\t-c <number> Set cable length in meters\n");
  fprintf(stderr, "\t-d Print status of T3 DSU/CSU\n");
  fprintf(stderr, "\t-e <number> Set T3 frame format, where:\n");
  fprintf(stderr, "\t   100:C-Bit Parity  101:M13\n");
  fprintf(stderr, "\t-f Read and print Framer registers\n");
  fprintf(stderr, "\t-F <addr> <data> Write Framer register\n");
  fprintf(stderr, "\t-l Send 'Line Loop Down' BOP msg\n");
  fprintf(stderr, "\t-L Send 'Line Loop Up' BOP msg\n");
  fprintf(stderr, "\t-s Print status of T3 DSU/CSU\n");
  fprintf(stderr, "\t-S <number> Set DS3 scrambler mode, where:\n");
  fprintf(stderr, "\t   1:OFF 2:DigitalLink|Kentrox 3:Larse\n");
  fprintf(stderr, "\t-V <number> Write to T3 VCXO freq control DAC\n");
  }

void call_driver(unsigned long cmd, struct iohdr *iohdr)
  {
  strncpy(iohdr->ifname, ifname, sizeof(iohdr->ifname));
  iohdr->cookie = NGM_LMC_COOKIE;
  iohdr->iohdr = iohdr;

  /* Exchange data with a running device driver. */
#if defined(NETGRAPH)
  if (netgraph)
    {
    NgSendMsg(fdcs, ifname, NGM_LMC_COOKIE, cmd, iohdr, IOCPARM_LEN(cmd));
    if (cmd & IOC_OUT)
      {
      int replen = sizeof(struct ng_mesg) + IOCPARM_LEN(cmd);
      char rep[replen];  /* storage for the reply */
      struct ng_mesg *reply = (struct ng_mesg *)rep;
      int rl = NgRecvMsg(fdcs, reply, replen, NULL);
      if (rl == replen)
        bcopy(&reply->data, iohdr, IOCPARM_LEN(cmd));
      else
        {
        fprintf(stderr, "%s: NgRecvMsg returned %d bytes, expected %d\n",
          progname, rl, replen);
        exit(1);
	}
      }
    }
  else
#endif
    {
    if (ioctl(fdcs, cmd, (caddr_t)iohdr) < 0)
      {
      fprintf(stderr, "%s: ioctl() returned error code %d: %s\n",
       progname, errno, strerror(errno));
      exit(1);
      }
    }

  if (iohdr->cookie != NGM_LMC_COOKIE)
    {
    fprintf(stderr, "%s: cookie is 0x%08X; expected 0x%08X\n",
     progname, iohdr->cookie, NGM_LMC_COOKIE);
    fprintf(stderr, "%s: recompile this program!\n", progname);
    exit(1);
    }
  }

u_int32_t read_pci_config(u_int8_t addr)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_PCI;
  ioc.address = addr;

  call_driver(LMCIOCREAD, &ioc.iohdr);

  return ioc.data;
  }

void write_pci_config(u_int8_t addr, u_int32_t data)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOW;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_PCI;
  ioc.address = addr;
  ioc.data = data;

  call_driver(LMCIOCWRITE, &ioc.iohdr);
  }

u_int32_t read_csr(u_int8_t addr)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_CSR;
  ioc.address = addr;

  call_driver(LMCIOCREAD, &ioc.iohdr);

  return ioc.data;
  }

void write_csr(u_int8_t addr, u_int32_t data)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOW;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_CSR;
  ioc.address = addr;
  ioc.data = data;

  call_driver(LMCIOCWRITE, &ioc.iohdr);
  }

u_int16_t read_srom(u_int8_t addr)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_SROM;
  ioc.address = addr;

  call_driver(LMCIOCREAD, &ioc.iohdr);

  return ioc.data;
  }

void write_srom(u_int8_t addr, u_int16_t data)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOW;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_SROM;
  ioc.address = addr;
  ioc.data = data;

  call_driver(LMCIOCWRITE, &ioc.iohdr);
  }

u_int8_t read_bios_rom(u_int32_t addr)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_BIOS;
  ioc.address = addr;

  call_driver(LMCIOCREAD, &ioc.iohdr);

  return ioc.data;
  }

void write_bios_rom(u_int32_t addr, u_int8_t data)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOW;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_BIOS;
  ioc.address = addr;
  ioc.data = data;

  call_driver(LMCIOCWRITE, &ioc.iohdr);
  }

u_int16_t read_mii(u_int8_t addr)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_MII;
  ioc.address = addr;

  call_driver(LMCIOCREAD, &ioc.iohdr);

  return ioc.data;
  }

void write_mii(u_int8_t addr, u_int16_t data)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOW;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_MII;
  ioc.address = addr;
  ioc.data = data;

  call_driver(LMCIOCWRITE, &ioc.iohdr);
  }

u_int8_t read_framer(u_int16_t addr)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_FRAME;
  ioc.address = addr;

  call_driver(LMCIOCREAD, &ioc.iohdr);

  return ioc.data;
  }

void write_framer(u_int16_t addr, u_int8_t data)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOW;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RW_FRAME;
  ioc.address = addr;
  ioc.data = data;

  call_driver(LMCIOCWRITE, &ioc.iohdr);
  }

void write_synth(struct synth synth)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOW;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_WO_SYNTH;
  bcopy(&synth, &ioc.data, sizeof(synth));

  call_driver(LMCIOCWRITE, &ioc.iohdr);
  }

void write_dac(u_int16_t data)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOW;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_WO_DAC;
  ioc.data = data;

  call_driver(LMCIOCWRITE, &ioc.iohdr);
  }

void reset_xilinx()
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_XILINX_RESET;

  call_driver(LMCIOCTL, &ioc.iohdr);
  }

void load_xilinx_from_rom()
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_XILINX_ROM;

  call_driver(LMCIOCTL, &ioc.iohdr);
  }

void load_xilinx_from_file(char *ucode, int len)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_XILINX_FILE;
  ioc.data = len;
  ioc.ucode = ucode;

  call_driver(LMCIOCTL, &ioc.iohdr);
  }

void ioctl_snmp_send(u_int32_t sendval)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_SNMP_SEND;
  ioc.data = sendval;

  call_driver(LMCIOCTL, &ioc.iohdr);
  }

void ioctl_snmp_loop(u_int32_t loop)
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_SNMP_LOOP;
  ioc.data = loop;

  call_driver(LMCIOCTL, &ioc.iohdr);
  }

void ioctl_reset_cntrs()
  {
  struct ioctl ioc;

  ioc.iohdr.direction = DIR_IOWR;
  ioc.iohdr.length = sizeof(struct ioctl);
  ioc.cmd = IOCTL_RESET_CNTRS;

  call_driver(LMCIOCTL, &ioc.iohdr);
  }

void ioctl_read_config()
  {
  config.iohdr.direction = DIR_IOWR;
  config.iohdr.length = sizeof(struct config);

  call_driver(LMCIOCGCFG, &config.iohdr);
  }

void ioctl_write_config()
  {
  config.iohdr.direction = DIR_IOW;
  config.iohdr.length = sizeof(struct config);

  call_driver(LMCIOCSCFG, &config.iohdr);
  }

void ioctl_read_status()
  {
  status.iohdr.direction = DIR_IOWR;
  status.iohdr.length = sizeof(struct status);

  call_driver(LMCIOCGSTAT, &status.iohdr);
  }

void print_card_name()
  {
  printf("Card name:\t\t%s\n", ifname);
  }

void print_card_type()
  {
  printf("Card type:\t\t");
  switch(status.card_type)
    {
    case CSID_LMC_HSSI:
      printf("HSSI (lmc5200)\n");
      break;
    case CSID_LMC_T3:
      printf("T3 (lmc5245)\n");
      break;
    case CSID_LMC_SSI:
      printf("SSI (lmc1000)\n");
      break;
    case CSID_LMC_T1E1:
      printf("T1E1 (lmc1200)\n");
      break;
    case CSID_LMC_HSSIc:
      printf("HSSI (lmc5200C)\n");
      break;
    default:
      printf("Unknown card_type: %d\n", status.card_type);
      break;
    }
  }

void print_status()
  {
  const char *status_string;

  if      (status.link_state == STATE_UP)
    status_string = "Up";
  else if (status.link_state == STATE_DOWN)
    status_string = "Down";
  else if (status.link_state == STATE_TEST)
    status_string = "Test";
  else
    status_string = "Unknown";
  printf("Link status:\t\t%s\n", status_string);
  }

void print_tx_speed()
  {
  printf("Tx Speed:\t\t%u\n", status.tx_speed);
  }

void print_debug()
  {
  if (config.debug != 0)
    printf("Debug:\t\t\t%s\n", "On");
  }

void print_line_prot()
  {
  const char *on = "On", *off = "Off";

  printf("Line Prot/Pkg:\t\t");
  switch (status.proto)
    {
    case 0:
      printf("NotSet/");
      break;
    case PROTO_IP_HDLC:
      printf("IP-in-HDLC/");
      break;
    case PROTO_PPP:
      printf("PPP/");
      break;
    case PROTO_C_HDLC:
      printf("Cisco-HDLC/");
      break;
    case PROTO_FRM_RLY:
      printf("Frame-Relay/");
      break;
    case PROTO_ETH_HDLC:
      printf("Ether-in-HDLC/");
      break;
    case PROTO_X25:
      printf("X25+LAPB/");
      break;
    default:
      printf("Unknown proto: %d/", status.proto);
      break;
    }

  switch (status.stack)
    {
    case 0:
      printf("NotSet\n");
      break;
    case STACK_RAWIP:
      printf("Driver\n");
      break;
    case STACK_SPPP:
      printf("SPPP\n");
      break;
    case STACK_P2P:
      printf("P2P\n");
      break;
    case STACK_GEN_HDLC:
      printf("GenHDLC\n");
      break;
    case STACK_SYNC_PPP:
      printf("SyncPPP\n");
      break;
    case STACK_NETGRAPH:
      printf("Netgraph\n");
      break;
    default:
      printf("Unknown stack: %d\n", status.stack);
      break;
    }

  if ((status.stack == STACK_SPPP) ||
      (status.stack == STACK_SYNC_PPP) ||
     ((status.stack == STACK_GEN_HDLC) && (status.proto == PROTO_PPP)))
    printf("Keep-alive pkts:\t%s\n", status.keep_alive ? on : off);
  }

void print_crc_len()
  {
  printf("CRC length:\t\t");
  if      (config.crc_len == CFG_CRC_0)
    printf("no CRC\n");
  else if (config.crc_len == CFG_CRC_16)
    printf("16 bits\n");
  else if (config.crc_len == CFG_CRC_32)
    printf("32 bits\n");
  else
    printf("bad crc_len: %d\n", config.crc_len);
  }

void print_loop_back(int skip_none)
  {
  if ((config.loop_back == CFG_LOOP_NONE) && skip_none)
    return;

  printf("Loopback:\t\t");
  switch (config.loop_back)
    {
    case CFG_LOOP_NONE:
      printf("None\n");
      break;
    case CFG_LOOP_PAYLOAD:
      printf("Outward thru framer (payload loop)\n");
      break;
    case CFG_LOOP_LINE:
      printf("Outward thru line interface (line loop)\n");
      break;
    case CFG_LOOP_OTHER:
      printf("Inward thru line interface\n");
      break;
    case CFG_LOOP_INWARD:
      printf("Inward thru framer\n");
      break;
    case CFG_LOOP_DUAL:
      printf("Inward & outward (dual loop)\n");
      break;
    case CFG_LOOP_TULIP:
      printf("Inward thru Tulip chip\n");
      break;
    case CFG_LOOP_PINS:
      printf("Inward thru drvrs/rcvrs\n");
      break;
    case CFG_LOOP_LL:
      printf("LA/LL asserted\n");
      break;
    case CFG_LOOP_RL:
      printf("LB/RL asserted\n");
      break;
    default:
      printf("Unknown loop_back: %d\n", config.loop_back);
      break;
    }
  }

void print_tx_clk_src()
  {
  printf("Tx Clk src:\t\t");
  switch (config.tx_clk_src)
    {
    case CFG_CLKMUX_ST:
      printf("Modem Tx Clk\n");
      break;
    case CFG_CLKMUX_INT:
      printf("Crystal osc\n");
      break;
    case CFG_CLKMUX_RT:
      printf("Modem Rx Clk (loop timed)\n");
      break;
    case CFG_CLKMUX_EXT:
      printf("External connector\n");
      break;
    default:
      printf("Unknown tx_clk_src: %d\n", config.tx_clk_src);
      break;
    }
  }

void print_format()
  {
  printf("Format-Frame/Code:\t");
  switch (config.format)
    {
    case CFG_FORMAT_T1SF:
      printf("T1-SF/AMI\n");
      break;
    case CFG_FORMAT_T1ESF:
      printf("T1-ESF/B8ZS\n");
      break;
    case CFG_FORMAT_E1FAS:
      printf("E1-FAS/HDB3\n");
      break;
    case CFG_FORMAT_E1FASCRC:
      printf("E1-FAS+CRC/HDB3\n");
      break;
    case CFG_FORMAT_E1FASCAS:
      printf("E1-FAS+CAS/HDB3\n");
      break;
    case CFG_FORMAT_E1FASCRCCAS:
      printf("E1-FAS+CRC+CAS/HDB3\n");
      break;
    case CFG_FORMAT_E1NONE:
      printf("E1-NOFRAMING/HDB3\n");
      break;
    case CFG_FORMAT_T3CPAR:
      printf("T3-CParity/B3ZS\n");
      break;
    case CFG_FORMAT_T3M13:
      printf("T3-M13/B3ZS\n");
      break;
    default:
      printf("Unknown format: %d\n", config.format);
      break;
    }
  }

void print_dte_dce()
  {
  printf("DTE or DCE:\t\t");
  switch(config.dte_dce)
    {
    case CFG_DTE:
      printf("DTE (receiving TxClk)\n");
      break;
    case CFG_DCE:
      printf("DCE (driving TxClk)\n");
      break;
    default:
      printf("Unknown dte_dce: %d\n", config.dte_dce);
      break;
    }
  }

void print_synth_freq()
  {
  double Fref = 20e6;
  double Fout, Fvco;

  /* decode the synthesizer params */
  Fvco = (Fref * (config.synth.n<<(3*config.synth.v)))/config.synth.m;
  Fout =  Fvco / (1<<(config.synth.x+config.synth.r+config.synth.prescale));

  printf("Synth freq:\t\t%.0f\n", Fout);
  }

void synth_freq(unsigned long target)
  {
  unsigned int n, m, v, x, r;
  double Fout, Fvco, Ftarg;
  double newdiff, olddiff;
  double bestF=0.0, bestV=0.0;
  unsigned prescale = (target < 50000) ? 9:4;

  Ftarg = target<<prescale;
  for (n=3; n<=127; n++)
    for (m=3; m<=127; m++)
      for (v=0;  v<=1;  v++)
        for (x=0;  x<=3;  x++)
          for (r=0;  r<=3;  r++)
            {
            Fvco = (SYNTH_FREF * (n<<(3*v)))/m;
            if (Fvco < SYNTH_FMIN || Fvco > SYNTH_FMAX) continue;
            Fout =  Fvco / (1<<(x+r));
            if (Fout >= Ftarg)
              newdiff = Fout - Ftarg;
            else
              newdiff = Ftarg - Fout;
            if (bestF >= Ftarg)
              olddiff = bestF - Ftarg;
            else
              olddiff = Ftarg - bestF;
            if ((newdiff < olddiff) ||
               ((newdiff == olddiff) && (Fvco < bestV)))
              {
              config.synth.n = n;
              config.synth.m = m;
              config.synth.v = v;
              config.synth.x = x;
              config.synth.r = r;
              config.synth.prescale = prescale;
              bestF = Fout;
              bestV = Fvco;
	      }
            }
#if 0
  printf("Fbest=%.0f, Ftarg=%u, Fout=%.0f\n", bestF>>prescale, target, bestF);
  printf("N=%u, M=%u, V=%u, X=%u, R=%u\n", config.synth.n,
   config.synth.m, config.synth.v, config.synth.x, config.synth.r);
#endif
  }

void print_cable_len()
  {
  printf("Cable length:\t\t%d meters\n", config.cable_len);
  }

void print_cable_type()
  {
  printf("Cable type:\t\t");
  if (status.cable_type > 7)
    printf("Unknown cable_type: %d\n", status.cable_type);
  else
    printf("%s\n", ssi_cables[status.cable_type]);
  }

void print_time_slots()
  {
  printf("TimeSlot [31-0]:\t0x%08X\n", status.time_slots);
  }

void print_scrambler()
  {
  printf("Scrambler:\t\t");
  if      (config.scrambler == CFG_SCRAM_OFF)
    printf("off\n");
  else if (config.scrambler == CFG_SCRAM_DL_KEN)
    printf("DigLink/Kentrox: X^43+1\n");
  else if (config.scrambler == CFG_SCRAM_LARS)
    printf("Larse: X^20+X^17+1 w/28ZS\n");
  else
    printf("Unknown scrambler: %d\n", config.scrambler);
  }

double vga_dbs(u_int8_t vga)
  {
  if  (vga <  0x0F)                   	   return  0.0;
  else if ((vga >= 0x0F) && (vga <= 0x1B)) return  0.0 + 0.77 * (vga - 0x0F);
  else if ((vga >= 0x1C) && (vga <= 0x33)) return 10.0 + 1.25 * (vga - 0x1C);
  else if ((vga >= 0x34) && (vga <= 0x39)) return 40.0 + 1.67 * (vga - 0x34);
  else if ((vga >= 0x3A) && (vga <= 0x3F)) return 50.0 + 2.80 * (vga - 0x3A);
  else /* if  (vga >  0x3F) */             return 64.0;
  }

void print_rx_gain_max()
  {
  if (config.rx_gain_max != CFG_GAIN_AUTO)
    {
    printf("Rx gain max:\t\t");
    printf("up to %02.1f dB\n", vga_dbs(config.rx_gain_max));
    }
  }

void print_tx_lbo()
  {
  u_int8_t saved_lbo = config.tx_lbo;

  printf("LBO = ");
  if (config.tx_lbo == CFG_LBO_AUTO)
    {
    config.tx_lbo = read_framer(Bt8370_TLIU_CR) & 0x30;
    printf("auto-set to ");
    }

  switch (config.tx_lbo)
    {
    case CFG_LBO_0DB:
      printf("0 dB\n");
      break;
    case CFG_LBO_7DB:
      printf("7.5 dB\n");
      break;
    case CFG_LBO_15DB:
      printf("15 dB\n");
      break;
    case CFG_LBO_22DB:
      printf("22.5 dB\n");
      break;
    default:
      printf("Unknown tx_lbo: %d\n", config.tx_lbo);
      break;
    }

  if (saved_lbo == CFG_LBO_AUTO)
    config.tx_lbo = saved_lbo;
  }

void print_tx_pulse(int skip_auto)
  {
  u_int8_t saved_pulse = config.tx_pulse;

  if ((config.tx_pulse == CFG_PULSE_AUTO) && skip_auto)
    return;

  printf("Tx pulse shape:\t\t");
  if (config.tx_pulse == CFG_PULSE_AUTO)
    {
    config.tx_pulse = read_framer(Bt8370_TLIU_CR) & 0x0E;
    printf("auto-set to ");
    }

  switch (config.tx_pulse)
    {
    case CFG_PULSE_T1DSX0:
      printf("T1-DSX: 0 to 40 meters\n");
      break;
    case CFG_PULSE_T1DSX1:
      printf("T1-DSX: 40 to 80 meters\n");
      break;
    case CFG_PULSE_T1DSX2:
      printf("T1-DSX: 80 to 120 meters\n");
      break;
    case CFG_PULSE_T1DSX3:
      printf("T1-DSX: 120 to 160 meters\n");
      break;
    case CFG_PULSE_T1DSX4:
      printf("T1-DSX: 160 to 200 meters\n");
      break;
    case CFG_PULSE_E1COAX:
      printf("E1: Twin Coax\n");
      break;
    case CFG_PULSE_E1TWIST:
      printf("E1: Twisted Pairs\n");
      break;
    case CFG_PULSE_T1CSU:
      printf("T1-CSU; ");
      print_tx_lbo();
      break;
    default:
      printf("Unknown tx_pulse: %d\n", config.tx_pulse);
      break;
    }

  if (saved_pulse == CFG_PULSE_AUTO)
    config.tx_pulse = saved_pulse;
  }

void print_ssi_sigs()
  {
  u_int32_t mii16 = status.snmp.ssi.sigs;
  const char *on = "On", *off = "Off";

  printf("Modem signals:\t\tDTR=%s DSR=%s RTS=%s CTS=%s\n",
   (mii16 & MII16_SSI_DTR) ? on : off,
   (mii16 & MII16_SSI_DSR) ? on : off,
   (mii16 & MII16_SSI_RTS) ? on : off,
   (mii16 & MII16_SSI_CTS) ? on : off);
  printf("Modem signals:\t\tDCD=%s RI=%s LL=%s RL=%s TM=%s\n",
   (mii16 & MII16_SSI_DCD) ? on : off,
   (mii16 & MII16_SSI_RI)  ? on : off,
   (mii16 & MII16_SSI_LL)  ? on : off,
   (mii16 & MII16_SSI_RL)  ? on : off,
   (mii16 & MII16_SSI_TM)  ? on : off);
  }

void print_hssi_sigs()
  {
  u_int32_t mii16 = status.snmp.hssi.sigs;
  const char *on = "On", *off = "Off";

  printf("Modem signals:\t\tTA=%s CA=%s\n",
   (mii16 & MII16_HSSI_TA) ? on : off,
   (mii16 & MII16_HSSI_CA) ? on : off);
  printf("Modem signals:\t\tLA=%s LB=%s LC=%s TM=%s\n",
   (mii16 & MII16_HSSI_LA) ? on : off,
   (mii16 & MII16_HSSI_LB) ? on : off,
   (mii16 & MII16_HSSI_LC) ? on : off,
   (mii16 & MII16_HSSI_TM) ? on : off);
  }

void print_events()
  {
  const char *timestr;
  struct timeval tv;
  time_t tv_sec;

  (void)gettimeofday(&tv, NULL);
  tv_sec = tv.tv_sec;
  timestr = ctime(&tv_sec);
  printf("Current time:\t\t%s", timestr);

  if (status.cntrs.reset_time.tv_sec < 1000)
    timestr = "Never\n";
  else
    {
    tv_sec = status.cntrs.reset_time.tv_sec;
    timestr = ctime(&tv_sec);
    }
  printf("Cntrs reset:\t\t%s", timestr);

  if (status.cntrs.ibytes)     printf("Rx bytes:\t\t%llu\n",   (unsigned long long)status.cntrs.ibytes);
  if (status.cntrs.obytes)     printf("Tx bytes:\t\t%llu\n",   (unsigned long long)status.cntrs.obytes);
  if (status.cntrs.ipackets)   printf("Rx packets:\t\t%llu\n", (unsigned long long)status.cntrs.ipackets);
  if (status.cntrs.opackets)   printf("Tx packets:\t\t%llu\n", (unsigned long long)status.cntrs.opackets);
  if (status.cntrs.ierrors)    printf("Rx errors:\t\t%u\n",    status.cntrs.ierrors);
  if (status.cntrs.oerrors)    printf("Tx errors:\t\t%u\n",    status.cntrs.oerrors);
  if (status.cntrs.idrops)     printf("Rx drops:\t\t%u\n",     status.cntrs.idrops);
  if (status.cntrs.missed)     printf("Rx missed:\t\t%u\n",    status.cntrs.missed);
  if (status.cntrs.odrops)     printf("Tx drops:\t\t%u\n",     status.cntrs.odrops);
  if (status.cntrs.fifo_over)  printf("Rx fifo overruns:\t%u\n", status.cntrs.fifo_over);
  if (status.cntrs.overruns)   printf("Rx overruns:\t\t%u\n",  status.cntrs.overruns);
  if (status.cntrs.fifo_under) printf("Tx fifo underruns:\t%u\n", status.cntrs.fifo_under);
  if (status.cntrs.underruns)  printf("Rx underruns:\t\t%u\n", status.cntrs.underruns);
  if (status.cntrs.fdl_pkts)   printf("Rx FDL pkts:\t\t%u\n",  status.cntrs.fdl_pkts);
  if (status.cntrs.crc_errs)   printf("Rx CRC:\t\t\t%u\n",     status.cntrs.crc_errs);
  if (status.cntrs.lcv_errs)   printf("Rx line code:\t\t%u\n", status.cntrs.lcv_errs);
  if (status.cntrs.frm_errs)   printf("Rx F-bits:\t\t%u\n",    status.cntrs.frm_errs);
  if (status.cntrs.febe_errs)  printf("Rx FEBE:\t\t%u\n",      status.cntrs.febe_errs);
  if (status.cntrs.par_errs)   printf("Rx P-parity:\t\t%u\n",  status.cntrs.par_errs);
  if (status.cntrs.cpar_errs)  printf("Rx C-parity:\t\t%u\n",  status.cntrs.cpar_errs);
  if (status.cntrs.mfrm_errs)  printf("Rx M-bits:\t\t%u\n",    status.cntrs.mfrm_errs);
  if (config.debug)
    { /* These events are hard to explain and may worry users, */
    if (status.cntrs.rxbuf)     printf("Rx no buffs:\t\t%u\n", status.cntrs.rxbuf);
    if (status.cntrs.txdma)     printf("Tx no descs:\t\t%u\n", status.cntrs.txdma);
    if (status.cntrs.lck_watch) printf("Lock watch:\t\t%u\n",  status.cntrs.lck_watch);
    if (status.cntrs.lck_intr)  printf("Lock intr:\t\t%u\n",   status.cntrs.lck_intr);
    if (status.cntrs.spare1)    printf("Spare1:\t\t\t%u\n",    status.cntrs.spare1);
    if (status.cntrs.spare2)    printf("Spare2:\t\t\t%u\n",    status.cntrs.spare2);
    if (status.cntrs.spare3)    printf("Spare3:\t\t\t%u\n",    status.cntrs.spare3);
    if (status.cntrs.spare4)    printf("Spare4:\t\t\t%u\n",    status.cntrs.spare4);
    }
  }

void print_summary()
  {
  switch(status.card_type)
    {
    case CSID_LMC_HSSI:
      {
      print_card_name();
      print_card_type();
      print_debug();
      print_status();
      print_tx_speed();
      print_line_prot();
      print_crc_len();
      print_loop_back(1);
      print_tx_clk_src();
      print_hssi_sigs();
      print_events();
      break;
      }
    case CSID_LMC_T3:
      {
      print_card_name();
      print_card_type();
      print_debug();
      print_status();
      print_tx_speed();
      print_line_prot();
      print_crc_len();
      print_loop_back(1);
      print_format();
      print_cable_len();
      print_scrambler();
      print_events();
      break;
      }
    case CSID_LMC_SSI:
      {
      print_card_name();
      print_card_type();
      print_debug();
      print_status();
      print_tx_speed();
      print_line_prot();
      print_crc_len();
      print_loop_back(1);
      print_dte_dce();
      print_synth_freq();
      print_cable_type();
      print_ssi_sigs();
      print_events();
      break;
      }
    case CSID_LMC_T1E1:
      {
      print_card_name();
      print_card_type();
      print_debug();
      print_status();
      print_tx_speed();
      print_line_prot();
      print_crc_len();
      print_loop_back(1);
      print_tx_clk_src();
      print_format();
      print_time_slots();
      print_cable_len();
      print_tx_pulse(1);
      print_rx_gain_max();
      print_events();
      break;
      }
    case CSID_LMC_HSSIc:
      {
      print_card_name();
      print_card_type();
      print_debug();
      print_status();
      print_line_prot();
      print_tx_speed();
      print_crc_len();
      print_loop_back(1);
      print_tx_clk_src();
      print_dte_dce();
      print_synth_freq();
      print_hssi_sigs();
      print_events();
      break;
      }
    default:
      {
      printf("%s: Unknown card type: %d\n", ifname, status.card_type);
      break;
      }
    }
  }

const char *print_t3_bop(int bop_code)
  {
  switch(bop_code)
    {
    case 0x00:
      return "far end LOF";
    case 0x0E:
      return "far end LOS";
    case 0x16:
      return "far end AIS";
    case 0x1A:
      return "far end IDL";
    case 0x07:
      return "Line Loopback activate";
    case 0x1C:
      return "Line Loopback deactivate";
    case 0x1B:
      return "Entire DS3 line";
    default:
      return "Unknown BOP code";
    }
  }

void print_t3_snmp()
  {
  printf("SNMP performance data:\n");
  printf(" LCV=%d",  status.snmp.t3.lcv);
  printf(" LOS=%d", (status.snmp.t3.line & TLINE_LOS)    ? 1 : 0);
  printf(" PCV=%d",  status.snmp.t3.pcv);
  printf(" CCV=%d",  status.snmp.t3.ccv);
  printf(" AIS=%d", (status.snmp.t3.line & TLINE_RX_AIS) ? 1 : 0);
  printf(" SEF=%d", (status.snmp.t3.line & T1LINE_SEF)   ? 1 : 0);
  printf(" OOF=%d", (status.snmp.t3.line & TLINE_LOF)    ? 1 : 0);
  printf(" FEBE=%d", status.snmp.t3.febe);
  printf(" RAI=%d", (status.snmp.t3.line & TLINE_RX_RAI) ? 1 : 0);
  printf("\n");
  }

void print_t3_dsu()
  {
  const char *no = "No", *yes = "Yes";
  u_int16_t mii16 = read_mii(16);
  u_int8_t ctl1   = read_framer(T3CSR_CTL1);
  u_int8_t ctl8   = read_framer(T3CSR_CTL8);
  u_int8_t stat9  = read_framer(T3CSR_STAT9);
  u_int8_t ctl12  = read_framer(T3CSR_CTL12);
  u_int8_t stat16 = read_framer(T3CSR_STAT16);

  printf("Framing:       \t\t%s\n", ctl1   & CTL1_M13MODE    ? "M13" : "CPAR");
  print_tx_speed();
  if ((mii16 & MII16_DS3_SCRAM)==0)
    printf("Scrambler:     \t\toff\n");
  else if (mii16 & MII16_DS3_POLY)
    printf("Scrambler:     \t\tX^20+X^17+1\n");
  else
    printf("Scrambler:     \t\tX^43+1\n");
  printf("Cable length   \t\t%s\n", mii16  & MII16_DS3_ZERO  ? "Short" : "Long");
  printf("Line    loop:  \t\t%s\n", mii16  & MII16_DS3_LNLBK ? yes : no);
  printf("Payload loop:  \t\t%s\n", ctl12  & CTL12_RTPLOOP   ? yes : no);
  printf("Frame   loop:  \t\t%s\n", ctl1   & CTL1_3LOOP      ? yes : no);
  printf("Host    loop:  \t\t%s\n", mii16  & MII16_DS3_TRLBK ? yes : no);
  printf("Transmit RAI:  \t\t%s\n", ctl1   & CTL1_XTX        ? no  : yes);
  printf("Receive  RAI:  \t\t%s\n", stat16 & STAT16_XERR     ? yes : no);
  printf("Transmit AIS:  \t\t%s\n", ctl1   & CTL1_TXAIS      ? yes : no);
  printf("Receive  AIS:  \t\t%s\n", stat16 & STAT16_RAIS     ? yes : no);
  printf("Transmit IDLE: \t\t%s\n", ctl1   & CTL1_TXIDL      ? yes : no);
  printf("Receive  IDLE: \t\t%s\n", stat16 & STAT16_RIDL     ? yes : no);
  printf("Transmit BLUE: \t\t%s\n", ctl8   & CTL8_TBLU       ? yes : no);
  printf("Receive  BLUE: \t\t%s\n", stat9  & STAT9_RBLU      ? yes : no);
  printf("Loss of Signal:\t\t%s\n", stat16 & STAT16_RLOS     ? yes : no);
  printf("Loss of Frame: \t\t%s\n", stat16 & STAT16_ROOF     ? yes : no);
  printf("Sev Err Frms:  \t\t%s\n", stat16 & STAT16_SEF      ? yes : no);
  printf("Code  errors:  \t\t%d\n", read_framer(T3CSR_CVLO) + (read_framer(T3CSR_CVHI)<<8));
  printf("C-Par errors:  \t\t%d\n", read_framer(T3CSR_CERR));
  printf("P-Par errors:  \t\t%d\n", read_framer(T3CSR_PERR));
  printf("F-Bit errors:  \t\t%d\n", read_framer(T3CSR_FERR));
  printf("M-Bit errors:  \t\t%d\n", read_framer(T3CSR_MERR));
  printf("FarEndBitErrs: \t\t%d\n", read_framer(T3CSR_FEBE));
  printf("Last Tx  FEAC msg:\t0x%02X (%s)\n",
   read_framer(T3CSR_TX_FEAC)  & 0x3F,
   print_t3_bop(read_framer(T3CSR_TX_FEAC) & 0x3F));
  printf("Last dbl FEAC msg:\t0x%02X (%s)\n",
   read_framer(T3CSR_DBL_FEAC) & 0x3F,
   print_t3_bop(read_framer(T3CSR_DBL_FEAC) & 0x3F));
  printf("Last Rx  FEAC msg:\t0x%02X (%s)\n",
   read_framer(T3CSR_RX_FEAC)  & 0x3F,
   print_t3_bop(read_framer(T3CSR_RX_FEAC) & 0x3F));
  print_t3_snmp();
  }

void t3_cmd(int argc, char **argv)
  {
  int ch;
  const char *optstring = "a:A:B:c:de:fF:lLsS:V:";

  while ((ch = getopt(argc, argv, optstring)) != -1)
    {
    switch (ch)
      {
      case 'a': /* stop alarms */
        {
        switch (optarg[0])
          {
          case 'a': /* Stop sending AIS Signal */
            {
            write_mii(16,
             read_mii(16) & ~MII16_DS3_FRAME);
            write_framer(T3CSR_CTL1,
             read_framer(T3CSR_CTL1) & ~CTL1_TXAIS);
            if (verbose) printf("Stop sending Alarm Indication Signal (AIS)\n");
            break;
            }
          case 'b': /* Stop sending Blue signal */
            {
            write_mii(16,
             read_mii(16) & ~MII16_DS3_FRAME);
            write_framer(T3CSR_CTL8,
             read_framer(T3CSR_CTL8) & ~CTL8_TBLU);
            if (verbose) printf("Stop sending Blue signal\n");
            break;
            }
          case 'i': /* Stop sending IDLE signal */
            {
            write_framer(T3CSR_CTL1,
             read_framer(T3CSR_CTL1) & ~CTL1_TXIDL);
            if (verbose) printf("Stop sending IDLE signal\n");
            break;
            }
          case 'y': /* Stop sending Yellow alarm */
            {
            write_framer(T3CSR_CTL1,
             read_framer(T3CSR_CTL1) | CTL1_XTX);
            if (verbose) printf("Stop sending Yellow alarm\n");
            break;
            }
          default:
            printf("Unknown alarm: %c\n", optarg[0]);
            break;
          }
        break;
        }
      case 'A': /* start alarms */
        {
        switch (optarg[0])
          {
          case 'a': /* Start sending AIS Signal */
            {
            write_mii(16,
             read_mii(16) | MII16_DS3_FRAME);
            write_framer(T3CSR_CTL1,
             read_framer(T3CSR_CTL1) | CTL1_TXAIS);
            if (verbose) printf("Sending AIS signal (framed 1010..)\n");
            break;
            }
          case 'b': /* Start sending Blue signal */
            {
            write_mii(16,
             read_mii(16) | MII16_DS3_FRAME);
            write_framer(T3CSR_CTL8,
             read_framer(T3CSR_CTL8) | CTL8_TBLU);
            if (verbose) printf("Sending Blue signal (unframed all 1s)\n");
            break;
            }
          case 'i': /* Start sending IDLE signal */
            {
            write_framer(T3CSR_CTL1,
             read_framer(T3CSR_CTL1) | CTL1_TXIDL);
            if (verbose) printf("Sending IDLE signal (framed 1100..)\n");
            break;
            }
          case 'y': /* Start sending Yellow alarm */
            {
            write_framer(T3CSR_CTL1,
             read_framer(T3CSR_CTL1) & ~CTL1_XTX);
            if (verbose) printf("Sending Yellow alarm (X-bits=0)\n");
            break;
            }
          default:
            printf("Unknown alarm: %c\n", optarg[0]);
            break;
          }
        break;
        }
      case 'B': /* send BOP msg */
        {
        u_int8_t bop = (u_int8_t)strtoul(optarg, NULL, 0);
        write_framer(T3CSR_TX_FEAC,  0xC0 + bop);
        if (verbose) printf("Sent '0x%02X' BOP msg 10 times\n", bop);
        break;
	}
      case 'c': /* set cable length */
        {
        config.cable_len = strtoul(optarg, NULL, 0);
        if (verbose) print_cable_len();
        update = 1;
        break;
        }
      case 'd': /* DSU status */
      case 's': /* deprecated */
        {
        print_t3_dsu();
        break;
        }
      case 'e': /* set framimg format */
        {
        config.format = strtoul(optarg, NULL, 0);
        if (verbose) print_format();
        update = 1;
        break;
        }
      case 'f': /* read and print framer regs */
        {
        int i;
        printf("TXC03401 regs:\n");
        printf("     0  1  2  3  4  5  6  7");
        for (i=0; i<21; i++)
          {
          if (i%8 == 0) printf("\n%02X: ", i);
          printf("%02X ", read_framer(i));
          }
        printf("\n\n");
        break;
        }
      case 'F': /* write framer reg */
        {
        u_int32_t addr = strtoul(optarg, NULL, 0);
        u_int32_t data = strtoul(argv[optind++], NULL, 0);
        write_framer(addr, data);
        if (verbose)
          {
          data = read_framer(addr);
          printf("Write framer register: addr = 0x%02X data = 0x%02X\n", addr, data);
	  }
        break;
        }
      case 'l': /* send DS3 line loopback deactivate BOP cmd */
        {
        ioctl_snmp_send(TSEND_RESET);
        if (verbose) printf("Sent 'DS3 Line Loopback deactivate' BOP cmd\n");
        break;
        }
      case 'L': /* send DS3 line loopback activate BOP cmd */
        {
        ioctl_snmp_send(TSEND_LINE);
        if (verbose) printf("Sent 'DS3 Line Loopback activate' BOP cmd\n");
        break;
        }
      case 'S': /* set scrambler */
        {
        config.scrambler = strtoul(optarg, NULL, 0);
        if (verbose) print_scrambler();
        update = 1;
        break;
        }
      case 'V': /* set T3 freq control DAC */
        {
        u_int32_t dac = strtoul(optarg, NULL, 0);
        write_dac(dac);
        if (verbose) printf("VCXO DAC value is %d\n", dac);
        break;
        }
      default:
        {
        printf("Unknown command char: %c\n", ch);
        exit(1);
        } /* case */
      } /* switch */
    } /* while */
  } /* proc */

const char *print_t1_bop(int bop_code)
  {
  switch(bop_code)
    {
    case 0x00:
      return "Yellow Alarm (far end LOF)";
    case 0x07:
      return "Line Loop up";
    case 0x1C:
      return "Line Loop down";
    case 0x0A:
      return "Payload Loop up";
    case 0x19:
      return "Payload Loop down";
    case 0x09:
      return "Network Loop up";
    case 0x12:
      return "Network Loop down";
    default:
      return "Unknown BOP code";
    }
  }

void print_t1_test_pattern(int patt)
  {
  printf("Test Pattern:\t\t");
  switch (patt)
    {
    case 0:
      printf("unframed X^11+X^9+1\n");
      break;
    case 1:
      printf("unframed X^15+X^14+1\n");
      break;
    case 2:
      printf("unframed X^20+X^17+1\n");
      break;
    case 3:
      printf("unframed X^23+X^18+1\n");
      break;
    case 4:
      printf("unframed X^11+X^9+1 w/7ZS\n");
      break;
    case 5:
      printf("unframed X^15+X^14+1 w/7ZS\n");
      break;
    case 6:
      printf("unframed X^20+X^17+1 w/14ZS (QRSS)\n");
      break;
    case 7:
      printf("unframed X^23+X^18+1 w/14ZS\n");
      break;
    case 8:
      printf("framed X^11+X^9+1\n");
      break;
    case 9:
      printf("framed X^15+X^14+1\n");
      break;
    case 10:
      printf("framed X^20+X^17+1\n");
      break;
    case 11:
      printf("framed X^23+X^18+1\n");
      break;
    case 12:;
      printf("framed X^11+X^9+1 w/7ZS\n");
      break;
    case 13:
      printf("framed X^15+X^14+1 w/7ZS\n");
      break;
    case 14:
      printf("framed X^20+X^17+1 w/14ZS (QRSS)\n");
      break;
    case 15:
      printf("framed X^23+X^18+1 w/14ZS\n");
      break;
    }
  }

void print_t1_far_report(int idx)
  {
  u_int16_t far = status.snmp.t1.prm[idx];

  printf(" SEQ=%d ", (far & T1PRM_SEQ)>>8);
  if      (far & T1PRM_G1) printf("CRC=1");
  else if (far & T1PRM_G2) printf("CRC=1 to 5");
  else if (far & T1PRM_G3) printf("CRC=5 to 10");
  else if (far & T1PRM_G4) printf("CRC=10 to 100");
  else if (far & T1PRM_G5) printf("CRC=100 to 319");
  else if (far & T1PRM_G6) printf("CRC>=320");
  else                     printf("CRC=0");
  printf(" SE=%d", (far & T1PRM_SE) ? 1 : 0);
  printf(" FE=%d", (far & T1PRM_FE) ? 1 : 0);
  printf(" LV=%d", (far & T1PRM_LV) ? 1 : 0);
  printf(" SL=%d", (far & T1PRM_SL) ? 1 : 0);
  printf(" LB=%d", (far & T1PRM_LB) ? 1 : 0);
  printf("\n");
  }

void print_t1_snmp()
  {
  printf("SNMP Near-end performance data:\n");
  printf(" LCV=%d",  status.snmp.t1.lcv);
  printf(" LOS=%d", (status.snmp.t1.line & TLINE_LOS)    ? 1 : 0);
  printf(" FE=%d",   status.snmp.t1.fe);
  printf(" CRC=%d",  status.snmp.t1.crc);
  printf(" AIS=%d", (status.snmp.t1.line & TLINE_RX_AIS) ? 1 : 0);
  printf(" SEF=%d", (status.snmp.t1.line & T1LINE_SEF)   ? 1 : 0);
  printf(" OOF=%d", (status.snmp.t1.line & TLINE_LOF)    ? 1 : 0);
  printf("  RAI=%d",(status.snmp.t1.line & TLINE_RX_RAI) ? 1 : 0);
  printf("\n");
  if (config.format == CFG_FORMAT_T1ESF)
    {
    printf("ANSI Far-end performance reports:\n");
    print_t1_far_report(0);
    print_t1_far_report(1);
    print_t1_far_report(2);
    print_t1_far_report(3);
    }
  }

void print_t1_dsu()
  {
  const char *no = "No", *yes = "Yes";
  u_int16_t mii16  = read_mii(16);
  u_int8_t isr0    = read_framer(Bt8370_ISR0);
  u_int8_t loop    = read_framer(Bt8370_LOOP);
  u_int8_t vga_max = read_framer(Bt8370_VGA_MAX) & 0x3F;
  u_int8_t alm1    = read_framer(Bt8370_ALM1);
  u_int8_t alm3    = read_framer(Bt8370_ALM3);
  u_int8_t talm    = read_framer(Bt8370_TALM);
  u_int8_t tpatt   = read_framer(Bt8370_TPATT);
  u_int8_t tpulse  = read_framer(Bt8370_TLIU_CR);
  u_int8_t vga;
  u_int8_t bop;
  u_int8_t saved_pulse, saved_lbo;

  /* d/c write required before read */
  write_framer(Bt8370_VGA, 0);
  vga = read_framer(Bt8370_VGA) & 0x3F;

  print_format();
  print_time_slots();
  print_tx_clk_src();
  print_tx_speed();

  saved_pulse     = config.tx_pulse;
  config.tx_pulse = tpulse & 0x0E;
  saved_lbo       = config.tx_lbo;
  config.tx_lbo   = tpulse & 0x30;
  print_tx_pulse(0);
  config.tx_pulse = saved_pulse;
  config.tx_lbo   = saved_lbo;

  printf("Tx outputs:    \t\t%sabled\n", (mii16 & MII16_T1_XOE) ? "En" : "Dis");
  printf("Line impedance:\t\t%s ohms\n", (mii16 & MII16_T1_Z) ? "120" : "100");
  printf("Max line loss: \t\t%4.1f dB\n", vga_dbs(vga_max));
  printf("Cur line loss: \t\t%4.1f dB\n", vga_dbs(vga));
  printf("Invert data:   \t\t%s\n", (mii16 & MII16_T1_INVERT) ? yes : no);
  printf("Line    loop:  \t\t%s\n", (loop & LOOP_LINE)    ? yes : no);
  printf("Payload loop:  \t\t%s\n", (loop & LOOP_PAYLOAD) ? yes : no);
  printf("Framer  loop:  \t\t%s\n", (loop & LOOP_FRAMER)  ? yes : no);
  printf("Analog  loop:  \t\t%s\n", (loop & LOOP_ANALOG)  ? yes : no);
  printf("Tx AIS:        \t\t%s\n", ((talm & TALM_TAIS) ||
   ((talm & TALM_AUTO_AIS) && (alm1 & ALM1_RLOS))) ? yes : no);
  printf("Rx AIS:        \t\t%s\n", (alm1 & ALM1_RAIS)  ? yes : no);
  if (((config.format & 1)==0) && (config.format != CFG_FORMAT_E1NONE))
    {
    printf("Tx RAI:        \t\t%s\n", ((talm & TALM_TYEL) ||
     ((talm & TALM_AUTO_YEL) && (alm3 & ALM3_FRED))) ? yes : no);
    printf("Rx RAI:        \t\t%s\n", (alm1 & ALM1_RYEL)  ? yes : no);
    }
  if (config.format == CFG_FORMAT_T1ESF)
    {
    printf("Tx BOP RAI:    \t\t%s\n", (alm1 & ALM1_RLOF)  ? yes : no);
    printf("Rx BOP RAI:    \t\t%s\n", (alm1 & ALM1_RMYEL) ? yes : no);
    }
  if ((config.format & 0x11) == 0x10) /* E1CAS */
    {
    printf("Rx TS16 AIS:   \t\t%s\n", (alm3 & ALM3_RMAIS) ? yes : no);
    printf("Tx TS16 RAI;   \t\t%s\n",
     ((talm & TALM_AUTO_MYEL) && (alm3 & ALM3_SRED)) ? yes : no);
    }
  printf("Rx LOS analog: \t\t%s\n", (alm1 & ALM1_RALOS) ? yes : no);
  printf("Rx LOS digital:\t\t%s\n", (alm1 & ALM1_RLOS)  ? yes : no);
  printf("Rx LOF:        \t\t%s\n", (alm1 & ALM1_RLOF)  ? yes : no);
  printf("Tx QRS:        \t\t%s\n", (tpatt & 0x10)      ? yes : no);
  printf("Rx QRS:        \t\t%s\n", (isr0 & 0x10)       ? yes : no);
  printf("LCV errors:    \t\t%d\n",
   read_framer(Bt8370_LCV_LO)  + (read_framer(Bt8370_LCV_HI)<<8));
  if (config.format != CFG_FORMAT_E1NONE)
    {
    if ((config.format & 1)==0) printf("Far End Block Errors:\t%d\n",
     read_framer(Bt8370_FEBE_LO) + (read_framer(Bt8370_FEBE_HI)<<8));
    printf("CRC errors:    \t\t%d\n",
     read_framer(Bt8370_CRC_LO)  + (read_framer(Bt8370_CRC_HI)<<8));
    printf("Frame errors:  \t\t%d\n",
     read_framer(Bt8370_FERR_LO) + (read_framer(Bt8370_FERR_HI)<<8));
    printf("Sev Err Frms:  \t\t%d\n", read_framer(Bt8370_AERR) & 0x03);
    printf("Change of Frm align:\t%d\n",  (read_framer(Bt8370_AERR) & 0x0C)>>2);
    printf("Loss of Frame events:\t%d\n", (read_framer(Bt8370_AERR) & 0xF0)>>4);
    }
  if (config.format == CFG_FORMAT_T1ESF)
    {
    if ((bop = read_framer(Bt8370_TBOP)))
      printf("Last Tx BOP msg:\t0x%02X (%s)\n", bop, print_t1_bop(bop));
    if ((bop = read_framer(Bt8370_RBOP)))
      printf("Last Rx BOP msg:\t0x%02X (%s)\n", bop, print_t1_bop(bop&0x3F));
    }
  print_t1_snmp();
  }

void t1_cmd(int argc, char **argv)
  {
  int ch;
  const char *optstring = "a:A:B:c:de:E:fF:g:iIlLpPstT:u:U:xX";

  while ((ch = getopt(argc, argv, optstring)) != -1)
    {
    switch (ch)
      {
      case 'a': /* stop alarms */
        {
        switch (optarg[0])
          {
          case 'y': /* Stop sending Yellow Alarm */
            {
            if ((config.format == CFG_FORMAT_T1SF) ||
                (config.format == CFG_FORMAT_E1NONE))
              printf("No Yellow alarm for this frame format\n");
            else if (config.format == CFG_FORMAT_T1ESF)
              write_framer(Bt8370_BOP,  0xE0); /* rbop 25, tbop off */
            else
              {
              u_int8_t talm = read_framer(Bt8370_TALM);
              write_framer(Bt8370_TALM, talm & ~TALM_TYEL);
	      }
            if (verbose) printf("Stop sending Yellow alarm\n");
            break;
            }
          case 'a': /* Stop sending AIS */
          case 'b': /* Stop sending Blue Alarm */
            {
            u_int8_t talm = read_framer(Bt8370_TALM);
            write_framer(Bt8370_TALM, talm & ~TALM_TAIS);
            if (verbose) printf("Stop sending AIS/Blue signal\n");
            break;
            }
          default:
            printf("Unknown alarm: %c\n", optarg[0]);
          }
        break;
        }
      case 'A': /* start alarms */
        {
        switch (optarg[0])
          {
          case 'y': /* Start sending Yellow Alarm */
            {
            if ((config.format == CFG_FORMAT_T1SF) ||
                (config.format == CFG_FORMAT_E1NONE))
              printf("No Yellow alarm for this frame format\n");
            else if (config.format == CFG_FORMAT_T1ESF)
              {
              write_framer(Bt8370_BOP,  0x0F); /* rbop off, tbop cont */
              write_framer(Bt8370_TBOP, T1BOP_OOF);
	      }
            else
              {
              u_int8_t talm = read_framer(Bt8370_TALM);
              write_framer(Bt8370_TALM, talm | TALM_TYEL);
	      }
            if (verbose) printf("Sending Yellow alarm\n");
            break;
            }
          case 'a': /* Start sending AIS */
          case 'b': /* Start sending Blue Alarm */
            {
            u_int8_t talm = read_framer(Bt8370_TALM);
            write_framer(Bt8370_TALM, talm | TALM_TAIS);
            if (verbose) printf("Sending AIS/Blue signal\n");
            break;
            }
          default:
            printf("Unknown alarm: %c\n", optarg[0]);
          }
        break;
        }
      case 'B': /* send BOP msg */
        {
        u_int8_t bop = (u_int8_t)strtoul(optarg, NULL, 0);
        if (config.format == CFG_FORMAT_T1ESF)
          {
          write_framer(Bt8370_BOP, 0x0B); /* rbop off, tbop 25 */
          write_framer(Bt8370_TBOP, bop); /* start sending BOP msg */
          sleep(1);  /* sending 25 BOP msgs takes about 100 ms. */
          write_framer(Bt8370_BOP, 0xE0); /* rbop 25, tbop off */
          if (verbose) printf("Sent '0x%02X' BOP msg 25 times\n", bop);
	  }
        else
          printf("BOP msgs only work in T1-ESF format\n");
        break;
	}
      case 'c': /* set cable length */
        {
        config.cable_len = strtoul(optarg, NULL, 0);
        if (verbose) print_cable_len();
        update = 1;
        break;
        }
      case 'd': /* DSU status */
      case 's': /* deprecated */
        {
        print_t1_dsu();
        break;
        }
      case 'e': /* set framimg format */
        {
        config.format = strtoul(optarg, NULL, 0);
        if (verbose) print_format();
        update = 1;
        break;
        }
      case 'E': /* set time slots */
        {
        config.time_slots = strtoul(optarg, NULL, 16);
        if (verbose) print_time_slots();
        update = 1;
        break;
        }
      case 'f': /* read and print framer regs */
        {
        int i;
        printf("Bt8370 regs:\n");
        printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
        for (i=0; i<512; i++)
          {
          if (i%16 == 0) printf("\n%03X: ", i);
          printf("%02X ", read_framer(i));
	  }
        printf("\n\n");
        break;
	}
      case 'F': /* write framer reg */
        {
        u_int32_t addr = strtoul(optarg, NULL, 0);
        u_int32_t data = strtoul(argv[optind++], NULL, 0);
        write_framer(addr, data);
        if (verbose)
          {
          data = read_framer(addr);
          printf("Write framer register: addr = 0x%02X data = 0x%02X\n", addr, data);
	  }
        break;
	}
      case 'g': /* set receiver gain */
        {
        config.rx_gain_max = strtoul(optarg, NULL, 0);
        if (verbose) print_rx_gain_max();
        update = 1;
        break;
        }
      case 'i': /* send CSU loopback deactivate inband cmd */
        {
        if (config.format == CFG_FORMAT_T1SF)
          {
          if (verbose) printf("Sending 'CSU loop down' inband cmd for 10 secs...");
          ioctl_snmp_send(TSEND_RESET);
          sleep(10);
          ioctl_snmp_send(TSEND_NORMAL);
          if (verbose) printf("done\n");
	  }
        else
          printf("Inband loopback cmds only work in T1-SF format");
        break;
        }
      case 'I': /* send CSU loopback activate inband cmd */
        {
        if (config.format == CFG_FORMAT_T1SF)
          {
          if (verbose) printf("Sending 'CSU loop up' inband cmd for 10 secs...");
          ioctl_snmp_send(TSEND_LINE);
          sleep(10);
          ioctl_snmp_send(TSEND_NORMAL);
          if (verbose) printf("done\n");
	  }
        else
          printf("Inband loopback cmds only work in T1-SF format");
        break;
        }
      case 'l': /* send line loopback deactivate BOP msg */
        {
        if (config.format == CFG_FORMAT_T1ESF)
          {
          ioctl_snmp_send(TSEND_RESET);
          if (verbose) printf("Sent 'Line Loop Down' BOP cmd\n");
	  }
        else
          printf("BOP msgs only work in T1-ESF format\n");
        break;
        }
      case 'L': /* send line loopback activate BOP msg */
        {
        if (config.format == CFG_FORMAT_T1ESF)
          {
          ioctl_snmp_send(TSEND_LINE);
          if (verbose) printf("Sent 'Line Loop Up' BOP cmd\n");
	  }
        else
          printf("BOP msgs only work in T1-ESF format\n");
        break;
        }
      case 'p': /* send payload loopback deactivate BOP msg */
        {
        if (config.format == CFG_FORMAT_T1ESF)
          {
          ioctl_snmp_send(TSEND_RESET);
          if (verbose) printf("Sent 'Payload Loop Down' BOP cmd\n");
	  }
        else
          printf("BOP msgs only work in T1-ESF format\n");
        break;
        }
      case 'P': /* send payload loopback activate BOP msg */
        {
        if (config.format == CFG_FORMAT_T1ESF)
          {
          ioctl_snmp_send(TSEND_PAYLOAD);
          if (verbose) printf("Sent 'Payload Loop Up' BOP cmd\n");
	  }
        else
          printf("BOP msgs only work in T1-ESF format\n");
        break;
        }
      case 't': /* stop sending test pattern */
        {
        ioctl_snmp_send(TSEND_NORMAL);
        if (verbose) printf("Stop sending test pattern\n");
        break;
        }
      case 'T': /* start sending test pattern */
        {
        u_int8_t patt = (u_int8_t)strtoul(optarg, NULL, 0);
        write_framer(Bt8370_TPATT, 0x10 + patt);
        write_framer(Bt8370_RPATT, 0x30 + patt);
        if (verbose) print_t1_test_pattern(patt);
        break;
        }
      case 'u': /* set transmit pulse shape */
        {
        config.tx_pulse = strtoul(optarg, NULL, 0);
        if (verbose) print_tx_pulse(0);
        update = 1;
        break;
        }
      case 'U': /* set tx line build-out */
        {
        if (config.tx_pulse == CFG_PULSE_T1CSU)
          {
          config.tx_lbo = strtoul(optarg, NULL, 0);
          if (verbose) print_tx_pulse(0);
          update = 1;
	  }
        else
          printf("LBO only meaningful if Tx Pulse is T1CSU\n");
        break;
        }
      case 'x': /* disable transmitter outputs */
        {
        write_mii(16, read_mii(16) & ~MII16_T1_XOE);
        if (verbose) printf("Transmitter outputs disabled\n");
        break;
	}
      case 'X': /* enable transmitter outputs */
        {
        write_mii(16, read_mii(16) |  MII16_T1_XOE);
        if (verbose) printf("Transmitter outputs enabled\n");
        break;
        }
      default:
        {
        printf("Unknown command char: %c\n", ch);
        exit(1);
        } /* case */
      } /* switch */
    } /* while */
  } /* proc */

/* used when reading Motorola S-Record format ROM files */
unsigned char read_hex(FILE *f)
  {
  unsigned char a, b, c;
  for (a=0, b=0; a<2; a++)
    {
    c = fgetc(f);
    c -= 48;
    if (c > 9) c -= 7;
    b = (b<<4) | (c & 0xF);
    }
  checksum += b;
  return b;
  }

void load_xilinx(char *name)
  {
  FILE *f;
  char *ucode;
  int c, i, length;

  if (verbose) printf("Load firmware from file %s...\n", name);
  if ((f = fopen(name, "r")) == 0)
    {
    perror("Failed to open file");
    exit(1);
    }

  ucode = (char *)malloc(8192); bzero(ucode, 8192);

  c = fgetc(f);
  if (c == 'X')
    { /* Xilinx raw bits file (foo.rbt) */
    /* skip seven lines of boiler plate */
    for (i=0; i<7;) if ((c=fgetc(f))=='\n') i++;
    /* build a dense bit array */
    i = length = 0;
    while ((c=fgetc(f))!=EOF)
      {  /* LSB first */
      if (c=='1') ucode[length] |= 1<<i++;
      if (c=='0') i++;
      if (i==8) { i=0; length++; }
      }
    }
  else if (c == 'S')
    { /* Motarola S records (foo.exo) */
    int blklen;
    length = 0;
    ungetc(c, f);
    while ((c = fgetc(f)) != EOF)
      {
      if (c != 'S')
        {
        printf("I'm confused; I expected an 'S'\n");
        exit(1);
        }
      c = fgetc(f);
      if (c == '9') break;
      else if (c == '1')
        {
        checksum = 0;
        blklen = read_hex(f) -3;
        read_hex(f); /* hi blkaddr */
        read_hex(f); /* lo blkaddr */
        for (i=0; i<blklen; i++)
          ucode[length++] = read_hex(f);
        read_hex(f); /* process but ignore checksum */
        if (checksum != 0xFF)
          {
          printf("File checksum error\n");
          exit(1);
          }
        c = fgetc(f); /* throw away eol */
        c = fgetc(f); /* throw away eol */
        }
      else
        {
        printf("I'm confused; I expected a '1' or a '9'\n");
        exit(1);
        }
      } /* while */
    } /* Motorola S-Record */
  else
    {
    printf("Unknown file type giving up\n");
    exit(1);
    }

  load_xilinx_from_file(ucode, length);
  }

/* 32-bit CRC calculated right-to-left over 8-bit bytes */
u_int32_t crc32(u_int8_t *bufp, int len)
  {
  int bit, i;
  u_int32_t data;
  u_int32_t crc  = 0xFFFFFFFFL;
  u_int32_t poly = 0xEDB88320L;

  for (i = 0; i < len; i++)
    for (data = *bufp++, bit = 0; bit < 8; bit++, data >>= 1)
      crc = (crc >> 1) ^ (((crc ^ data) & 1) ? poly : 0);

  return crc;
  }

/* 8-bit CRC calculated left-to-right over 16-bit words */
u_int8_t crc8(u_int16_t *bufp, int len)
  {
  int bit, i;
  u_int16_t data;
  u_int8_t crc  = 0xFF;
  u_int8_t poly = 0x07;

  for (i = 0; i < len; i++)
    for (data = *bufp++, bit = 15; bit >= 0; bit--)
      {
      if ((i==8) && (bit==7)) break;
      crc = (crc << 1) ^ ((((crc >> 7) ^ (data >> bit)) & 1) ? poly : 0);
      }
  return crc;
  }

void main_cmd(int argc, char **argv)
  {
  int ch;
  const char *optstring = "13a:bBcCdDeEf:gG:hi:L:mM:pP:sS:tT:uUvVw:x:X:yY?";

  while ((ch = getopt(argc, argv, optstring)) != -1)
    {
    switch (ch)
      {
      case '1': /* T1 commands */
        {
        if (verbose) printf("Doing T1 settings\n");
        if (status.card_type != CSID_LMC_T1E1)
          {
          printf("T1 settings only apply to T1E1 cards\n");
          exit(1);
          }
        t1_cmd(argc, argv);
        break;
        }
      case '3': /* T3 commands */
        {
        if (verbose) printf("Doing T3 settings\n");
        if (status.card_type != CSID_LMC_T3)
          {
          printf("T3 settings only apply to T3 cards\n");
          exit(1);
          }
        t3_cmd(argc, argv);
        break;
        }
      case 'a': /* clock source */
        {
        if ((status.card_type != CSID_LMC_T1E1) ||
            (status.card_type != CSID_LMC_HSSI) ||
            (status.card_type != CSID_LMC_HSSIc))
          {
          if (verbose) print_tx_clk_src();
          config.tx_clk_src = strtoul(optarg, NULL, 0);
          update = 1;
	  }
        else
          printf("txclksrc only applies to T1E1 and HSSI card types\n");
        break;
        }
      case 'b': /* read bios rom */
        {
        unsigned int i;
        printf("Bios ROM:\n");
        printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
        for (i=0; i<256; i++)
          {
          if (i%16 == 0) printf("\n%02X: ", i);
          printf("%02X ", read_bios_rom(i));
	  }
        printf("\n\n");
        break;
	}
      case 'B': /* write bios rom */
        {
        unsigned int i;
        for (i=0; i<256; i++) write_bios_rom(i, 255-i);
        if (verbose) printf("wrote (0..255) to bios rom addrs (0..255)\n");
        break;
	}
      case 'c': /* set crc_len = 16 */
        {
        config.crc_len = CFG_CRC_16;
        if (verbose) print_crc_len();
        update = 1;
        break;
        }
      case 'C': /* set crc_len = 32 */
        {
        config.crc_len = CFG_CRC_32;
        if (verbose) print_crc_len();
        update = 1;
        break;
        }
      case 'd': /* clear DEBUG flag */
        {
        config.debug = 0;
        if (verbose) printf("DEBUG flag cleared\n");
        update = 1;
        break;
	}
      case 'D': /* set DEBUG flag */
        {
        config.debug = 1;
        if (verbose) printf("DEBUG flag set\n");
        update = 1;
        break;
	}
      case 'e': /* set DTE (default) */
        {
        if ((status.card_type == CSID_LMC_SSI) ||
            (status.card_type == CSID_LMC_HSSIc))
          {
          config.dte_dce = CFG_DTE;
          if (verbose) print_dte_dce();
          update = 1;
	  }
        else
          printf("DTE cmd only applies to SSI & HSSIc cards\n");
        break;
	}
      case 'E': /* set DCE */
        {
        if ((status.card_type == CSID_LMC_SSI) ||
            (status.card_type == CSID_LMC_HSSIc))
          {
          config.dte_dce = CFG_DCE;
          if (verbose) print_dte_dce();
          update = 1;
	  }
        else
          printf("DCE cmd only applies to SSI & HSSIc cards\n");
        break;
	}
      case 'f': /* set synth osc freq */
        {
        if ((status.card_type == CSID_LMC_SSI) ||
            (status.card_type == CSID_LMC_HSSIc))
          {
          synth_freq(strtoul(optarg, NULL, 0));
          write_synth(config.synth);
          if (verbose) print_synth_freq();
	  }
        else
          printf("synth osc freq only applies to SSI & HSSIc cards\n");
        break;
        }
      case 'g': /* load gate array microcode from ROM */
        {
        load_xilinx_from_rom();
        if (verbose) printf("gate array configured from on-board ROM\n");
        break;
        }
      case 'G': /* load gate array microcode from file */
        {
        load_xilinx(optarg);
        if (verbose) printf("gate array configured from file %s\n", optarg);
        break;
        }
      case 'h': /* help */
      case '?':
        {
        usage();
        exit(0);
        /*NOTREACHED*/
        }
      case 'i': /* interface name */
        {
        /* already scanned this */
        break;
        }
      case 'L': /* set loopback modes */
        {
        config.loop_back = strtoul(optarg, NULL, 0);
        if (verbose) print_loop_back(0);
        update = 1;
        break;
	}
      case 'm': /* read and print MII regs */
        {
        int i;
        printf("MII regs:\n");
        printf("      0    1    2    3    4    5    6    7");
        for (i=0; i<32; i++)
          {
          u_int16_t mii = read_mii(i);
          if (i%8 == 0) printf("\n%02X: ", i);
          printf("%04X ", mii);
	  }
        printf("\n\n");
        break;
        }
      case 'M': /* write MII reg */
        {
        u_int32_t addr = strtoul(optarg, NULL, 0);
        u_int32_t data = strtoul(argv[optind++], NULL, 0);
        write_mii(addr, data);
        if (verbose)
          {
          data = read_mii(addr);
          printf("Write mii register: addr = 0x%02X data = 0x%04X\n", addr, data);
	  }
        break;
        }
      case 'p': /* read and print PCI config regs */
        {
        int i;
        printf("21140A PCI Config regs:\n");
        printf("       0        1        2        3");
        for (i=0; i<16; i++)
          {
          if (i%4 == 0) printf("\n%X: ", i);
          printf("%08X ", read_pci_config(i<<2));
	  }
        printf("\n\n");
        break;
	}
      case 'P': /* write PCI config reg */
        {
        u_int32_t addr = strtoul(optarg, NULL, 0);
        u_int32_t data = strtoul(argv[optind++], NULL, 0);
        write_pci_config(addr, data);
        if (verbose)
          {
          data = read_pci_config(addr);
          printf("Write PCI config reg: addr = 0x%02X data = 0x%08X\n", addr, data);
	  }
        break;
	}
      case 's': /* read and print Tulip SROM */
        {
        int i;
        printf("21140A SROM:\n");
        printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
        for (i=0; i<64; i++)
          {
          u_int16_t srom = read_srom(i);
          if (i%8 == 0) printf("\n%02X: ", i<<1);
          printf("%02X %02X ", srom & 0xFF, srom>>8);
	  }
        printf("\n\n");
        break;
	}
      case 'S': /* write Tulip SROM loc */
        {
        int i;
        u_int16_t srom[64];
        u_int32_t board = strtoul(optarg, NULL, 0);
        /* board: HSSI=3, DS3=4, SSI=5, T1E1=6, HSSIc=7 */

        for (i=0; i<64; i++) srom[i] = 0;
        srom[0]  = 0x1376; /* subsys vendor id */
        srom[1]  = board ? (u_int16_t)board : (read_mii(3)>>4 & 0xF) +1;
        /* Tulip hardware checks this checksum */
        srom[8]  = crc8(srom, 9);
        srom[10] = 0x6000; /* ethernet address */
        srom[11] = 0x0099; /* ethernet address */
        srom[12] = read_srom(12); /* 0x0000; */
        /* srom checksum is low 16 bits of Ethernet CRC-32 */
        srom[63] = (u_int16_t)~crc32((u_int8_t *)srom, 126);

#if 0 /* really write it */
        for (i=0; i<64; i++) write_srom(i, srom[i]);
#else /* print what would be written */
        printf("Caution! Recompile %s to enable this.\n", progname);
        printf("This is what would have been written:\n");
        printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
        for (i=0; i<64; i++)
          {
          if (i%8 == 0) printf("\n%02X: ", i<<1);
          printf("%02X %02X ", srom[i] & 0xFF, srom[i]>>8);
          }
        printf("\n\n");
#endif
        break;
	}
      case 't': /* read and print Tulip CSRs */
        {
        int i;
        printf("21140A CSRs:\n");
        printf("       0        1        2        3");
        for (i=0; i<16; i++)
          {
          if (i%4 == 0) printf("\n%X: ", i);
          printf("%08X ", read_csr(i));
	  }
        printf("\n\n");
        break;
	}
      case 'T': /* write Tulip CSR */
        {
        u_int32_t addr = strtoul(optarg, NULL, 0);
        u_int32_t data = strtoul(argv[optind++], NULL, 0);
        write_csr(addr, data);
        if (verbose)
          {
          data = read_csr(addr);
          printf("Write 21140A CSR: addr = 0x%02X data = 0x%08X\n", addr, data);
	  }
        break;
	}
      case 'u': /* reset event counters */
        {
        ioctl_reset_cntrs();
        if (verbose) printf("Event counters reset\n");
        break;
	}
      case 'U': /* reset gate array */
        {
        reset_xilinx();
        if (verbose) printf("gate array reset\n");
        break;
        }
      case 'v': /* set verbose mode */
        {
        verbose = 1;
        break;
        }
      case 'V': /* print card configuration */
        {
        summary = 1;
        break;
	}
      case 'w':
        {
        waittime = strtoul(optarg, NULL, 0);
        break;
	}
      case 'x': /* <number> set line protocol */
        {
        config.keep_alive = 1; /* required for LMI operation */
        config.proto = strtoul(optarg, NULL, 0);
        if (verbose) printf("line protocol set to %d\n", config.proto);
        update = 1;
        break;
	}
      case 'X': /* <number> set line package */
        {
        config.keep_alive = 1; /* required for LMI operation */
        config.stack = strtoul(optarg, NULL, 0);
        if (verbose) printf("line package set to %d\n", config.stack);
        update = 1;
        break;
	}
      case 'y': /* disable SPPP keep-alive packets */
        {
        if ((config.stack == STACK_SPPP) &&
            (config.proto == PROTO_FRM_RLY))
          printf("keep-alives must be ON for Frame-Relay/SPPP\n");
        else
          {
          config.keep_alive = 0;
          if (verbose) printf("SPPP keep-alive packets disabled\n");
          update = 1;
	  }
        break;
	}
      case 'Y': /* enable SPPP keep-alive packets */
        {
        config.keep_alive = 1;
        if (verbose) printf("SPPP keep-alive packets enabled\n");
        update = 1;
        break;
	}
      default:
        {
        printf("Unknown command char: %c\n", ch);
        exit(1);
	} /* case */
      } /* switch */
    } /* while */
  } /* proc */

int main(int argc, char **argv)
  {
  int i, error;

  progname = (char *)argv[0];

  /* 1) Read the interface name from the command line. */
#if __linux__
  ifname = (argc==1) ? "hdlc0" : (char *)argv[1];
#else
  ifname = (argc==1) ? DEVICE_NAME"0" : (char *)argv[1];
#endif

  /* 2) Open the device; decide if netgraph is being used, */
  /* use netgraph if ifname ends with ":" */
  for (i=0; ifname[i] != 0; i++) continue;

  /* Get a socket type file descriptor. */
#if defined(NETGRAPH)
  if ((netgraph = (ifname[i-1] == ':')))
    error = NgMkSockNode(NULL, &fdcs, NULL);
  else
#endif
    error = fdcs = socket(AF_INET, SOCK_DGRAM, 0);
  if (error < 0)
    {
    fprintf(stderr, "%s: %s() failed: %s\n", progname,
     netgraph? "NgMkSockNode" : "socket", strerror(errno));
    exit(1);
    }

  /* 3) Read the current interface configuration from the driver. */
  ioctl_read_config();
  ioctl_read_status();

  summary = (argc <= 2);  /* print summary at end */
  update  = 0;	/* write to card at end */

  /* 4) Read the command line args and carry out their actions. */
  optind = 2;
  if (argc > 2) main_cmd(argc, argv);

  if (summary) print_summary();

  /* 5) Write the modified interface configuration to the driver. */
  if (update) ioctl_write_config();

  while (waittime)
    {
    struct status old;

    ioctl_read_status();
    old = status;
    sleep(waittime);
    ioctl_read_status();

    status.cntrs.ibytes    -= old.cntrs.ibytes;
    status.cntrs.obytes    -= old.cntrs.obytes;
    status.cntrs.ipackets  -= old.cntrs.ipackets;
    status.cntrs.opackets  -= old.cntrs.opackets;
    status.cntrs.ierrors   -= old.cntrs.ierrors;
    status.cntrs.oerrors   -= old.cntrs.oerrors;
    status.cntrs.idrops    -= old.cntrs.idrops;
    status.cntrs.missed    -= old.cntrs.missed;
    status.cntrs.odrops    -= old.cntrs.odrops;
    status.cntrs.fifo_over -= old.cntrs.fifo_over;
    status.cntrs.overruns  -= old.cntrs.overruns;
    status.cntrs.fifo_under-= old.cntrs.fifo_under;
    status.cntrs.underruns -= old.cntrs.underruns;
    status.cntrs.crc_errs  -= old.cntrs.crc_errs;
    status.cntrs.lcv_errs  -= old.cntrs.lcv_errs;
    status.cntrs.frm_errs  -= old.cntrs.frm_errs;
    status.cntrs.febe_errs -= old.cntrs.febe_errs;
    status.cntrs.par_errs  -= old.cntrs.par_errs;
    status.cntrs.cpar_errs -= old.cntrs.cpar_errs;
    status.cntrs.mfrm_errs -= old.cntrs.mfrm_errs;
    status.cntrs.rxbuf     -= old.cntrs.rxbuf;
    status.cntrs.txdma     -= old.cntrs.txdma;
    status.cntrs.lck_watch -= old.cntrs.lck_watch;
    status.cntrs.lck_intr  -= old.cntrs.lck_intr;
    status.cntrs.spare1    -= old.cntrs.spare1;
    status.cntrs.spare2    -= old.cntrs.spare2;
    status.cntrs.spare3    -= old.cntrs.spare3;
    status.cntrs.spare4    -= old.cntrs.spare4;

    putchar('\n');

    print_summary();
    }

  exit(0);
  /* NOTREACHED */
  }
