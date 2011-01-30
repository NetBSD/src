# $NetBSD: quotas_common.sh,v 1.1.2.3 2011/01/30 13:23:26 bouyer Exp $ 

create_with_quotas()
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
	atf_check -o ignore -e ignore $(atf_get_srcdir)/h_quota2_server -b \
		${IMG} ${RUMP_SERVER}
}

# from tests/ipf/h_common.sh via tests/sbin/resize_ffs
test_case()
{
	local name="${1}"; shift
	local check_function="${1}"; shift
	local descr="${1}"; shift
	
	atf_test_case "${name}" cleanup

	eval "${name}_head() { \
		atf_set "descr" "${descr}"
		atf_set "timeout" "60"
	}"
	eval "${name}_body() { \
		${check_function} " "${@}" "; \
	}"
	eval "${name}_cleanup() { \
		atf_check -s exit:1 -o ignore -e ignore rump.halt; \
	}"
	tests="${tests} ${name}"
}

test_case_root()
{
	local name="${1}"; shift
	local check_function="${1}"; shift
	local descr="${1}"; shift
	
	atf_test_case "${name}" cleanup

	eval "${name}_head() { \
		atf_set "descr" "${descr}"
		atf_set "require.user" "root"
		atf_set "timeout" "60"
	}"
	eval "${name}_body() { \
		${check_function} " "${@}" "; \
	}"
	eval "${name}_cleanup() { \
		atf_check -s exit:1 -o ignore -e ignore rump.halt; \
	}"
	tests="${tests} ${name}"
}

atf_init_test_cases()
{
	IMG=fsimage
	DIR=target
	RUMP_SOCKET=test
	RUMP_SERVER=unix://${RUMP_SOCKET}
	export RUMP_SERVER
	for i in ${tests}; do
		atf_add_test_case $i
	done
}
