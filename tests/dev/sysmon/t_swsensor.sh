# $NetBSD: t_swsensor.sh,v 1.1 2010/12/20 04:56:18 pgoyette Exp $

get_sensor_info() {
	rump.envstat -x | \
	sed -e "\;swsensor;,\;/array;p" -e "d"
}

get_sensor_key() {
	get_sensor_info | grep -A1 $1 | grep integer | sed -e 's;<[/a-z]*>;;g'
}

get_powerd_event_count() {
	grep "not running" powerd.log | wc -l
}

check_powerd_event() {
	event=$(grep "not running" powerd.log | \
		sed -e "$1p" -e "d" )
	event=${event##*//}
	script=${event%% *}
	event=${event#* }
	device=${event%% *}
	event=${event#* }
	state=${event%% *}
	sensor=${event#* }
	sensor=${sensor% *}

	if [ "${script}" != "sensor_indicator" ] ; then
		echo "Event uses wrong script: ${script}"
	elif [ "${device}" != "swsensor" ] ; then
		echo "Event uses wrong device: ${device}"
	elif [ "${sensor}" != "sensor" ] ; then
		echo "Event uses wrong sensor: ${sensor}"
	elif [ "${state}" != "$2" ] ; then
		echo "Event uses wrong state: ${state}"
	fi
}

# Start the rump server, then load the swsensor module with the
# requested properties

start_rump() {
	rump_allserver ${RUMP_SERVER}
	if [ $( get_sensor_info | wc -l ) -ne 0 ] ; then
		rump.modunload swsensor
		rump.modload -f $1 swsensor
	else
		rump.modload $1 swsensor
	fi
	return $?
}

common_head() {
	atf_set	descr		"$1"
	atf_set	timeout		60
	atf_set	require.progs	rump.powerd rump.envstat rump.modload   \
				rump.halt   rump.sysctl  rump_allserver \
				sed         grep
	atf_set	require.user	root
}

common_cleanup() {
	if [ -n "$(jobs)" ] ; then
		kill %1			# get rid of our rump.powerd
	fi

	rump.modunload swsensor
	rump.halt
	rm -f ${RUMP_SERVER}
}

create_envsys_conf_files() {
	cat << ENV0 > env0.conf
	swsensor {
		refresh-timeout = 2s;
	}
ENV0
	cat << ENV1 > env1.conf
	swsensor {
		sensor0 { critical-min = $(( $1 - $2 )); }
	}
ENV1
	cat << ENV2 > env2.conf
	swsensor {
		sensor0 { critical-min = $1; }
	}
ENV2
}

# Test body common to all sensors
#	$1	sensor mode
#	$2	initial sensor value
#	$3	initial limit
#	$4	amount to lower limit
#	$5	difference from limit to trigger event

common_body() {
	if [ $1 -ne 0 ] ; then
		atf_skip "rump.modload doesn't pass proplist attributes"
	fi

	# Start the rump-server process and load the module
	start_rump "-i mode=$1 -i value=$2 -i limit=$3"

	# create configuration files for updates
	create_envsys_conf_files $3 $4

	if [ $? -ne 0 ] ; then
		atf_skip "Cannot set-up rump environment"
	fi

	# start powerd so we can detect sensor events
	rump.powerd -n -d > powerd.log 2>&1 &
	if [ -z "$(jobs)" ] ; then
		skip_events=1
		echo "Skipping event sub-tests - powerd did not start"
	else
		skip_events=0
		expected_event=1
	fi

	# Step 0 - verify that sensor is registered
	get_sensor_info | grep -q swsensor ||
		atf_fail "0: Device swsensor not registered"

	# Step 1 - update the refresh-timeout and verify
	# (use $(( ... )) since the timeout is displayed in hex!)
	rump.envstat -c env0.conf
	if [ $(( $( get_sensor_key refresh-timeout ) )) -ne 2 ] ; then
		atf_fail "1: Could not set refresh-timout to 2s"
	fi

	# Step 2 - verify that we can read sensor's value
	if [ $1 -ne 0 -a $( get_sensor_key cur-value ) -ne $2 ] ; then
		atf_fail "2: Value not available"
	fi

	# Step 3 - verify that changes in sensor value are seen
	rump.sysctl -w hw.swsensor.cur_value=$(( $2 + 1 ))
	if [ $( get_sensor_key cur-value ) -ne $(( $2 + 1 )) ] ; then
		atf_fail "3: Value not updated"
	fi

	# Step 4 - if sensor provides hw limit, make sure we can read it
	if [ $1 -ne 0 ] ; then
		if [ $( get_sensor_key critical-min ) -ne $3 ] ; then
			atf_fail "4: Limit not set by device"
		fi
	fi

	# Step 5 - if sensor provides hw limit, make sure it works
	if [ $1 -ne 0 -a ${skip_events} -eq 0 ] ; then
		rump.sysctl -w hw.swsensor.cur_value=$(( $3 - $5 ))
		sleep 5
		cnt=$(get_powerd_event_count)
		if [ ${cnt} -lt ${expected_event} ] ; then
			atf_fail "5: No event triggered"
		elif [ ${cnt} -gt ${expected_event} ] ; then
			atf_fail "5: Multiple events triggered"
		fi
		evt=$( check_powerd_event ${cnt} "critical-under")
		if [ -n "${evt}" ] ; then
			atf_fail "5: ${evt}"
		fi
		expected_event=$(( 1 + ${expected_event} ))
	fi

	# Step 6 - verify that we return to normal state
	if [ $1 -ne 0 -a ${skip_events} -eq 0 ] ; then
		rump.sysctl -w hw.swsensor.cur_value=$(( $3 + $5 ))
		sleep 5
		cnt=$(get_powerd_event_count)
		if [ ${cnt} -lt ${expected_event} ] ; then
			atf_fail "6: No event triggered"
		elif [ ${cnt} -gt ${expected_event} ] ; then
			atf_fail "6: Multiple events triggered"
		fi
		evt=$( check_powerd_event ${cnt} "normal")
		if [ -n "${evt}" ] ; then
			atf_fail "6: ${evt}"
		fi
		expected_event=$(( 1 + ${expected_event} ))
	fi

	# Step 7 - verify that we can set our own limit
	rump.envstat -c env1.conf
	if [ $( get_sensor_key critical-min ) -ne $(( $3 - $4 )) ] ; then
		atf_fail "7: Limit not set by envstat -c"
	fi

	# Step 8 - make sure user-set limit works
	if [ ${skip_events} -eq 0 ] ; then
		rump.sysctl -w hw.swsensor.cur_value=$(( $3 - $4 - $5 ))
		sleep 5
		cnt=$(get_powerd_event_count)
		if [ ${cnt} -lt ${expected_event} ] ; then
			atf_fail "8: No event triggered"
		elif [ ${cnt} -gt ${expected_event} ] ; then
			atf_fail "8: Multiple events triggered"
		fi
		evt=$( check_powerd_event ${cnt} "critical-under")
		if [ -n "${evt}" ] ; then
			atf_fail "8: ${evt}"
		fi
		expected_event=$(( 1 + ${expected_event} ))
	fi

	# Step 9 - verify that we return to normal state
	if [ ${skip_events} -eq 0 ] ; then
		rump.sysctl -w hw.swsensor.cur_value=$(( $3 - $4 + $5 ))
		sleep 5
		cnt=$(get_powerd_event_count)
		if [ ${cnt} -lt ${expected_event} ] ; then
			atf_fail "9: No event triggered"
		elif [ ${cnt} -gt ${expected_event} ] ; then
			atf_fail "9: Multiple events triggered"
		fi
		evt=$( check_powerd_event ${cnt} "normal")
		if [ -n "${evt}" ] ; then
			atf_fail "9: ${evt}"
		fi
		expected_event=$(( 1 + ${expected_event} ))
	fi

	# Step 10 - reset to defaults
	rump.envstat -S
	if [ $1 -eq 0 ] ; then
		get_sensor_info | grep -q critical-min &&
			atf_fail "10: Failed to clear a limit with envstat -S"
	else
		if [ $( get_sensor_key critical-min ) -ne $3 ] ; then
			atf_fail "10: Limit not reset to initial value"
		fi
	fi

	# Step 11 - see if more events occur
	if [ ${skip_events} -eq 0 ] ; then
		rump.envstat -c env0.conf
		rump.sysctl -w hw.swsensor.cur_value=$(( $3 - $4 - $5 ))
		sleep 5
		cnt=$(get_powerd_event_count)
		if [ ${cnt} -ge ${expected_event} ] ; then
			atf_fail "9: Event triggered after reset"
		fi
	fi

	# Step 12 - make sure we can set new limits once more
	rump.envstat -c env2.conf
	if [ $( get_sensor_key critical-min ) -ne $3 ] ; then
		atf_fail "12a: Limit not reset to same value"
	fi
	rump.envstat -c env1.conf
	if [ $( get_sensor_key critical-min ) -ne $(( $3 - $4 )) ] ; then
		atf_fail "12b: Limit not reset to new value"
	fi
}

atf_test_case simple_sensor cleanup
simple_sensor_head() {
	common_head "Test a simple sensor"
}

simple_sensor_body() {
	common_body 0 50 30 10 1
}

simple_sensor_cleanup() {
	common_cleanup
}

atf_test_case limit_sensor cleanup
limit_sensor_head() {
	common_head "Test a sensor with internal limit"
}

limit_sensor_body() {
	common_body 1 45 25 8 2
}

limit_sensor_cleanup() {
	common_cleanup
}

atf_test_case alarm_sensor cleanup
alarm_sensor_head() {
	common_head "Test a sensor with internal checking"
}

alarm_sensor_body() {
	common_body 2 40 20 6 3
}

alarm_sensor_cleanup() {
	common_cleanup
}

atf_init_test_cases() {
	RUMP_SERVER="unix:///tmp/t_swsensor" ; export RUMP_SERVER
	atf_add_test_case simple_sensor
	atf_add_test_case limit_sensor
	atf_add_test_case alarm_sensor
}
