/*	$NetBSD: aica_arm.c,v 1.4.92.1 2009/05/13 17:16:37 jym Exp $	*/

/*
 * Copyright (c) 2003 SHIMIZU Ryo <ryo@misakimix.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

typedef	unsigned char	uint8_t;
typedef	unsigned short	uint16_t;
typedef	unsigned long	uint32_t;

#include <arch/dreamcast/dev/g2/aicavar.h>

#define	DC_REG_ADDR	0x00800000

#define	REG_READ_1(off)		\
	(*(volatile uint8_t *)(DC_REG_ADDR + (off)))
#define	REG_READ_2(off)		\
	(*(volatile uint16_t *)(DC_REG_ADDR + (off)))
#define	REG_READ_4(off)		\
	(*(volatile uint32_t *)(DC_REG_ADDR + (off)))
#define	REG_WRITE_1(off,val)	\
	((*(volatile uint8_t *)(DC_REG_ADDR + (off))) = (val))
#define	REG_WRITE_2(off,val)	\
	((*(volatile uint16_t *)(DC_REG_ADDR + (off))) = (val))
#define	REG_WRITE_4(off,val)	\
	((*(volatile uint32_t *)((DC_REG_ADDR)+(off))) = (val))

#define	CH_READ_1(ch,off)	REG_READ_1(((ch) << 7) + (off))
#define	CH_READ_2(ch,off)	REG_READ_2(((ch) << 7) + (off))
#define	CH_READ_4(ch,off)	REG_READ_4(((ch) << 7) + (off))
#define	CH_WRITE_1(ch,off,val)	REG_WRITE_1(((ch) << 7) + (off), val)
#define	CH_WRITE_2(ch,off,val)	REG_WRITE_2(((ch) << 7) + (off), val)
#define	CH_WRITE_4(ch,off,val)	REG_WRITE_4(((ch) << 7) + (off), val)

void aica_init(void);
inline int in_first_half(unsigned int);
inline int in_second_half(unsigned int);
void bzero_4(void *, unsigned int);
void bzero(void *, unsigned int);
uint32_t rate2reg(unsigned int);
void aica_stop(void);
void aica_main(void);

void
aica_init(void)
{
	int ch, off;

	/* Initialize AICA channels */
	REG_WRITE_4(0x2800, 0x0000);	/* Master volume: Min */

	for (ch = 0; ch < 64; ch++) {
		CH_WRITE_4(ch, 0x00, 0x8000);	/* Key off */
		CH_WRITE_4(ch, 0x04, 0x0000);	/* DataAddress (low) */
		CH_WRITE_4(ch, 0x08, 0x0000);	/* LoopStartPosition */
		CH_WRITE_4(ch, 0x0c, 0x0000);	/* LoopEndPosition */
		CH_WRITE_4(ch, 0x10, 0x001f);	/* AR = 0x1f = 0 msec */
		CH_WRITE_4(ch, 0x14, 0x001f);	/* RR = 0x1f = 0 msec */
		CH_WRITE_4(ch, 0x18, 0x0000);	/* Pitch */
		CH_WRITE_4(ch, 0x1c, 0x0000);	/* LFO Control */
		CH_WRITE_4(ch, 0x20, 0x0000);	/* DSP Channel to send */
		CH_WRITE_4(ch, 0x24, 0x0000);	/* Pan & Volume */
		CH_WRITE_4(ch, 0x28, 0x0024);	/* Volume & LowPassFilter */
		CH_WRITE_4(ch, 0x2c, 0x0000);	/* LowPassFilter for Attack  */
		CH_WRITE_4(ch, 0x30, 0x0000);	/* LowPassFilter for Decay   */
		CH_WRITE_4(ch, 0x34, 0x0000);	/* LowPassFilter for Sustain */
		CH_WRITE_4(ch, 0x38, 0x0000);	/* LowPassFilter for Keyoff  */
		CH_WRITE_4(ch, 0x3c, 0x0000);	/* LowPassFilter for Release */
		CH_WRITE_4(ch, 0x40, 0x0000);	/* LowPassFilter transition
						   for Attack, Decay */
		CH_WRITE_4(ch, 0x44, 0x0000);	/* LowPassFilter transition
						   for Decay, Release */

		for (off = 0x48; off < 0x80; off+=4) {
			CH_WRITE_4(ch, off, 0x0000);	/* other = 0 */
		}
	}

	REG_WRITE_4(0x2800, 0x000f);	/* Master volume: Max */
}


