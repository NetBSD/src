/*	$NetBSD: ate_subr.c,v 1.1.2.2 2001/04/09 01:56:09 nathanw Exp $	*/

/*
 * All Rights Reserved, Copyright (C) Fujitsu Limited 1995
 *
 * This software may be used, modified, copied, distributed, and sold, in
 * both source and binary form provided that the above copyright, these
 * terms and the following disclaimer are retained.  The name of the author
 * and/or the contributor may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND THE CONTRIBUTOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION.
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Portions copyright (C) 1993, David Greenman.  This software may be used,
 * modified, copied, distributed, and sold, in both source and binary form
 * provided that the above copyright and these terms are retained.  Under no
 * circumstances is the author responsible for the proper functioning of this
 * software, nor does the author assume any responsibility for damages
 * incurred with its use.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/syslog.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/mb86960reg.h>
#include <dev/ic/ate_subr.h>

#include <dev/isa/if_fereg.h>	/* XXX */

static __inline__ void ate_strobe __P((bus_space_tag_t, bus_space_handle_t));

/*
 * Routines to read all bytes from the config EEPROM through MB86965A.
 * I'm not sure what exactly I'm doing here...  I was told just to follow
 * the steps, and it worked.  Could someone tell me why the following
 * code works?  (Or, why all similar codes I tried previously doesn't
 * work.)  FIXME.
 */

static __inline__ void
ate_strobe (iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{

	/*
	 * Output same value twice.  To speed-down execution?
	 */
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT);
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT);
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT | FE_B16_CLOCK);
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT | FE_B16_CLOCK);
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT);
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT);
}

void
ate_read_eeprom(iot, ioh, data)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_char *data;
{
	int n, count;
	u_char val, bit;

	/* Read bytes from EEPROM; two bytes per an iteration. */
	for (n = 0; n < FE_EEPROM_SIZE / 2; n++) {
		/* Reset the EEPROM interface. */
		bus_space_write_1(iot, ioh, FE_BMPR16, 0x00);
		bus_space_write_1(iot, ioh, FE_BMPR17, 0x00);
		bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT);

		/* Start EEPROM access. */
		bus_space_write_1(iot, ioh, FE_BMPR17, FE_B17_DATA);
		ate_strobe(iot, ioh);

		/* Pass the iteration count to the chip. */
		count = 0x80 | n;
		for (bit = 0x80; bit != 0x00; bit >>= 1) {
			bus_space_write_1(iot, ioh, FE_BMPR17,
			    (count & bit) ? FE_B17_DATA : 0);
			ate_strobe(iot, ioh);
		}
		bus_space_write_1(iot, ioh, FE_BMPR17, 0x00);

		/* Read a byte. */
		val = 0;
		for (bit = 0x80; bit != 0x00; bit >>= 1) {
			ate_strobe(iot, ioh);
			if (bus_space_read_1(iot, ioh, FE_BMPR17) &
			    FE_B17_DATA)
				val |= bit;
		}
		*data++ = val;

		/* Read one more byte. */
		val = 0;
		for (bit = 0x80; bit != 0x00; bit >>= 1) {
			ate_strobe(iot, ioh);
			if (bus_space_read_1(iot, ioh, FE_BMPR17) &
			    FE_B17_DATA)
				val |= bit;
		}
		*data++ = val;
	}

	/* Make sure the EEPROM is turned off. */
	bus_space_write_1(iot, ioh, FE_BMPR16, 0);
	bus_space_write_1(iot, ioh, FE_BMPR17, 0);

#if ATE_DEBUG >= 3
	/* Report what we got. */
	data -= FE_EEPROM_SIZE;
	log(LOG_INFO, "ate_read_eeprom: EEPROM at %04x:"
	    " %02x%02x%02x%02x %02x%02x%02x%02x -"
	    " %02x%02x%02x%02x %02x%02x%02x%02x -"
	    " %02x%02x%02x%02x %02x%02x%02x%02x -"
	    " %02x%02x%02x%02x %02x%02x%02x%02x\n",
	    (int) ioh,		/* XXX */
	    data[ 0], data[ 1], data[ 2], data[ 3],
	    data[ 4], data[ 5], data[ 6], data[ 7],
	    data[ 8], data[ 9], data[10], data[11],
	    data[12], data[13], data[14], data[15],
	    data[16], data[17], data[18], data[19],
	    data[20], data[21], data[22], data[23],
	    data[24], data[25], data[26], data[27],
	    data[28], data[29], data[30], data[31]);
#endif
}
