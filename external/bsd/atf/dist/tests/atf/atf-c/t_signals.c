/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <atf-c.h>

#include "atf-c/process.h"
#include "atf-c/signals.h"

#include "h_macros.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

static
void
null_handler(int signo)
{
}

static bool test1_happened = false;

static
void
test1_handler(int signo)
{
    test1_happened = true;
}

static bool test2_happened = false;

static
void
test2_handler(int signo)
{
    test2_happened = true;
}

/* ---------------------------------------------------------------------
 * Test cases for globals.
 * --------------------------------------------------------------------- */

ATF_TC(last_signo);
ATF_TC_HEAD(last_signo, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the value of "
                      "atf_signals_last_signo");
}
ATF_TC_BODY(last_signo, tc)
{
    struct sigaction sa;

    sa.sa_handler = null_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(atf_signals_last_signo, &sa, NULL) == -1)
        atf_tc_fail("Failed to program last signal (number %d)",
                    atf_signals_last_signo);

    if (sigaction(atf_signals_last_signo + 1, &sa, NULL) != -1)
        atf_tc_fail("Successfully programmed a signal after last (number %d)",
                    atf_signals_last_signo + 1);
}

/* ---------------------------------------------------------------------
 * Test cases for the "signal_holder" type.
 * --------------------------------------------------------------------- */

ATF_TC(signal_holder_init);
ATF_TC_HEAD(signal_holder_init, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_signal_holder_init "
                      "function");
}
ATF_TC_BODY(signal_holder_init, tc)
{
    atf_signal_programmer_t sp;
    atf_signal_holder_t sh;

    RE(atf_signal_programmer_init(&sp, SIGUSR1, test1_handler));

    RE(atf_signal_holder_init(&sh, SIGUSR1));
    ATF_REQUIRE(!test1_happened);
    ATF_REQUIRE(kill(getpid(), SIGUSR1) != -1);
    ATF_REQUIRE(!test1_happened);
    atf_signal_holder_fini(&sh);

    atf_signal_programmer_fini(&sp);
}

ATF_TC(signal_holder_fini);
ATF_TC_HEAD(signal_holder_fini, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_signal_holder_fini "
                      "function");
}
ATF_TC_BODY(signal_holder_fini, tc)
{
    atf_signal_programmer_t sp;
    atf_signal_holder_t sh;

    RE(atf_signal_programmer_init(&sp, SIGUSR1, test1_handler));

    printf("Checking that the signal is not delivered if it did not "
           "happen\n");
    test1_happened = false;
    RE(atf_signal_holder_init(&sh, SIGUSR1));
    atf_signal_holder_fini(&sh);
    ATF_REQUIRE(!test1_happened);
    printf("Checking that the original handler is restored\n");
    kill(getpid(), SIGUSR1);
    ATF_REQUIRE(test1_happened);

    printf("Checking that the signal is delivered if it happened\n");
    test1_happened = false;
    RE(atf_signal_holder_init(&sh, SIGUSR1));
    ATF_REQUIRE(kill(getpid(), SIGUSR1) != -1);
    atf_signal_holder_fini(&sh);
    ATF_REQUIRE(test1_happened);

    atf_signal_programmer_fini(&sp);
}

ATF_TC(signal_holder_process);
ATF_TC_HEAD(signal_holder_process, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_signal_holder_process "
                      "function");
}
ATF_TC_BODY(signal_holder_process, tc)
{
    atf_signal_programmer_t sp;
    atf_signal_holder_t sh;

    RE(atf_signal_programmer_init(&sp, SIGUSR1, test1_handler));

    printf("Checking that the signal is delivered if it happened\n");
    RE(atf_signal_holder_init(&sh, SIGUSR1));
    ATF_REQUIRE(kill(getpid(), SIGUSR1) != -1);
    ATF_REQUIRE(!test1_happened);
    atf_signal_holder_process(&sh);
    ATF_REQUIRE(test1_happened);
    test1_happened = false;
    printf("Checking that the signal is not re-delivered during "
           "destruction\n");
    atf_signal_holder_fini(&sh);
    ATF_REQUIRE(!test1_happened);

    atf_signal_programmer_fini(&sp);
}

