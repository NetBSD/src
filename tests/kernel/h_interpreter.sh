#!/bin/sh

cmd=$1
shift
case "${cmd}" in
interpreter)
	"${1}" $$
	;;
dot)
	cp "${1}" .
	z="./$(basename "${1}")"
	x=$(${z} -1)
	case ${x} in
	/*)	x=$(readlink "${x}");;
	*)	echo "non absolute path" 1>&2; exit 1;;
	esac

	e=$(readlink "$(/bin/pwd)/${z}")
	if [ "${x}" != "${e}" ]; then
		echo bad: ${x} != ${e} 1>&2
		exit 1
	fi
	;;
*)
	echo bad command ${cmd}
esac
		
