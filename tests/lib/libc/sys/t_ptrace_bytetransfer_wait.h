/*	$NetBSD: t_ptrace_bytetransfer_wait.h,v 1.1 2020/05/04 22:05:28 kamil Exp $	*/

/*-
 * Copyright (c) 2016, 2017, 2018, 2019, 2020 The NetBSD Foundation, Inc.
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


enum bytes_transfer_type {
	BYTES_TRANSFER_DATA,
	BYTES_TRANSFER_DATAIO,
	BYTES_TRANSFER_TEXT,
	BYTES_TRANSFER_TEXTIO,
	BYTES_TRANSFER_AUXV
};

static int __used
bytes_transfer_dummy(int a, int b, int c, int d)
{
	int e, f, g, h;

	a *= 4;
	b += 3;
	c -= 2;
	d /= 1;

	e = strtol("10", NULL, 10);
	f = strtol("20", NULL, 10);
	g = strtol("30", NULL, 10);
	h = strtol("40", NULL, 10);

	return (a + b * c - d) + (e * f - g / h);
}

static void
bytes_transfer(int operation, size_t size, enum bytes_transfer_type type)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
	bool skip = false;

	int lookup_me = 0;
	uint8_t lookup_me8 = 0;
	uint16_t lookup_me16 = 0;
	uint32_t lookup_me32 = 0;
	uint64_t lookup_me64 = 0;

	int magic = 0x13579246;
	uint8_t magic8 = 0xab;
	uint16_t magic16 = 0x1234;
	uint32_t magic32 = 0x98765432;
	uint64_t magic64 = 0xabcdef0123456789;

	struct ptrace_io_desc io;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	/* 513 is just enough, for the purposes of ATF it's good enough */
	AuxInfo ai[513], *aip;

	ATF_REQUIRE(size < sizeof(ai));

	/* Prepare variables for .TEXT transfers */
	switch (type) {
	case BYTES_TRANSFER_TEXT:
		memcpy(&magic, bytes_transfer_dummy, sizeof(magic));
		break;
	case BYTES_TRANSFER_TEXTIO:
		switch (size) {
		case 8:
			memcpy(&magic8, bytes_transfer_dummy, sizeof(magic8));
			break;
		case 16:
			memcpy(&magic16, bytes_transfer_dummy, sizeof(magic16));
			break;
		case 32:
			memcpy(&magic32, bytes_transfer_dummy, sizeof(magic32));
			break;
		case 64:
			memcpy(&magic64, bytes_transfer_dummy, sizeof(magic64));
			break;
		}
		break;
	default:
		break;
	}

	/* Prepare variables for PIOD and AUXV transfers */
	switch (type) {
	case BYTES_TRANSFER_TEXTIO:
	case BYTES_TRANSFER_DATAIO:
		io.piod_op = operation;
		switch (size) {
		case 8:
			io.piod_offs = (type == BYTES_TRANSFER_TEXTIO) ?
			               (void *)bytes_transfer_dummy :
			               &lookup_me8;
			io.piod_addr = &lookup_me8;
			io.piod_len = sizeof(lookup_me8);
			break;
		case 16:
			io.piod_offs = (type == BYTES_TRANSFER_TEXTIO) ?
			               (void *)bytes_transfer_dummy :
			               &lookup_me16;
			io.piod_addr = &lookup_me16;
			io.piod_len = sizeof(lookup_me16);
			break;
		case 32:
			io.piod_offs = (type == BYTES_TRANSFER_TEXTIO) ?
			               (void *)bytes_transfer_dummy :
			               &lookup_me32;
			io.piod_addr = &lookup_me32;
			io.piod_len = sizeof(lookup_me32);
			break;
		case 64:
			io.piod_offs = (type == BYTES_TRANSFER_TEXTIO) ?
			               (void *)bytes_transfer_dummy :
			               &lookup_me64;
			io.piod_addr = &lookup_me64;
			io.piod_len = sizeof(lookup_me64);
			break;
		default:
			break;
		}
		break;
	case BYTES_TRANSFER_AUXV:
		io.piod_op = operation;
		io.piod_offs = 0;
		io.piod_addr = ai;
		io.piod_len = size;
		break;
	default:
		break;
	}

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		switch (type) {
		case BYTES_TRANSFER_DATA:
			switch (operation) {
			case PT_READ_D:
			case PT_READ_I:
				lookup_me = magic;
				break;
			default:
				break;
			}
			break;
		case BYTES_TRANSFER_DATAIO:
			switch (operation) {
			case PIOD_READ_D:
			case PIOD_READ_I:
				switch (size) {
				case 8:
					lookup_me8 = magic8;
					break;
				case 16:
					lookup_me16 = magic16;
					break;
				case 32:
					lookup_me32 = magic32;
					break;
				case 64:
					lookup_me64 = magic64;
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		default:
			break;
		}

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		/* Handle PIOD and PT separately as operation values overlap */
		switch (type) {
		case BYTES_TRANSFER_DATA:
			switch (operation) {
			case PT_WRITE_D:
			case PT_WRITE_I:
				FORKEE_ASSERT_EQ(lookup_me, magic);
				break;
			default:
				break;
			}
			break;
		case BYTES_TRANSFER_DATAIO:
			switch (operation) {
			case PIOD_WRITE_D:
			case PIOD_WRITE_I:
				switch (size) {
				case 8:
					FORKEE_ASSERT_EQ(lookup_me8, magic8);
					break;
				case 16:
					FORKEE_ASSERT_EQ(lookup_me16, magic16);
					break;
				case 32:
					FORKEE_ASSERT_EQ(lookup_me32, magic32);
					break;
				case 64:
					FORKEE_ASSERT_EQ(lookup_me64, magic64);
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
			break;
		case BYTES_TRANSFER_TEXT:
			FORKEE_ASSERT(memcmp(&magic, bytes_transfer_dummy,
			                     sizeof(magic)) == 0);
			break;
		case BYTES_TRANSFER_TEXTIO:
			switch (size) {
			case 8:
				FORKEE_ASSERT(memcmp(&magic8,
				                     bytes_transfer_dummy,
				                     sizeof(magic8)) == 0);
				break;
			case 16:
				FORKEE_ASSERT(memcmp(&magic16,
				                     bytes_transfer_dummy,
				                     sizeof(magic16)) == 0);
				break;
			case 32:
				FORKEE_ASSERT(memcmp(&magic32,
				                     bytes_transfer_dummy,
				                     sizeof(magic32)) == 0);
				break;
			case 64:
				FORKEE_ASSERT(memcmp(&magic64,
				                     bytes_transfer_dummy,
				                     sizeof(magic64)) == 0);
				break;
			}
			break;
		default:
			break;
		}

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	/* Check PaX MPROTECT */
	if (!can_we_write_to_text(child)) {
		switch (type) {
		case BYTES_TRANSFER_TEXTIO:
			switch (operation) {
			case PIOD_WRITE_D:
			case PIOD_WRITE_I:
				skip = true;
				break;
			default:
				break;
			}
			break;
		case BYTES_TRANSFER_TEXT:
			switch (operation) {
			case PT_WRITE_D:
			case PT_WRITE_I:
				skip = true;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	/* Bailout cleanly killing the child process */
	if (skip) {
		SYSCALL_REQUIRE(ptrace(PT_KILL, child, (void *)1, 0) != -1);
		DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
		TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
		                      child);

		validate_status_signaled(status, SIGKILL, 0);

		atf_tc_skip("PaX MPROTECT setup prevents writes to .text");
	}

	DPRINTF("Calling operation to transfer bytes between child=%d and "
	       "parent=%d\n", child, getpid());

	switch (type) {
	case BYTES_TRANSFER_TEXTIO:
	case BYTES_TRANSFER_DATAIO:
	case BYTES_TRANSFER_AUXV:
		switch (operation) {
		case PIOD_WRITE_D:
		case PIOD_WRITE_I:
			switch (size) {
			case 8:
				lookup_me8 = magic8;
				break;
			case 16:
				lookup_me16 = magic16;
				break;
			case 32:
				lookup_me32 = magic32;
				break;
			case 64:
				lookup_me64 = magic64;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, 0) != -1);
		switch (operation) {
		case PIOD_READ_D:
		case PIOD_READ_I:
			switch (size) {
			case 8:
				ATF_REQUIRE_EQ(lookup_me8, magic8);
				break;
			case 16:
				ATF_REQUIRE_EQ(lookup_me16, magic16);
				break;
			case 32:
				ATF_REQUIRE_EQ(lookup_me32, magic32);
				break;
			case 64:
				ATF_REQUIRE_EQ(lookup_me64, magic64);
				break;
			default:
				break;
			}
			break;
		case PIOD_READ_AUXV:
			DPRINTF("Asserting that AUXV length (%zu) is > 0\n",
			        io.piod_len);
			ATF_REQUIRE(io.piod_len > 0);
			for (aip = ai; aip->a_type != AT_NULL; aip++)
				DPRINTF("a_type=%#llx a_v=%#llx\n",
				    (long long int)aip->a_type,
				    (long long int)aip->a_v);
			break;
		default:
			break;
		}
		break;
	case BYTES_TRANSFER_TEXT:
		switch (operation) {
		case PT_READ_D:
		case PT_READ_I:
			errno = 0;
			lookup_me = ptrace(operation, child,
			                   bytes_transfer_dummy, 0);
			ATF_REQUIRE_EQ(lookup_me, magic);
			SYSCALL_REQUIRE_ERRNO(errno, 0);
			break;
		case PT_WRITE_D:
		case PT_WRITE_I:
			SYSCALL_REQUIRE(ptrace(operation, child,
			                       bytes_transfer_dummy, magic)
			                != -1);
			break;
		default:
			break;
		}
		break;
	case BYTES_TRANSFER_DATA:
		switch (operation) {
		case PT_READ_D:
		case PT_READ_I:
			errno = 0;
			lookup_me = ptrace(operation, child, &lookup_me, 0);
			ATF_REQUIRE_EQ(lookup_me, magic);
			SYSCALL_REQUIRE_ERRNO(errno, 0);
			break;
		case PT_WRITE_D:
		case PT_WRITE_I:
			lookup_me = magic;
			SYSCALL_REQUIRE(ptrace(operation, child, &lookup_me,
			                       magic) != -1);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define BYTES_TRANSFER(test, operation, size, type)			\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify bytes transfer operation" #operation " and size " #size \
	    " of type " #type);						\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	bytes_transfer(operation, size, BYTES_TRANSFER_##type);		\
}

// DATA

BYTES_TRANSFER(bytes_transfer_piod_read_d_8, PIOD_READ_D, 8, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_16, PIOD_READ_D, 16, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_32, PIOD_READ_D, 32, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_64, PIOD_READ_D, 64, DATAIO)

BYTES_TRANSFER(bytes_transfer_piod_read_i_8, PIOD_READ_I, 8, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_16, PIOD_READ_I, 16, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_32, PIOD_READ_I, 32, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_64, PIOD_READ_I, 64, DATAIO)

BYTES_TRANSFER(bytes_transfer_piod_write_d_8, PIOD_WRITE_D, 8, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_16, PIOD_WRITE_D, 16, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_32, PIOD_WRITE_D, 32, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_64, PIOD_WRITE_D, 64, DATAIO)

BYTES_TRANSFER(bytes_transfer_piod_write_i_8, PIOD_WRITE_I, 8, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_16, PIOD_WRITE_I, 16, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_32, PIOD_WRITE_I, 32, DATAIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_64, PIOD_WRITE_I, 64, DATAIO)

BYTES_TRANSFER(bytes_transfer_read_d, PT_READ_D, 32, DATA)
BYTES_TRANSFER(bytes_transfer_read_i, PT_READ_I, 32, DATA)
BYTES_TRANSFER(bytes_transfer_write_d, PT_WRITE_D, 32, DATA)
BYTES_TRANSFER(bytes_transfer_write_i, PT_WRITE_I, 32, DATA)

// TEXT

BYTES_TRANSFER(bytes_transfer_piod_read_d_8_text, PIOD_READ_D, 8, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_16_text, PIOD_READ_D, 16, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_32_text, PIOD_READ_D, 32, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_d_64_text, PIOD_READ_D, 64, TEXTIO)

BYTES_TRANSFER(bytes_transfer_piod_read_i_8_text, PIOD_READ_I, 8, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_16_text, PIOD_READ_I, 16, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_32_text, PIOD_READ_I, 32, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_read_i_64_text, PIOD_READ_I, 64, TEXTIO)

BYTES_TRANSFER(bytes_transfer_piod_write_d_8_text, PIOD_WRITE_D, 8, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_16_text, PIOD_WRITE_D, 16, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_32_text, PIOD_WRITE_D, 32, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_d_64_text, PIOD_WRITE_D, 64, TEXTIO)

BYTES_TRANSFER(bytes_transfer_piod_write_i_8_text, PIOD_WRITE_I, 8, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_16_text, PIOD_WRITE_I, 16, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_32_text, PIOD_WRITE_I, 32, TEXTIO)
BYTES_TRANSFER(bytes_transfer_piod_write_i_64_text, PIOD_WRITE_I, 64, TEXTIO)

BYTES_TRANSFER(bytes_transfer_read_d_text, PT_READ_D, 32, TEXT)
BYTES_TRANSFER(bytes_transfer_read_i_text, PT_READ_I, 32, TEXT)
BYTES_TRANSFER(bytes_transfer_write_d_text, PT_WRITE_D, 32, TEXT)
BYTES_TRANSFER(bytes_transfer_write_i_text, PT_WRITE_I, 32, TEXT)

// AUXV

BYTES_TRANSFER(bytes_transfer_piod_read_auxv, PIOD_READ_AUXV, 4096, AUXV)

/// ----------------------------------------------------------------------------

static void
bytes_transfer_alignment(const char *operation)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	char *buffer;
	int vector;
	size_t len;
	size_t i;
	int op;

	struct ptrace_io_desc io;
	struct ptrace_siginfo info;

	memset(&io, 0, sizeof(io));
	memset(&info, 0, sizeof(info));

	/* Testing misaligned byte transfer crossing page boundaries */
	len = sysconf(_SC_PAGESIZE) * 2;
	buffer = malloc(len);
	ATF_REQUIRE(buffer != NULL);

	/* Initialize the buffer with random data */
	for (i = 0; i < len; i++)
		buffer[i] = i & 0xff;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info))
		!= -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		"si_errno=%#x\n",
		info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
		info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	if (strcmp(operation, "PT_READ_I") == 0 ||
	    strcmp(operation, "PT_READ_D") == 0) {
		if (strcmp(operation, "PT_READ_I"))
			op = PT_READ_I;
		else
			op = PT_READ_D;

		for (i = 0; i <= (len - sizeof(int)); i++) {
			errno = 0;
			vector = ptrace(op, child, buffer + i, 0);
			ATF_REQUIRE_EQ(errno, 0);
			ATF_REQUIRE(!memcmp(&vector, buffer + i, sizeof(int)));
		}
	} else if (strcmp(operation, "PT_WRITE_I") == 0 ||
	           strcmp(operation, "PT_WRITE_D") == 0) {
		if (strcmp(operation, "PT_WRITE_I"))
			op = PT_WRITE_I;
		else
			op = PT_WRITE_D;

		for (i = 0; i <= (len - sizeof(int)); i++) {
			memcpy(&vector, buffer + i, sizeof(int));
			SYSCALL_REQUIRE(ptrace(op, child, buffer + 1, vector)
			    != -1);
		}
	} else if (strcmp(operation, "PIOD_READ_I") == 0 ||
	           strcmp(operation, "PIOD_READ_D") == 0) {
		if (strcmp(operation, "PIOD_READ_I"))
			op = PIOD_READ_I;
		else
			op = PIOD_READ_D;

		io.piod_op = op;
		io.piod_addr = &vector;
		io.piod_len = sizeof(int);

		for (i = 0; i <= (len - sizeof(int)); i++) {
			io.piod_offs = buffer + i;

			SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, sizeof(io))
			                != -1);
			ATF_REQUIRE(!memcmp(&vector, buffer + i, sizeof(int)));
		}
	} else if (strcmp(operation, "PIOD_WRITE_I") == 0 ||
	           strcmp(operation, "PIOD_WRITE_D") == 0) {
		if (strcmp(operation, "PIOD_WRITE_I"))
			op = PIOD_WRITE_I;
		else
			op = PIOD_WRITE_D;

		io.piod_op = op;
		io.piod_addr = &vector;
		io.piod_len = sizeof(int);

		for (i = 0; i <= (len - sizeof(int)); i++) {
			io.piod_offs = buffer + i;

			SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, sizeof(io))
			                != -1);
		}
	} else if (strcmp(operation, "PIOD_READ_AUXV") == 0) {
		io.piod_op = PIOD_READ_AUXV;
		io.piod_addr = &vector;
		io.piod_len = sizeof(int);

		errno = 0;
		i = 0;
		/* Read the whole AUXV vector, it has no clear length */
		while (io.piod_len > 0) {
			io.piod_offs = (void *)(intptr_t)i;
			SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, sizeof(io))
			                != -1 || (io.piod_len == 0 && i > 0));
			++i;
		}
	}

	DPRINTF("Before resuming the child process where it left off "
	    "and without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
	    child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define BYTES_TRANSFER_ALIGNMENT(test, operation)			\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify bytes transfer for potentially misaligned "		\
	    "operation " operation);					\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	bytes_transfer_alignment(operation);				\
}

BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_pt_read_i, "PT_READ_I")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_pt_read_d, "PT_READ_D")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_pt_write_i, "PT_WRITE_I")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_pt_write_d, "PT_WRITE_D")

BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_piod_read_i, "PIOD_READ_I")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_piod_read_d, "PIOD_READ_D")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_piod_write_i, "PIOD_WRITE_I")
BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_piod_write_d, "PIOD_WRITE_D")

BYTES_TRANSFER_ALIGNMENT(bytes_transfer_alignment_piod_read_auxv, "PIOD_READ_AUXV")

/// ----------------------------------------------------------------------------

static void
bytes_transfer_eof(const char *operation)
{
	const int exitval = 5;
	const int sigval = SIGSTOP;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	int status;
#endif
	FILE *fp;
	char *p;
	int vector;
	int op;

	struct ptrace_io_desc io;
	struct ptrace_siginfo info;

	memset(&io, 0, sizeof(io));
	memset(&info, 0, sizeof(info));

	vector = 0;

	fp = tmpfile();
	ATF_REQUIRE(fp != NULL);

	p = mmap(0, 1, PROT_READ|PROT_WRITE, MAP_PRIVATE, fileno(fp), 0);
	ATF_REQUIRE(p != MAP_FAILED);

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before raising %s from child\n", strsignal(sigval));
		FORKEE_ASSERT(raise(sigval) == 0);

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	DPRINTF("Before calling ptrace(2) with PT_GET_SIGINFO for child\n");
	SYSCALL_REQUIRE(ptrace(PT_GET_SIGINFO, child, &info, sizeof(info))
		!= -1);

	DPRINTF("Signal traced to lwpid=%d\n", info.psi_lwpid);
	DPRINTF("Signal properties: si_signo=%#x si_code=%#x "
		"si_errno=%#x\n",
		info.psi_siginfo.si_signo, info.psi_siginfo.si_code,
		info.psi_siginfo.si_errno);

	ATF_REQUIRE_EQ(info.psi_siginfo.si_signo, sigval);
	ATF_REQUIRE_EQ(info.psi_siginfo.si_code, SI_LWP);

	if (strcmp(operation, "PT_READ_I") == 0 ||
	    strcmp(operation, "PT_READ_D") == 0) {
		if (strcmp(operation, "PT_READ_I"))
			op = PT_READ_I;
		else
			op = PT_READ_D;

		errno = 0;
		SYSCALL_REQUIRE(ptrace(op, child, p, 0) == -1);
		ATF_REQUIRE_EQ(errno, EINVAL);
	} else if (strcmp(operation, "PT_WRITE_I") == 0 ||
	           strcmp(operation, "PT_WRITE_D") == 0) {
		if (strcmp(operation, "PT_WRITE_I"))
			op = PT_WRITE_I;
		else
			op = PT_WRITE_D;

		errno = 0;
		SYSCALL_REQUIRE(ptrace(op, child, p, vector) == -1);
		ATF_REQUIRE_EQ(errno, EINVAL);
	} else if (strcmp(operation, "PIOD_READ_I") == 0 ||
	           strcmp(operation, "PIOD_READ_D") == 0) {
		if (strcmp(operation, "PIOD_READ_I"))
			op = PIOD_READ_I;
		else
			op = PIOD_READ_D;

		io.piod_op = op;
		io.piod_addr = &vector;
		io.piod_len = sizeof(int);
		io.piod_offs = p;

		errno = 0;
		SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, sizeof(io)) == -1);
		ATF_REQUIRE_EQ(errno, EINVAL);
	} else if (strcmp(operation, "PIOD_WRITE_I") == 0 ||
	           strcmp(operation, "PIOD_WRITE_D") == 0) {
		if (strcmp(operation, "PIOD_WRITE_I"))
			op = PIOD_WRITE_I;
		else
			op = PIOD_WRITE_D;

		io.piod_op = op;
		io.piod_addr = &vector;
		io.piod_len = sizeof(int);
		io.piod_offs = p;

		errno = 0;
		SYSCALL_REQUIRE(ptrace(PT_IO, child, &io, sizeof(io)) == -1);
		ATF_REQUIRE_EQ(errno, EINVAL);
	}

	DPRINTF("Before resuming the child process where it left off "
	    "and without signal to be sent\n");
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0),
	    child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define BYTES_TRANSFER_EOF(test, operation)				\
