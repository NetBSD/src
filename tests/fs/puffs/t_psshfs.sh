# $NetBSD: t_psshfs.sh,v 1.2 2007/12/28 08:57:42 jmmv Exp $
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
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# -------------------------------------------------------------------------
# Auxiliary functions.
# -------------------------------------------------------------------------

#
# Skips the calling test case if puffs is not supported in the kernel
# or if the calling user does not have the necessary permissions to mount
# file systems.
#
require_puffs() {
	case "$($(atf_get_srcdir)/h_have_puffs)" in
		eacces)
			atf_skip "Cannot open /dev/puffs for read/write access"
			;;
		enxio)
			atf_skip "puffs support not built into the kernel"
			;;
		failed)
			atf_skip "Unknown error trying to access /dev/puffs"
			;;
		yes)
			;;
		*)
			atf_fail "Unknown value returned by h_have_puffs"
			;;
	esac

	if [ $(id -u) -ne 0 -a $(sysctl -n vfs.generic.usermount) -eq 0 ]
	then
		atf_skip "Regular users cannot mount file systems" \
		    "(vfs.generic.usermount is set to 0)"
	fi
}

#
# Starts a SSH server and sets up the client to access it.
# Authentication is allowed and done using an RSA key exclusively, which
# is generated on the fly as part of the test case.
# XXX: Ideally, all the tests in this test program should be able to share
# the generated key, because creating it can be a very slow process on some
# machines.
#
start_ssh() {
	echo "Setting up SSH server configuration"
	sed -e "s,@SRCDIR@,$(atf_get_srcdir),g" -e "s,@WORKDIR@,$(pwd),g" \
	    $(atf_get_srcdir)/sshd_config.in >sshd_config || \
	    atf_fail "Failed to create sshd_config"
	atf_check 'cp /usr/libexec/sftp-server .' 0 null null

	/usr/sbin/sshd -e -D -f ./sshd_config >sshd.log 2>&1 &
	echo $! >sshd.pid
	echo "SSH server started (pid $(cat sshd.pid))"

	echo "Setting up SSH client configuration"
	atf_check 'ssh-keygen -f ssh_user_key -t rsa -b 1024 -N "" -q' \
	    0 null null
	atf_check 'cp ssh_user_key.pub authorized_keys' 0 null null
	echo "[localhost]:10000,[127.0.0.1]:10000,[::1]:10000" \
	    "$(cat $(atf_get_srcdir)/ssh_host_key.pub)" >known_hosts || \
	    atf_fail "Failed to create known_hosts"
	atf_check 'chmod 600 authorized_keys' 0 null null
	sed -e "s,@SRCDIR@,$(atf_get_srcdir),g" -e "s,@WORKDIR@,$(pwd),g" \
	    $(atf_get_srcdir)/ssh_config.in >ssh_config || \
	    atf_fail "Failed to create ssh_config"
}

#
# Stops the SSH server spawned by start_ssh and prints diagnosis data.
#
stop_ssh() {
	if [ -f sshd.pid ]; then
		echo "Stopping SSH server (pid $(cat sshd.pid))"
		kill $(cat sshd.pid)
	fi
	if [ -f sshd.log ]; then
		echo "Server output was:"
		sed -e 's,^,    ,' sshd.log
	fi
}

#
# Mounts the given source directory on the target directory using psshfs.
# Both directories are supposed to live on the current directory.
#
mount_psshfs() {
	atf_check "mount -t psshfs -o -F=$(pwd)/ssh_config
	    localhost:$(pwd)/${1} ${2}" 0 null null
}

#
# Compares the inodes of the two given files and returns true if they are
# different; false otherwise.
#
ne_inodes() {
	test $(stat -f %i ${1}) -ne $(stat -f %i ${2})
}

# -------------------------------------------------------------------------
# The test cases.
# -------------------------------------------------------------------------

atf_test_case inode_nos
inode_nos_head() {
	atf_set "descr" "Checks that different files get different inode" \
	    "numbers"
}
inode_nos_body() {
	require_puffs

	start_ssh

	mkdir root
	mkdir root/dir
	touch root/dir/file1
	touch root/dir/file2
	touch root/file3
	touch root/file4

	mkdir mnt
	mount_psshfs root mnt
	atf_check 'ne_inodes root/dir root/dir/file1' 0 null null
	atf_check 'ne_inodes root/dir root/dir/file2' 0 null null
	atf_check 'ne_inodes root/dir/file1 root/dir/file2' 0 null null
	atf_check 'ne_inodes root/file3 root/file4' 0 null null
}
inode_nos_cleanup() {
	umount mnt
	stop_ssh
}

atf_test_case pwd
pwd_head() {
	atf_set "descr" "Checks that pwd works correctly"
}
pwd_body() {
	require_puffs

	start_ssh

	mkdir root
	mkdir root/dir

	mkdir mnt
	atf_check 'echo $(cd mnt && /bin/pwd)/dir' 0 stdout null
	mv stdout expout
	mount_psshfs root mnt
	atf_check 'cd mnt/dir && ls .. >/dev/null && /bin/pwd' \
	    0 expout null
}
pwd_cleanup() {
	umount mnt
	stop_ssh
}

# -------------------------------------------------------------------------
# Initialization.
# -------------------------------------------------------------------------

atf_init_test_cases() {
	atf_add_test_case inode_nos
	atf_add_test_case pwd
}
