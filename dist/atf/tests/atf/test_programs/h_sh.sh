#
# Automated Testing Framework (atf)
#
# Copyright (c) 2007 The NetBSD Foundation, Inc.
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
# 3. All advertising materials mentioning features or use of this
#    software must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
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
# Helper tests for "t_env".
# -------------------------------------------------------------------------

atf_test_case env_home
env_home_head()
{
    atf_set "descr" "Helper test case for the t_env test program"
}
env_home_body()
{
    [ -n "${HOME}" ] || atf_fail "HOME is undefined"
    h=$(cd ${HOME} && pwd -P)
    [ "${h}" = "$(pwd -P)" ] || atf_fail "HOME is invalid"
}

atf_test_case env_list
env_list_head()
{
    atf_set "descr" "Helper test case for the t_env test program"
}
env_list_body()
{
    env
}

# -------------------------------------------------------------------------
# Helper tests for "t_fork".
# -------------------------------------------------------------------------

atf_test_case fork_mangle_fds
fork_mangle_fds_head()
{
    atf_set "descr" "Helper test case for the t_fork test program"
}
fork_mangle_fds_body()
{
    resfd=$(atf_config_get resfd)
    touch in
    exec 0<in
    exec 1>out
    exec 2>err
    eval "exec ${resfd}>res"
}

atf_test_case fork_umask
fork_umask_head()
{
    atf_set "descr" "Helper test case for the t_fork test program"
}
fork_umask_body()
{
    echo "umask: $(umask)"
}

# -------------------------------------------------------------------------
# Helper tests for "t_meta_data".
# -------------------------------------------------------------------------

atf_test_case ident_1
ident_1_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
}
ident_1_body()
{
    atf_check_equal '$(atf_get ident)' ident_1
}

atf_test_case ident_2
ident_2_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
}
ident_2_body()
{
    atf_check_equal '$(atf_get ident)' ident_2
}

atf_test_case require_config
require_config_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
    atf_set "require.config" "var1 var2"
}
require_config_body()
{
    echo "var1: $(atf_config_get var1)"
    echo "var2: $(atf_config_get var2)"
}

atf_test_case require_progs_body
require_progs_body_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
}
require_progs_body_body()
{
    for p in $(atf_config_get progs); do
        atf_require_prog ${p}
    done
}

atf_test_case require_progs_head
require_progs_head_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
    atf_set "require.progs" "$(atf_config_get progs)"
}
require_progs_head_body()
{
    :
}

atf_test_case require_user_root
require_user_root_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
    atf_set "require.user" "root"
}
require_user_root_body()
{
    :
}

atf_test_case require_user_root2
require_user_root2_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
    atf_set "require.user" "root"
}
require_user_root2_body()
{
    :
}

atf_test_case require_user_unprivileged
require_user_unprivileged_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
    atf_set "require.user" "unprivileged"
}
require_user_unprivileged_body()
{
    :
}

atf_test_case require_user_unprivileged2
require_user_unprivileged2_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
    atf_set "require.user" "unprivileged"
}
require_user_unprivileged2_body()
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
# Helper tests for "t_workdir".
# -------------------------------------------------------------------------

atf_test_case workdir_path
workdir_path_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
}
workdir_path_body()
{
    pwd -P >$(atf_config_get pathfile)
}

atf_test_case workdir_cleanup
workdir_cleanup_head()
{
    atf_set "descr" "Helper test case for the t_meta_data test program"
}
workdir_cleanup_body()
{
    pwd -P >$(atf_config_get pathfile)

    mkdir 1
    mkdir 1/1
    mkdir 1/2
    mkdir 1/3
    mkdir 1/3/1
    mkdir 1/3/2
    mkdir 2
    touch 2/1
    touch 2/2
    mkdir 2/3
    touch 2/3/1
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

    # Add helper tests for t_env.
    atf_add_test_case env_home
    atf_add_test_case env_list

    # Add helper tests for t_fork.
    atf_add_test_case fork_mangle_fds
    atf_add_test_case fork_umask

    # Add helper tests for t_meta_data.
    atf_add_test_case ident_1
    atf_add_test_case ident_2
    atf_add_test_case require_config
    atf_add_test_case require_progs_body
    atf_add_test_case require_progs_head
    atf_add_test_case require_user_root
    atf_add_test_case require_user_root2
    atf_add_test_case require_user_unprivileged
    atf_add_test_case require_user_unprivileged2

    # Add helper tests for t_srcdir.
    atf_add_test_case srcdir_exists

    # Add helper tests for t_workdir.
    atf_add_test_case workdir_path
    atf_add_test_case workdir_cleanup
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
