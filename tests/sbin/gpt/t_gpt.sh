# $NetBSD: t_gpt.sh,v 1.7 2015/12/05 14:23:41 christos Exp $
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

bootblk=/usr/mdec/gptmbr.bin
size=10240
newsize=20480
shdr=34
disk=gpt.disk
uuid="........-....-....-....-............"
zero="00000000-0000-0000-0000-000000000000"
src=$(atf_get_srcdir)

prepare() {
	rm -f "$disk"
	dd if=/dev/zero of="$disk" seek="$size" count=1
}

prepare_2part() {
	prepare
	atf_check -s exit:0 -o empty -e empty gpt create "$disk"
	atf_check -s exit:0 -o match:"$(partaddmsg 1 34 1024)" -e empty \
	    gpt add -t efi -s 1024 "$disk"
	atf_check -s exit:0 -o match:"$(partaddmsg 2 1058 9150)" -e empty \
	    gpt add "$disk"
}

# Calling this from tests does not work. BUG!
check_2part() {
	atf_check -s exit:0 -o file:"$src/gpt.2part.show.normal" \
	    -e empty gpt show "$disk"
	atf_check -s exit:0 -o file:"$src/gpt.2part.show.uuid" \
	    -e empty gpt show -u "$disk"
}

partaddmsg() {
	echo "^$disk: Partition $1 added: $uuid $2 $3\$"
}

partresmsg() {
	echo "^$disk: Partition $1 resized: $2 $3\$"
}

partremmsg() {
	echo "^$disk: Partition $1 removed\$"
}

partlblmsg() {
	echo "^$disk: Partition $1 label changed\$"
}

partbootmsg() {
	echo "^$disk: Partition $1 marked as bootable\$"

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
	atf_check -s exit:0 -o empty -e empty gpt create "$disk"
	atf_check -s exit:0 -o file:"$src/gpt.empty.show.normal" \
	    -e empty gpt show "$disk"
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
	atf_check -s exit:0 -o save:test.backup -e empty gpt backup "$disk"
	atf_check -s exit:0 -o file:"$src/gpt.backup" -e empty \
	    sed -e "s/$uuid/$zero/g" "test.backup"
}

atf_test_case restore_2part
restore_2part_head() {
	atf_set "descr" "Restore 2 partition disk"
}

restore_2part_body() {
	prepare_2part
	atf_check -s exit:0 -o save:test.backup -e empty gpt backup "$disk"
	prepare
	atf_check -s exit:0 -o empty -e empty gpt restore -i test.backup "$disk"
	check_2part
}

atf_test_case recover_backup
recover_backup_head() {
	atf_set "descr" "Recover the backup GPT header and table"
}

recover_backup_body() {
	prepare_2part
	dd conv=notrunc if=/dev/zero of="$disk" seek="$((size - shdr))" \
	    count="$shdr"
	atf_check -s exit:0 -o match:"$(recovermsg secondary primary)" \
	    -e empty gpt recover "$disk"
	check_2part
}

atf_test_case recover_primary
recover_primary_head() {
	atf_set "descr" "Recover the primary GPT header and table"
}

recover_primary_body() {
	prepare_2part
	dd conv=notrunc if=/dev/zero of="$disk" seek=1 count="$shdr"
	atf_check -s exit:0 -o match:"$(recovermsg primary secondary)" \
	    -e empty gpt recover "$disk"
	check_2part
}

atf_test_case resize_2part
resize_2part_head() {
	atf_set "descr" "Resize a 2 partition disk and partition"
}

resize_2part_body() {
	prepare_2part
	dd conv=notrunc if=/dev/zero of="$disk" seek="$newsize" count=1
	atf_check -s exit:0 -o empty -e empty gpt resizedisk "$disk"
	atf_check -s exit:0 -o file:"$src/gpt.resizedisk.show.normal" \
	    gpt show "$disk"
	atf_check -s exit:0 -o match:"$(partresmsg 2 1058 19390)" \
	    -e empty gpt resize -i 2 "$disk"
	atf_check -s exit:0 -o file:"$src/gpt.resizepart.show.normal" \
	    gpt show "$disk"
}

atf_test_case remove_2part
remove_2part_head() {
	atf_set "descr" "Remove a partition from a 2 partition disk"
}

remove_2part_body() {
	prepare_2part
	atf_check -s exit:0 -o match:"$(partremmsg 1)" -e empty gpt remove \
	    -i 1 "$disk"
	atf_check -s exit:0 -o file:"$src/gpt.removepart.show.normal" \
	    gpt show "$disk"
}

atf_test_case label_2part
label_2part_head() {
	atf_set "descr" "Label partitions in a 2 partition disk"
}

label_2part_body() {
	prepare_2part
	atf_check -s exit:0 -o match:"$(partlblmsg 1)" -e empty \
	    gpt label -i 1 -l potato "$disk"
	atf_check -s exit:0 -o match:"$(partlblmsg 2)" -e empty \
	    gpt label -i 2 -l tomato "$disk"
	atf_check -s exit:0 -o file:"$src/gpt.2part.show.label" \
	    gpt show -l "$disk"
}

atf_test_case bootable_2part
bootable_2part_head() {
	atf_set "descr" "Make partition 2 bootable in a 2 partition disk"
}

bootable_2part_body() {
	prepare_2part
	atf_check -s exit:0 -o match:"$(partbootmsg 2)" -e empty \
	    gpt biosboot -i 2 "$disk"
	local bootsz="$(ls -l "$bootblk" | awk '{ print $5 }')"
	dd if="$disk" of=bootblk bs=1 count="$bootsz"
	atf_check -s exit:0 -o empty -e empty cmp "$bootblk" bootblk
	gpt show -i 2 "$disk" | tail -1 > bootattr
	atf_check -s exit:0 -o match:"^  legacy BIOS boot partition\$" \
	    -e empty cat bootattr
}

atf_init_test_cases() {
	atf_add_test_case create_empty
	atf_add_test_case create_2part
	atf_add_test_case backup_2part
	atf_add_test_case remove_2part
	atf_add_test_case restore_2part
	atf_add_test_case recover_backup
	atf_add_test_case recover_primary
	atf_add_test_case resize_2part
	atf_add_test_case label_2part
	atf_add_test_case bootable_2part
}
