# $NetBSD: ffs_common.sh,v 1.5 2020/08/20 07:23:20 gson Exp $ 

create_ffs()
{
	local endian=$1; shift
	local vers=$1; shift
	local type=$1; shift
	local op;
	if [ ${type} = "both" ]; then
		op="-q user -q group"
	else
		op="-q ${type}"
	fi
	atf_check -o ignore -e ignore newfs ${op} \
		-B ${endian} -O ${vers} -s 4000 -F ${IMG}
}

create_ffs_server()
{	
	local sarg=$1; shift
	create_ffs $*
	atf_check -o ignore -e ignore $(atf_get_srcdir)/h_ffs_server \
		${sarg} ${IMG} ${RUMP_SERVER}
}

# from tests/ipf/h_common.sh via tests/sbin/resize_ffs
test_case()
{
	local name="${1}"; shift
	local check_function="${1}"; shift
	local descr="${1}"; shift
	
	atf_test_case "${name}"

	eval "${name}_head() { \
		atf_set "descr" "${descr}"
		atf_set "timeout" "120"
	}"
	eval "${name}_body() { \
		RUMP_SOCKETS_LIST=\${RUMP_SOCKET}; \
		export RUMP_SERVER=unix://\${RUMP_SOCKET}; \
		${check_function} " "${@}" "; \
	}"
	tests="${tests} ${name}"
}

test_case_root()
{
	local name="${1}"; shift
	local check_function="${1}"; shift
	local descr="${1}"; shift
	
	atf_test_case "${name}"

	eval "${name}_head() { \
		atf_set "descr" "${descr}"
		atf_set "require.user" "root"
		atf_set "timeout" "360"
	}"
	eval "${name}_body() { \
		RUMP_SOCKETS_LIST=\${RUMP_SOCKET}; \
		export RUMP_SERVER=unix://\${RUMP_SOCKET}; \
		${check_function} " "${@}" "; \
	}"
	tests="${tests} ${name}"
}

atf_init_test_cases()
{
	IMG=fsimage
	DIR=target
	RUMP_SOCKET=test;
	for i in ${tests}; do
		atf_add_test_case $i
	done
}
