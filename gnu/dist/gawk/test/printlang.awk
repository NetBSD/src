BEGIN {
	printf "\nLocale environment:\n\tLC_ALL=\"%s\" LANG=\"%s\"\n\n",
		ENVIRON["LC_ALL"], ENVIRON["LANG"]
}
