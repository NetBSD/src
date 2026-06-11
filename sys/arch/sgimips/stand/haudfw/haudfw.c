/*	$NetBSD: haudfw.c,v 1.1 2026/06/11 08:16:44 rumble Exp $	*/

/*
 * Copyright (c) 2025 Stephen M. Rumble <rumble@ephemeral.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This program facilitates loading the SGI Indigo's "Hollywood Audio" Motorola
 * 56K DSP firmware by translating a loadable text file into a binary blob.
 *
 * The input is a .lod text file and the output is a 128KiB binary that the
 * sgimips/haud(4) driver can copy directly into the DSP's SRAM at byte offset
 * zero.
 *
 * The DSP's memory space is provided by three 8-bit 32KiB SRAMs, which are
 * combined to provide 32768 24-bit DSP words. On the CPU side, this memory is
 * mapped onto 32768 contiguous 32-bit words via the HPC chip.
 *
 * The DSP's X, Y, and P address spaces map on to different subsets of the SRAM
 * memory space. The .lod file specifies where words are loaded into each DSP
 * address space. These addresses need to be translated into offsets into the
 * raw SRAM mapped into the CPU's address space in order to load code and data
 * in the correct locations.
 *
 * Mappings between the DSP addresses and CPU/SRAM addresses were determined by
 * using a test .lod file and IRIX's /usr/etc/audio/dsploader. Dumps of the SRAM
 * showed where the utility wrote test various patterns defined in the test
 * file's fabricated _DATA entries. The results are as follows below and appear
 * to indicate that the DSP is configured in mode 2 ("normal expanded mode"),
 * with 0xe000 as the post-reset start address. The ranges are inclusive.
 *
 *             Section      DSP Word Addr        CPU Word Addr
 *                P        0xd000 - 0xefff      0x1000 - 0x2fff
 *                X        0xb000 - 0xbfff      0x7000 - 0x7fff
 *                X        0xc000 - 0xcfff      0x0000 - 0x0fff
 *                Y        0xb000 - 0xefff      0x3000 - 0x6fff
 *
 * Note that the above CPU addresses are 32-bit word offsets from the HPC's
 * SRAM base address. So multiply by 4 to get the CPU byte address offset.
 */

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Dump a C array representing the data to load into SRAM to stdout.
bool c_output = false;

// Larger values > 0 increase debugging output to stderr.
int verbosity = 0;

const char whitespace[] = " \t\r\n";

static void
debug_printf(int level, const char *format, ...)
{
	if (verbosity >= level) {
		va_list ap;
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	}
}

static void
check(bool constraint, const char *format, ...)
{
	if (!constraint) {
		va_list ap;
		va_start(ap, format);
		fprintf(stderr, "error: ");
		verrx(1, format, ap);
		va_end(ap);
	}
}

__attribute__((noreturn))
static void 
fail(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	check(false, format, ap);
	// Never hit. exit() squelches warning.
	va_end(ap);
	exit(1);
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-cv] input.lod output.bin\n",
	    getprogname());
	exit(1);
}

static int
dsp_to_cpu_address(char section, int dsp_addr)
{
	switch (section) {
	case 'P':
		check(0xd000 <= dsp_addr && dsp_addr <= 0xefff,
		      "P section dsp_addr 0x%04x not in [0xd000,0xefff]",
		      dsp_addr);
		return dsp_addr - 0xc000;
	case 'X':
		if (0xb000 <= dsp_addr && dsp_addr <= 0xbfff) {
			return dsp_addr - 0x4000;
		} else if (0xc000 <= dsp_addr && dsp_addr <= 0xcfff) {
			return dsp_addr - 0xc000;
		} else {
			fail("X section dsp_addr 0x%04x not in either "
			    "[0xb000,0xbfff] or [0xc000,0xcfff]", dsp_addr);
		}
	case 'Y':
		check(0xb000 <= dsp_addr && dsp_addr <= 0xefff,
		      "Y section dsp_addr 0x%04x not in [0xb000,0xefff)",
		      dsp_addr);
		return dsp_addr - 0x8000;
	default:
		fail("unknown section type '%c'", section);
	}
}

static void
parse_data_line(const char *line, char *section, int *dsp_data_offset,
    int* section_data_count) {
	char *line_copy = strdup(line);
	char *field = strtok(line_copy, whitespace);
	for (int i = 0; field != NULL; i++, field = strtok(NULL, whitespace)) {
		switch (i) {
		case 0:
			check(strcmp(field, "_DATA") == 0,
			    "unexpected _DATA header: \"%s\"", field);
			break;
		case 1:
			*section = field[0];
			check(strlen(field) == 1 && strspn(field, "XYPL") == 1,
			    "unexpected section type: \"%s\"", field);
			break;
		case 2:
			*dsp_data_offset = strtol(field, NULL, 16);
			break;
		default:
			fail("too many fields in _DATA line: \"%s\", "
			   "expected 3", line);
		}
	}
	*section_data_count = 0;
	free(line_copy);
}