inline int
in_first_half(unsigned int loophalf)
{

	REG_WRITE_1(0x280d, 0);	/* select channel 0 */
	return REG_READ_4(0x2814) < loophalf;
}

inline int
in_second_half(unsigned int loophalf)
{

	REG_WRITE_1(0x280d, 0);	/* select channel 0 */
	return REG_READ_4(0x2814) >= loophalf;
}


void
bzero_4(void *b, unsigned int len)
{
	uint32_t *p;

	p = b;
	len = (len + 3) & ~3;
	for (; len != 0; len -= 4)
		*p++ = 0;
}

void
bzero(void *b,unsigned int len)
{
	uint8_t *p;

	p = b;
	for (; len != 0; len--)
		*p++ = 0;
}


uint32_t
rate2reg(unsigned int rate)
{
	uint32_t base, fns;
	int oct;

	base = 44100 << 7;
	for (oct = 7; oct >= -8 && rate < base; oct--)
		base >>= 1;

	if (rate < base)
		return (oct << 11) & 0xf800;

	rate -= base;

#if 0
	/* (base / 2) : round off */
	fns = (rate * 1024 + (base / 2)) / base;
#else
	/* avoid using udivsi3() */
	{
		uint32_t tmp = (rate * 1024 + (base / 2));
		for (fns = 0; tmp >= base; tmp -= base, fns++)
			;
	}
#endif

	/* adjustment */
	if (fns == 1024) {
		oct++;
		fns = 0;
	} else {
		if ((rate > base * fns / 1024) &&
		    (fns < 1023) &&
		    (rate == base * (fns + 1) / 1024)) {
			fns++;
		} else if ((rate < base * fns / 1024) &&
		           (fns > 0) &&
		           (rate == base * (fns - 1)/ 1024)) {
			fns--;
		}
	}

	return ((oct << 11) & 0xf800) + fns;
}



void
aica_stop(void)
{

	CH_WRITE_4(0, 0x00, 0x8000);
	CH_WRITE_4(1, 0x00, 0x8000);
	bzero_4((void *)AICA_DMABUF_LEFT, AICA_DMABUF_SIZE);
	bzero_4((void *)AICA_DMABUF_RIGHT, AICA_DMABUF_SIZE);
}

