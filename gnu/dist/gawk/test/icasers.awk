BEGIN { RS = "[[:upper:]]+" }
{ print ; IGNORECASE = ! IGNORECASE }
