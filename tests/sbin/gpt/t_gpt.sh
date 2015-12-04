# $NetBSD: t_gpt.sh,v 1.3 2015/12/04 01:21:12 christos Exp $
#
# Copyright (c) 2015 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Christos Zoulas
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

size=10240
shdr=34
disk=gpt.disk
uuid="........-....-....-....-............"
zero="00000000-0000-0000-0000-000000000000"

prepare() {
	rm -f $disk
	dd if=/dev/zero of=$disk seek=$size count=1
}

prepare_2part() {
	prepare
	atf_check -s exit:0 -o empty -e empty gpt create $disk
	atf_check -s exit:0 -o match:"$(partmsg 1 34 1024)" -e empty \
	    gpt add -t efi -s 1024 $disk
	atf_check -s exit:0 -o match:"$(partmsg 2 1058 9150)" -e empty \
	    gpt add $disk
}

# Calling this from tests does not work. BUG!
check_2part() {
#	atf_check -s exit:0 -o file:gpt.2part.show.normal \
#	    -e empty gpt show $disk
#	atf_check -s exit:0 -o file:gpt.2part.show.guid \
#	    -e empty gpt show -g $disk
}

partmsg() {
	echo "^$disk: Partition $1 added: $uuid $2 $3\$"
}

recovermsg() {
	echo "^$disk: Recovered $1 GPT [a-z]* from $2\$"
}

atf_test_case create_empty
create_empty_head() {
	atf_set "descr" "Create empty disk"
}

create_empty_body() {
	prepare
	atf_check -s exit:0 -o empty -e empty gpt create $disk
	atf_check -s exit:0 -o file:gpt.empty.show.normal \
	    -e empty gpt show $disk
}

atf_test_case create_2part
create_2part_head() {
	atf_set "descr" "Create 2 partition disk"
}

create_2part_body() {
	prepare_2part
	check_2part
}

atf_test_case backup_2part
backup_2part_head() {
	atf_set "descr" "Backup 2 partition disk"
}

backup_2part_body() {
	prepare_2part
	atf_check -s exit:0 -o save:test.backup -e empty gpt backup $disk
	atf_check -s exit:0 -o file:gpt.backup -e empty \
	    sed -e "s/$uuid/$zero/g" test.backup
}

atf_test_case restore_2part
restore_2part_head() {
	atf_set "descr" "Restore 2 partition disk"
}

restore_2part_body() {
	prepare_2part
	atf_check -s exit:0 -o save:test.backup -e empty gpt backup $disk
	prepare
	atf_check -s exit:0 -o empty -e empty gpt restore -i test.backup $disk
	check_2part
}

atf_test_case recover_backup
recover_backup_head() {
	atf_set "descr" "Recover the backup GPT header and table"
}

recover_backup_body() {
	prepare_2part
	dd conv=notrunc if=/dev/zero of=$disk seek=$((size - shdr)) count=$shdr
	atf_check -s exit:0 -o match:"$(recovermsg secondary primary)" \
	    -e empty gpt recover $disk
	check_2part
}

atf_test_case recover_primary
recover_primary_head() {
	atf_set "descr" "Recover the primary GPT header and table"
}

recover_primary_body() {
	prepare_2part
	dd conv=notrunc if=/dev/zero of=$disk seek=1 count=$shdr
	atf_check -s exit:0 -o match:"$(recovermsg primary secondary)" \
	    -e empty gpt recover $disk
	check_2part
}

atf_init_test_cases() {
	atf_add_test_case create_empty
	atf_add_test_case create_2part
	atf_add_test_case backup_2part
	atf_add_test_case restore_2part
	atf_add_test_case recover_backup
	atf_add_test_case recover_primary
}
