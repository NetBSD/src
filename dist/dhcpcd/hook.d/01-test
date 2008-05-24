# Just echo our DHCP options we have

if [ "${reason}" = "TEST" ]; then
	env | grep "^\(interface\|pid\|reason\|skip_hooks\)="
	env | grep "^\(new_\|old_\)" | sort
fi
