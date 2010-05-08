#
# Automated Testing Framework (atf)
#
# Copyright (c) 2007, 2008, 2010 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
# CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# -------------------------------------------------------------------------
# Helper tests for "t_cleanup".
# -------------------------------------------------------------------------

atf_test_case cleanup_pass
cleanup_pass_head()
{
    atf_set "descr" "Helper test case for the t_cleanup test program"
}
cleanup_pass_body()
{
    touch $(atf_config_get tmpfile)
}
cleanup_pass_cleanup()
{
    if [ $(atf_config_get cleanup no) = yes ]; then
        rm $(atf_config_get tmpfile)
    fi
}

atf_test_case cleanup_fail
cleanup_fail_head()
{
    atf_set "descr" "Helper test case for the t_cleanup test program"
}
cleanup_fail_body()
{
    touch $(atf_config_get tmpfile)
    atf_fail "On purpose"
}
cleanup_fail_cleanup()
{
    if [ $(atf_config_get cleanup no) = yes ]; then
        rm $(atf_config_get tmpfile)
    fi
}

atf_test_case cleanup_skip
cleanup_skip_head()
{
    atf_set "descr" "Helper test case for the t_cleanup test program"
}
cleanup_skip_body()
{
    touch $(atf_config_get tmpfile)
    atf_skip "On purpose"
}
cleanup_skip_cleanup()
{
    if [ $(atf_config_get cleanup no) = yes ]; then
        rm $(atf_config_get tmpfile)
    fi
}

atf_test_case cleanup_curdir
cleanup_curdir_head()
{
    atf_set "descr" "Helper test case for the t_cleanup test program"
}
cleanup_curdir_body()
{
    echo 1234 >oldvalue
}
cleanup_curdir_cleanup()
{
    test -f oldvalue && echo "Old value: $(cat oldvalue)"
}

atf_test_case cleanup_sigterm
cleanup_sigterm_head()
{
    atf_set "descr" "Helper test case for the t_cleanup test program"
}
cleanup_sigterm_body()
{
    touch $(atf_config_get tmpfile)
    kill $$
    touch $(atf_config_get tmpfile).no
}
cleanup_sigterm_cleanup()
{
    rm $(atf_config_get tmpfile)
}

atf_test_case cleanup_fork
cleanup_fork_head()
{
    atf_set "descr" "Helper test case for the t_cleanup test program"
}
cleanup_fork_body()
{
    :
}
cleanup_fork_cleanup()
{
    exec 1>out
    exec 2>err
    exec 3>res
    rm -f out err res
}

# -------------------------------------------------------------------------
# Helper tests for "t_config".
# -------------------------------------------------------------------------

atf_test_case config_unset
config_unset_head()
{
    atf_set "descr" "Helper test case for the t_config test program"
}
config_unset_body()
{
    atf_config_has 'test' && atf_fail "Test variable already defined"
}

atf_test_case config_empty
config_empty_head()
{
    atf_set "descr" "Helper test case for the t_config test program"
}
config_empty_body()
{
    atf_check_equal "$(atf_config_get 'test')" ""
}

atf_test_case config_value
config_value_head()
{
    atf_set "descr" "Helper test case for the t_config test program"
}
config_value_body()
{
    atf_check_equal "$(atf_config_get 'test')" "foo"
}

atf_test_case config_multi_value
config_multi_value_head()
{
    atf_set "descr" "Helper test case for the t_config test program"
}
config_multi_value_body()
{
    atf_check_equal "$(atf_config_get 'test')" "foo bar"
}

# -------------------------------------------------------------------------
# Helper tests for "t_fork".
# -------------------------------------------------------------------------

atf_test_case fork_stop
fork_stop_head()
{
    atf_set "descr" "Helper test case for the t_fork test program"
}
fork_stop_body()
{
    echo ${$} >$(atf_config_get pidfile)
    echo "Wrote pid file"
    echo "Waiting for done file"
    while ! test -f $(atf_config_get donefile); do sleep 1; done
    echo "Exiting"
}

# -------------------------------------------------------------------------
# Helper tests for "t_meta_data".
# -------------------------------------------------------------------------

atf_test_case metadata_no_descr
metadata_no_descr_head()
{
    :
}
metadata_no_descr_body()
{
    :
}

# -------------------------------------------------------------------------
# Helper tests for "t_srcdir".
# -------------------------------------------------------------------------

atf_test_case srcdir_exists
srcdir_exists_head()
{
    atf_set "descr" "Helper test case for the t_srcdir test program"
}
srcdir_exists_body()
{
    [ -f "$(atf_get_srcdir)/datafile" ] || atf_fail "Cannot find datafile"
}

# -------------------------------------------------------------------------
# Main.
# -------------------------------------------------------------------------

atf_init_test_cases()
{
    # Add helper tests for t_cleanup.
    atf_add_test_case cleanup_pass
    atf_add_test_case cleanup_fail
    atf_add_test_case cleanup_skip
    atf_add_test_case cleanup_curdir
    atf_add_test_case cleanup_sigterm
    atf_add_test_case cleanup_fork

    # Add helper tests for t_config.
    atf_add_test_case config_unset
    atf_add_test_case config_empty
    atf_add_test_case config_value
    atf_add_test_case config_multi_value

    # Add helper tests for t_fork.
    atf_add_test_case fork_stop

    # Add helper tests for t_meta_data.
    atf_add_test_case metadata_no_descr

    # Add helper tests for t_srcdir.
    atf_add_test_case srcdir_exists
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
