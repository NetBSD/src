# $NetBSD: t_mixerctl.sh,v 1.9 2017/07/25 21:25:03 kre Exp $

audio_setup() {
	# Open /dev/pad0 so we have a configured audio device.
	# Do it in a way that guarantees the open happens before
	# we proceed to the (</dev/mixer) below (do not expect
	# cat to be running in time to open the device.)

	# Note: it is not important that the open of pad0 succeeds,
	# if there is real audio hardware on the system, that can (will)
	# be used instead, and having pad0 open is irrelevant.
	# So, no errors reported if pad0 open fails.  If there turns
	# out to be no audio device of any kind, then the open of the
	# mixer will fail, causing the test to be skipped.

	# Saving padpid and later killing it seems to be unnecessary,
	# ATF appears to killpg() the process after the test finishes
	# which is a good thing, otherwise a test that is skipped/fails
	# would not kill the cat (doing it in a cleanup function is not
	# convenient as it is a different execution environment, no shared
	# variables, we would need to put $padpid in a file.)

	unset padpid
	test -r /dev/pad0 && 
	    { { cat >/dev/null & } < /dev/pad0 ; } 2>/dev/null && padpid=$!

	(</dev/mixer) >/dev/null 2>&1 ||
	    atf_skip "no audio mixer available in kernel"
}

atf_test_case noargs_usage
noargs_usage_head() {
	atf_set "descr" "Ensure mixerctl(1) with no args prints a usage message"
}
noargs_usage_body() {
	audio_setup

	atf_check -s exit:0 -o not-empty -e ignore \
		mixerctl

	${padpid+kill -HUP ${padpid}} 2>/dev/null || :
}

atf_test_case showvalue
showvalue_head() {
	atf_set "descr" "Ensure mixerctl(1) can print the value for all variables"
}
showvalue_body() {
	audio_setup

	for var in $(mixerctl -a | awk -F= '{print $1}'); do
		atf_check -s exit:0 -e ignore -o match:"^${var}=" \
			mixerctl ${var}
	done

	${padpid+kill -HUP ${padpid}} 2>/dev/null || :
}

atf_test_case nflag
nflag_head() {
	atf_set "descr" "Ensure 'mixerctl -n' actually suppresses some output"
}
nflag_body() {
	audio_setup

	varname="$(mixerctl -a | sed -e 's/=.*//' -e q)"

	atf_check -s exit:0 -o match:"${varname}" -e ignore \
		mixerctl ${varname}

	atf_check -s exit:0 -o not-match:"${varname}" -e ignore \
		mixerctl -n ${varname}

	${padpid+kill -HUP ${padpid}} 2>/dev/null || :
}

atf_test_case nonexistant_device
nonexistant_device_head() {
	atf_set "descr" "Ensure mixerctl(1) complains if provided a nonexistant mixer device"
}
nonexistant_device_body() {
	atf_check -s not-exit:0  -o ignore -e match:"No such file" \
		mixerctl -a -d /a/b/c/d/e
}

atf_init_test_cases() {
	atf_add_test_case noargs_usage
	atf_add_test_case showvalue
	atf_add_test_case nflag
	atf_add_test_case nonexistant_device
}