void
aica_main(void)
{
	volatile aica_cmd_t *aicacmd = (volatile aica_cmd_t *)AICA_ARM_CMD;
	int play_state;
	unsigned int loopend = 0,loophalf = 0;
	unsigned int blksize = 0, ratepitch;
	uint32_t cmd, serial;

	aica_init();

	REG_WRITE_4(0x28b4, 0x0020);	/* INT Enable to SH4 */

	bzero_4((void *)AICA_DMABUF_LEFT, AICA_DMABUF_SIZE);
	bzero_4((void *)AICA_DMABUF_RIGHT, AICA_DMABUF_SIZE);

	play_state = 0;
	serial = aicacmd->serial = 0;

	for (;;) {
		if (serial != aicacmd->serial) {
			serial = aicacmd->serial;
			cmd = aicacmd->command;
			aicacmd->command = AICA_COMMAND_NOP;
		} else {
			cmd = AICA_COMMAND_NOP;
		}

		switch (cmd) {
		case AICA_COMMAND_NOP:
			/*
			 * AICA_COMMAND_NOP - Idle process
			 */
			switch (play_state) {
			case 0: /* not playing */
				break;
			case 1: /* first half */
				if (in_second_half(loophalf)) {
					/* Send INT to SH4 */
					REG_WRITE_4(0x28b8, 0x0020);
					play_state = 2;
				}
				break;
			case 2: /* second half */
				if (in_first_half(loophalf)) {
					/* Send INT to SH4 */
					REG_WRITE_4(0x28b8, 0x0020);
					play_state = 1;
				}
				break;
			case 3:
				if (in_second_half(loophalf)) {
					aica_stop();
					play_state = 0;
				}
				break;
			case 4:
				if (in_first_half(loophalf)) {
					aica_stop();
					play_state = 0;
				}
				break;
			}
			break;

		case AICA_COMMAND_PLAY:
			blksize = aicacmd->blocksize;

			CH_WRITE_4(0, 0x00, 0x8000);
			CH_WRITE_4(1, 0x00, 0x8000);

			switch (aicacmd->precision) {
			case 16:
				loopend = blksize;
				break;
			case 8:
				loopend = blksize * 2;
				break;
			case 4:
				loopend = blksize * 4;
				break;
			}
			loophalf = loopend / 2;

			ratepitch = rate2reg(aicacmd->rate);

			/* setup left */
			CH_WRITE_4(0, 0x08, 0);		/* loop start */
			CH_WRITE_4(0, 0x0c, loopend);	/* loop end */
			CH_WRITE_4(0, 0x18, ratepitch);	/* SamplingRate */
			CH_WRITE_4(0, 0x24, 0x0f1f);	/* volume MAX,
							   right PAN */

			/* setup right */
			CH_WRITE_4(1, 0x08,0);		/* loop start */
			CH_WRITE_4(1, 0x0c, loopend);	/* loop end */
			CH_WRITE_4(1, 0x18, ratepitch);	/* SamplingRate */
			CH_WRITE_4(1, 0x24, 0x0f0f);	/* volume MAX,
							   right PAN */

			{
				uint32_t mode, lparam, rparam;

				if (aicacmd->precision == 4)
					mode = 3 << 7;	/* 4bit ADPCM */
				else if (aicacmd->precision == 8)
					mode = 1 << 7;	/* 8bit */
				else
					mode = 0;	/* 16bit */

				switch (aicacmd->channel) {
				case 2:
					CH_WRITE_4(0, 0x04,
					    AICA_DMABUF_LEFT & 0xffff);
					CH_WRITE_4(1, 0x04,
					    AICA_DMABUF_RIGHT & 0xffff);
					lparam = 0xc000 /*PLAY*/ |
					    0x0200 /*LOOP*/ | mode |
					    (AICA_DMABUF_LEFT >> 16);
					rparam = 0xc000 /*PLAY*/ |
					    0x0200 /*LOOP*/ | mode |
					    (AICA_DMABUF_RIGHT >> 16);
					CH_WRITE_4(0, 0x00, lparam);
					CH_WRITE_4(1, 0x00, rparam);
					break;
				case 1:
					CH_WRITE_1(0, 0x24, 0);	/* middle
								   balance */
					CH_WRITE_4(0, 0x04,
					    AICA_DMABUF_LEFT & 0xffff);
					CH_WRITE_4(0, 0x00, 0xc000 /*PLAY*/ |
					    0x0200 /*LOOP*/ | mode |
					    (AICA_DMABUF_LEFT >> 16));
					break;
				}
			}
			play_state = 1;
			break;

		case AICA_COMMAND_STOP:
			switch (play_state) {
			case 1:
				bzero_4((void *)(AICA_DMABUF_LEFT + blksize),
				    blksize);
				bzero_4((void *)(AICA_DMABUF_RIGHT + blksize),
				    blksize);
				play_state = 3;
				break;
			case 2:
				bzero_4((void *)AICA_DMABUF_LEFT, blksize);
				bzero_4((void *)AICA_DMABUF_RIGHT, blksize);
				play_state = 4;
				break;
			default:
				aica_stop();
				play_state = 0;
				break;
			}
			break;

		case AICA_COMMAND_INIT:
			aica_stop();
			play_state = 0;
			break;

		case AICA_COMMAND_MVOL:
			REG_WRITE_4(0x2800, L256TO16(aicacmd->l_param));
			break;

		case AICA_COMMAND_VOL:
			CH_WRITE_1(0, 0x29, 0xff - (aicacmd->l_param & 0xff));
			CH_WRITE_1(1, 0x29, 0xff - (aicacmd->r_param & 0xff));
			break;

		}
	}
}
