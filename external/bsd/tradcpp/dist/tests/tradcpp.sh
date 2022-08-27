test_case() {
	local name="$1"
	local source="$2"
	local tradcpp="$3"
	local descr="Test tradcpp behavior"
	atf_test_case ${name}
	if [ -e "$(atf_get_srcdir)/${name}.txt" ]; then
		descr=$(cat "$(atf_get_srcdir)/${name}.txt")
	fi
	eval "${name}_head() { \
		atf_set descr \"${descr}\"; \
		atf_set require.progs \"/usr/bin/tradcpp\"; \
	}"
	local stdout="file:$(atf_get_srcdir)/${name}.good"
	local options=""
	local options_file="$(atf_get_srcdir)/${name}.cmdline"
	if [ -e ${options_file} ]; then
		options=$(cat ${options_file})
	fi
	eval "${name}_body() { \
		atf_check -s eq:0 -o ${stdout} -x '${tradcpp} ${options} ${source} 2>&1 || echo FAILED'; \
	}"
}

atf_init_test_cases() {
	local tradcpp=$(make -V .OBJDIR -C $(atf_get_srcdir)/..)/tradcpp
	if [ ! -x ${tradcpp} ]; then
		tradcpp=/usr/bin/tradcpp
	fi
	cd $(atf_get_srcdir)
	for testfile in t*.c; do
		local name=${testfile%%.c}
		test_case ${name} ${testfile} ${tradcpp}
		atf_add_test_case ${name}
	done
}