ATF_TC(test);								\
ATF_TC_HEAD(test, tc)							\
{									\
	atf_tc_set_md_var(tc, "descr",					\
	    "Verify bytes EOF byte transfer for the " operation		\
	    " operation");						\
}									\
									\
ATF_TC_BODY(test, tc)							\
{									\
									\
	bytes_transfer_eof(operation);					\
}

BYTES_TRANSFER_EOF(bytes_transfer_eof_pt_read_i, "PT_READ_I")
BYTES_TRANSFER_EOF(bytes_transfer_eof_pt_read_d, "PT_READ_D")
BYTES_TRANSFER_EOF(bytes_transfer_eof_pt_write_i, "PT_WRITE_I")
BYTES_TRANSFER_EOF(bytes_transfer_eof_pt_write_d, "PT_WRITE_D")

BYTES_TRANSFER_EOF(bytes_transfer_eof_piod_read_i, "PIOD_READ_I")
BYTES_TRANSFER_EOF(bytes_transfer_eof_piod_read_d, "PIOD_READ_D")
BYTES_TRANSFER_EOF(bytes_transfer_eof_piod_write_i, "PIOD_WRITE_I")
BYTES_TRANSFER_EOF(bytes_transfer_eof_piod_write_d, "PIOD_WRITE_D")


