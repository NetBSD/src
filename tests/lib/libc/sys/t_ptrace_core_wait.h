/*	$NetBSD: t_ptrace_core_wait.h,v 1.6 2022/06/07 05:39:16 skrll Exp $	*/

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


/*
 * Parse the core file and find the requested note.  If the reading or parsing
 * fails, the test is failed.  If the note is found, it is read onto buf, up to
 * buf_len.  The actual length of the note is returned (which can be greater
 * than buf_len, indicating that it has been truncated).  If the note is not
 * found, -1 is returned.
 *
 * If the note_name ends in '*', then we find the first note that matches
 * the note_name prefix up to the '*' character, e.g.:
 *
 *	NetBSD-CORE@*
 *
 * finds the first note whose name prefix matches "NetBSD-CORE@".
 */
static ssize_t core_find_note(const char *core_path,
    const char *note_name, uint64_t note_type, void *buf, size_t buf_len)
{
	int core_fd;
	Elf *core_elf;
	size_t core_numhdr, i;
	ssize_t ret = -1;
	size_t name_len = strlen(note_name);
	bool prefix_match = false;

	if (note_name[name_len - 1] == '*') {
		prefix_match = true;
		name_len--;
	} else {
		/* note: we assume note name will be null-terminated */
		name_len++;
	}

	SYSCALL_REQUIRE((core_fd = open(core_path, O_RDONLY)) != -1);
	SYSCALL_REQUIRE(elf_version(EV_CURRENT) != EV_NONE);
	SYSCALL_REQUIRE((core_elf = elf_begin(core_fd, ELF_C_READ, NULL)));

	SYSCALL_REQUIRE(elf_getphnum(core_elf, &core_numhdr) != 0);
	for (i = 0; i < core_numhdr && ret == -1; i++) {
		GElf_Phdr core_hdr;
		size_t offset;
		SYSCALL_REQUIRE(gelf_getphdr(core_elf, i, &core_hdr));
		if (core_hdr.p_type != PT_NOTE)
		    continue;

		for (offset = core_hdr.p_offset;
		    offset < core_hdr.p_offset + core_hdr.p_filesz;) {
			Elf64_Nhdr note_hdr;
			char name_buf[64];

			switch (gelf_getclass(core_elf)) {
			case ELFCLASS64:
				SYSCALL_REQUIRE(pread(core_fd, &note_hdr,
				    sizeof(note_hdr), offset)
				    == sizeof(note_hdr));
				offset += sizeof(note_hdr);
				break;
			case ELFCLASS32:
				{
				Elf32_Nhdr tmp_hdr;
				SYSCALL_REQUIRE(pread(core_fd, &tmp_hdr,
				    sizeof(tmp_hdr), offset)
				    == sizeof(tmp_hdr));
				offset += sizeof(tmp_hdr);
				note_hdr.n_namesz = tmp_hdr.n_namesz;
				note_hdr.n_descsz = tmp_hdr.n_descsz;
				note_hdr.n_type = tmp_hdr.n_type;
				}
				break;
			}

			/* indicates end of notes */
			if (note_hdr.n_namesz == 0 || note_hdr.n_descsz == 0)
				break;
			if (((prefix_match &&
			      note_hdr.n_namesz > name_len) ||
			     (!prefix_match &&
			      note_hdr.n_namesz == name_len)) &&
			    note_hdr.n_namesz <= sizeof(name_buf)) {
				SYSCALL_REQUIRE(pread(core_fd, name_buf,
				    note_hdr.n_namesz, offset)
				    == (ssize_t)(size_t)note_hdr.n_namesz);

				if (!strncmp(note_name, name_buf, name_len) &&
				    note_hdr.n_type == note_type)
					ret = note_hdr.n_descsz;
			}

			offset += note_hdr.n_namesz;
			/* fix to alignment */
			offset = roundup(offset, core_hdr.p_align);

			/* if name & type matched above */
			if (ret != -1) {
				ssize_t read_len = MIN(buf_len,
				    note_hdr.n_descsz);
				SYSCALL_REQUIRE(pread(core_fd, buf,
				    read_len, offset) == read_len);
				break;
			}

			offset += note_hdr.n_descsz;
			/* fix to alignment */
			offset = roundup(offset, core_hdr.p_align);
		}
	}

	elf_end(core_elf);
	close(core_fd);

	return ret;
}

