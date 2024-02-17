# nsd-control(1) completion                              -*- shell-script -*-

_nsdcontrol()
{
    local cur prev words cword
    _init_completion || return

    local WORDS

    case $prev in
        assoc_tsig|\
        changezone|\
        delzone|\
        force_transfer|\
        notify|\
        reload|\
        transfer|\
        write|\
        zonestatus)
            WORDS=$($1 zonestatus |awk '/zone:/ {print $2}' ORS=' ')
            COMPREPLY=($(compgen -W "$WORDS" -- "$cur"))
            return 0
            ;;
    esac

    if [[ $cur == -* ]]; then
        WORDS=$($1 |awk '/^  -/ {print $1}' ORS=' ')
    elif ((cword == 1)); then
        WORDS=$($1 |awk '/^  [^-]/ {print $1}' ORS=' ')
    fi
    COMPREPLY=($(compgen -W "$WORDS" -- "$cur"))
} &&
    complete -F _nsdcontrol nsd-control

# ex: filetype=sh