int
main(int argc, char **argv)
{
	int ch;
	while ((ch = getopt(argc, argv, "cv")) != -1) {
		switch (ch) {
		case 'c':
			c_output = true;
			break;
		case 'v':
			verbosity++;
			break;
		case '?':
		default:
			usage();
		}
	}
	argv += optind;
	argc -= optind;
	if (argc != 2) {
		usage();
	}

	check(strcmp(argv[0], argv[1]) != 0,
	    "input and output files must be different");

	const int firmware_words = 32768;
	u_int32_t *firmware = malloc(firmware_words * sizeof(u_int32_t));
	check(firmware != NULL, "unable to malloc buffer for firmware: %s",
	    strerror(errno));
	memset(firmware, 0, firmware_words * sizeof(u_int32_t));

	FILE *fp = fopen(argv[0], "r");
	check(fp != NULL, "unable to open input file %s: %s",
	    argv[0], strerror(errno));

	int dsp_data_offset = 0;
	int section_data_count = 0;
	char section = '\0';
	char line[100];
	for (int lineno = 1; fgets(line, sizeof(line), fp) != NULL; lineno++) {
		if (strspn(line, whitespace) == strlen(line)) {
			// skip blank lines
		} else if (strncmp(line, "_START", 6) == 0) {
			debug_printf(1, "_START on line %d\n", lineno);
		} else if (strncmp(line, "_END", 4) == 0) {
			if (section != '\0') {
				debug_printf(1, "%c section had %d words\n",
				    section, section_data_count);
			}
			debug_printf(1, "_END on line %d\n", lineno);
			break;
		} else if (strncmp(line, "_DATA", 5) == 0) {
			if (section != '\0') {
				debug_printf(1, "%c section had %d words\n",
				    section, section_data_count);
			}
			debug_printf(1, "_DATA on line %d\n", lineno);
			parse_data_line(line, &section, &dsp_data_offset,
					&section_data_count);
			debug_printf(1, "%c data section starting at dsp "
			    "offset 0x%04x\n", section, dsp_data_offset);
		} else {
			// We must be in a _DATA section
			check(section != '\0', "parse error on line %d: not in "
			    "a _DATA section", lineno);

			char *word_str = strtok(line, whitespace);
			while (word_str != NULL) {
				u_int32_t word = strtol(word_str, NULL, 16);

				int cpu_offset = dsp_to_cpu_address(
				    section != 'L' ? section : (
				    section_data_count % 2 ?  'Y' : 'X'),
				    dsp_data_offset);
				if (section_data_count == 0 || section == 'L') {
					debug_printf(1, "  cpu offset 0x%04x "
					    " (KSEG1 address 0x%08x)\n",
					    cpu_offset,
					    0xbfbe0000 + cpu_offset * 4);
				}

				check(cpu_offset >= 0 &&
				    cpu_offset < firmware_words,
				    "cpu_offset %d is out of bounds for "
				    "section %c, on line %d", cpu_offset,
				    section, lineno);

				firmware[cpu_offset] = word;

				section_data_count++;
				dsp_data_offset++;

				word_str = strtok(NULL, whitespace);
			}
		}
	}
	fclose(fp);

	if (c_output) {
		printf("// Firmware in big-endian format:\n");
		printf("u_int32_t firmware[%d] = {\n", firmware_words);
		u_int32_t zeroes[16];
		memset(zeroes, 0, sizeof(zeroes));
		for (int i = 0; i < firmware_words; i += 4) { 
			if (i > 0) {
				printf(",\n");
			}
			if (i < firmware_words - 16 &&
			    memcmp(&firmware[i], zeroes, sizeof(zeroes)) == 0) {
				// Make empty ranges more compact.
				printf("\t/* 0x%04x */  0, 0, 0, 0, 0, 0, 0, "
				    "0, 0, 0, 0, 0, 0, 0, 0, 0", i);
				i += (16 - 4);
			} else {
				printf("\t/* 0x%04x */  0x%08x, 0x%08x, "
				    "0x%08x, 0x%08x", i,
				    firmware[i+0], firmware[i+1],
				    firmware[i+2], firmware[i+3]);
			}
		}
		printf("\n};\n");
	}

	// Output needs to be big-endian, so swap if we're not on a BE machine.
	for (int i = 0; i < firmware_words; i++) {
		firmware[i] = htonl(firmware[i]);
	}

	FILE *fpout = fopen(argv[1], "w");
	check(fpout != NULL, "unable to open output file %s: %s",
	    argv[1], strerror(errno));
	check(fwrite(firmware, sizeof(u_int32_t), firmware_words, fpout) ==
	    firmware_words, "failed to write entire output to file %s: %s",
	    argv[1], strerror(errno));
	fclose(fpout);

	return 0;
}