ATF_TC(core_dump_procinfo);
ATF_TC_HEAD(core_dump_procinfo, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"Trigger a core dump and verify its contents.");
}

ATF_TC_BODY(core_dump_procinfo, tc)
{
	const int exitval = 5;
	pid_t child, wpid;
#if defined(TWAIT_HAVE_STATUS)
	const int sigval = SIGTRAP;
	int status;
#endif
	char core_path[] = "/tmp/core.XXXXXX";
	int core_fd;
	struct netbsd_elfcore_procinfo procinfo;

	DPRINTF("Before forking process PID=%d\n", getpid());
	SYSCALL_REQUIRE((child = fork()) != -1);
	if (child == 0) {
		DPRINTF("Before calling PT_TRACE_ME from child %d\n", getpid());
		FORKEE_ASSERT(ptrace(PT_TRACE_ME, 0, NULL, 0) != -1);

		DPRINTF("Before triggering SIGTRAP\n");
		trigger_trap();

		DPRINTF("Before exiting of the child process\n");
		_exit(exitval);
	}
	DPRINTF("Parent process PID=%d, child's PID=%d\n", getpid(), child);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_stopped(status, sigval);

	SYSCALL_REQUIRE((core_fd = mkstemp(core_path)) != -1);
	close(core_fd);

	DPRINTF("Call DUMPCORE for the child process\n");
	SYSCALL_REQUIRE(ptrace(PT_DUMPCORE, child, core_path, strlen(core_path))
	    != -1);

	DPRINTF("Read core file\n");
	ATF_REQUIRE_EQ(core_find_note(core_path, "NetBSD-CORE",
	    ELF_NOTE_NETBSD_CORE_PROCINFO, &procinfo, sizeof(procinfo)),
	    sizeof(procinfo));

	ATF_CHECK_EQ(procinfo.cpi_version, 1);
	ATF_CHECK_EQ(procinfo.cpi_cpisize, sizeof(procinfo));
	ATF_CHECK_EQ(procinfo.cpi_signo, SIGTRAP);
	ATF_CHECK_EQ(procinfo.cpi_pid, child);
	ATF_CHECK_EQ(procinfo.cpi_ppid, getpid());
	ATF_CHECK_EQ(procinfo.cpi_pgrp, getpgid(child));
	ATF_CHECK_EQ(procinfo.cpi_sid, getsid(child));
	ATF_CHECK_EQ(procinfo.cpi_ruid, getuid());
	ATF_CHECK_EQ(procinfo.cpi_euid, geteuid());
	ATF_CHECK_EQ(procinfo.cpi_rgid, getgid());
	ATF_CHECK_EQ(procinfo.cpi_egid, getegid());
	ATF_CHECK_EQ(procinfo.cpi_nlwps, 1);
	ATF_CHECK(procinfo.cpi_siglwp > 0);

	unlink(core_path);

	DPRINTF("Before resuming the child process where it left off and "
	    "without signal to be sent\n");

#if defined(__aarch64__) || defined(__arm__) || defined(__hppa___) || \
    defined(__powerpc__) || defined(__sh3__) || defined(sparc)
	/*
	 * For these archs, program counter is not automatically incremented
	 * by a trap instruction. We cannot increment PC in the trap handler,
	 * which breaks applications depending on this behavior, e.g., GDB.
	 * Therefore, we need to pass PC++ instead of (void *)1 (== PC) to
	 * PT_CONTINUE here.
	 */
	struct reg r;

	SYSCALL_REQUIRE(ptrace(PT_GETREGS, child, &r, 0) != -1);
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child,
	    (void *)(PTRACE_REG_PC(&r) + PTRACE_BREAKPOINT_SIZE), 0) != -1);
#else
	SYSCALL_REQUIRE(ptrace(PT_CONTINUE, child, (void *)1, 0) != -1);
#endif

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_SUCCESS(wpid = TWAIT_GENERIC(child, &status, 0), child);

	validate_status_exited(status, exitval);

	DPRINTF("Before calling %s() for the child\n", TWAIT_FNAME);
	TWAIT_REQUIRE_FAILURE(ECHILD, wpid = TWAIT_GENERIC(child, &status, 0));
}

#define ATF_TP_ADD_TCS_PTRACE_WAIT_CORE() \
	ATF_TP_ADD_TC(tp, core_dump_procinfo);