#define ATF_TP_ADD_TCS_PTRACE_WAIT_BYTETRANSFER() \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_8); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_16); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_32); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_64); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_8); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_16); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_32); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_64); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_8); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_16); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_32); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_64); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_8); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_16); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_32); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_64); \
	ATF_TP_ADD_TC(tp, bytes_transfer_read_d); \
	ATF_TP_ADD_TC(tp, bytes_transfer_read_i); \
	ATF_TP_ADD_TC(tp, bytes_transfer_write_d); \
	ATF_TP_ADD_TC(tp, bytes_transfer_write_i); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_8_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_16_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_32_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_d_64_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_8_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_16_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_32_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_i_64_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_8_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_16_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_32_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_d_64_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_8_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_16_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_32_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_write_i_64_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_read_d_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_read_i_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_write_d_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_write_i_text); \
	ATF_TP_ADD_TC(tp, bytes_transfer_piod_read_auxv); \
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_pt_read_i); \
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_pt_read_d); \
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_pt_write_i); \
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_pt_write_d); \
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_piod_read_i); \
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_piod_read_d); \
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_piod_write_i); \
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_piod_write_d); \
	ATF_TP_ADD_TC(tp, bytes_transfer_alignment_piod_read_auxv); \
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_pt_read_i); \
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_pt_read_d); \
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_pt_write_i); \
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_pt_write_d); \
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_piod_read_i); \
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_piod_read_d); \
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_piod_write_i); \
	ATF_TP_ADD_TC(tp, bytes_transfer_eof_piod_write_d);