/* ---------------------------------------------------------------------
 * Test cases for the "signal_programmer" type.
 * --------------------------------------------------------------------- */

ATF_TC(signal_programmer_init);
ATF_TC_HEAD(signal_programmer_init, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_signal_programmer_init "
                      "function");
}
ATF_TC_BODY(signal_programmer_init, tc)
{
    atf_signal_programmer_t sp;

    RE(atf_signal_programmer_init(&sp, SIGUSR1, test1_handler));
    ATF_REQUIRE(!test1_happened);
    ATF_REQUIRE(kill(getpid(), SIGUSR1) != -1);
    ATF_REQUIRE(test1_happened);

    RE(atf_signal_programmer_init(&sp, SIGUSR2, test2_handler));
    ATF_REQUIRE(!test2_happened);
    ATF_REQUIRE(kill(getpid(), SIGUSR2) != -1);
    ATF_REQUIRE(test2_happened);
}

ATF_TC(signal_programmer_fini);
ATF_TC_HEAD(signal_programmer_fini, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_signal_programmer_fini "
                      "function");
}
ATF_TC_BODY(signal_programmer_fini, tc)
{
    atf_signal_programmer_t sp1, sp2;

    RE(atf_signal_programmer_init(&sp1, SIGUSR1, test1_handler));
    RE(atf_signal_programmer_init(&sp2, SIGUSR1, test2_handler));

    test1_happened = test2_happened = false;
    kill(getpid(), SIGUSR1);
    ATF_REQUIRE(!test1_happened);
    ATF_REQUIRE(test2_happened);

    atf_signal_programmer_fini(&sp2);

    test1_happened = test2_happened = false;
    kill(getpid(), SIGUSR1);
    ATF_REQUIRE(test1_happened);
    ATF_REQUIRE(!test2_happened);

    atf_signal_programmer_fini(&sp1);
}

/* ---------------------------------------------------------------------
 * Test cases for the free functions.
 * --------------------------------------------------------------------- */

ATF_TC(signal_reset);
ATF_TC_HEAD(signal_reset, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_signal_reset function");
}
ATF_TC_BODY(signal_reset, tc)
{
    pid_t pid;
    RE(atf_process_fork(&pid));
    if (pid == 0) {
        struct sigaction sa;

        sa.sa_handler = test1_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        ATF_REQUIRE(sigaction(SIGTERM, &sa, NULL) != -1);
        ATF_REQUIRE(!test1_happened);
        kill(getpid(), SIGTERM);
        ATF_REQUIRE(test1_happened);

        test1_happened = false;
        atf_signal_reset(SIGTERM);
        kill(getpid(), SIGTERM);

        printf("Signal was not resetted correctly\n");
        abort();
    } else {
        int ecode;

        ATF_REQUIRE(waitpid(pid, &ecode, 0) != -1);
        ATF_REQUIRE(WIFEXITED(ecode) || WIFSIGNALED(ecode));
        ATF_REQUIRE(!WIFSIGNALED(ecode) || WTERMSIG(ecode) == SIGTERM);
    }
}

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    /* Add the tests for globals. */
    ATF_TP_ADD_TC(tp, last_signo);

    /* Add the tests for the "signal_holder" type. */
    ATF_TP_ADD_TC(tp, signal_holder_init);
    ATF_TP_ADD_TC(tp, signal_holder_fini);
    ATF_TP_ADD_TC(tp, signal_holder_process);

    /* Add the tests for the "signal_programmer" type. */
    ATF_TP_ADD_TC(tp, signal_programmer_init);
    ATF_TP_ADD_TC(tp, signal_programmer_fini);

    /* Add the tests for the free functions. */
    ATF_TP_ADD_TC(tp, signal_reset);

    return atf_no_error();
}
