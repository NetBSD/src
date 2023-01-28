# $NetBSD: nanpa.awk,v 1.3 2023/01/28 13:12:16 jmcneill Exp $
#
# todo:
#	parse "https://nationalnanpa.com/nanp1/npa_report.csv"
#	    instead of scraping HTML.
#
function trim(s)
{
	gsub(/^[ \t]+|[ \t]+$/, "", s);
	return s;
}
function mapinit(postdb)
{
	while ((getline < postdb) > 0) {
		sub(/#.*/, "");
		if (length($0)==0) continue;
		NF=split($0, f);
		location[f[1]] = f[2];
		flocation[tolower(f[2])] = f[2];
		country[f[1]] = f[4];
		fcountry[tolower(f[2])] = f[4];
	}
}
function countrymap(s)
{
	if (s == "CA") return "Canada";
	if (s == "US") return "USA";
	return s;
}
function locationmap(s,	t)
{
	if (s in location) {
		t = location[s];
		if (s in country) {
			t = t " (" countrymap(country[s]) ")";
		}
	} else if (tolower(s) in flocation) {
		t = flocation[tolower(s)];
		if (tolower(s) in fcountry) {
			t = t " (" countrymap(fcountry[tolower(s)]) ")";
		}
	} else {
		t = s;
	}
	return t;
}
function parse(file, ispipe, isplanning,	i, planinit, t)
{
	planinit = 0;
	while((ispipe?(file | getline):(getline < file)) > 0) {
		sub(/#.*/, "");
		if (length($0)==0) continue;
		if (isplanning) {
			NF=split($0, f);
			if (!planinit && f[2]=="New NPA") {
				planinit=1;
				for (i=1; i<=NF; i++)
					fnames[f[i]]=i-1;
			} else if (planinit && length(f[fnames["New NPA"]])>1) {
				t = locationmap(trim(f[fnames["Location"]])) FS;
				if (trim(f[fnames["Overlay?"]])=="Yes")
				  t = t "Overlay of " trim(f[fnames["Old NPA"]]);
				else if (f[fnames["Old NPA"]])
				  t = t "Split of " trim(f[fnames["Old NPA"]]);
				if (f[fnames["Status"]])
					t = t " (" trim(f[fnames["Status"]]) ")";
				if (length(f[fnames["In Service Date"]]) > 1)
					t = t " effective " \
					    trim(f[fnames["In Service Date"]]);
				data[trim(f[fnames["New NPA"]]) "*"] = t;
			}
		} else {
			# digits only
			match($0, /^[0-9]/);
			if (RSTART==0) continue;
			i=index($0, FS);
			data[substr($0, 1, i-1)]=locationmap(trim(substr($0,i+1)));
		}
	}
	close(file);
}

BEGIN{
	FS=":"
	mapinit("na.postal");
	print "# $""NetBSD: $";
	print "# Generated from https://nationalnanpa.com/area_codes/index.html";
	print "# (with local exceptions)";
	print "# ";
	print "# format:";
	print "#   Area Code : Description : Detail : State/Province Abbrev.";
	print "#   (3rd and 4th fields optional)";
	print "#   A * in the Area Code field indicates a future area code."
	print "# ";
	parse("ftp -o - " \
	    "https://nationalnanpa.com/enas/geoAreaCodeNumberReport.do" \
	    " | sed -f nanpa.sed", 1, 0);
	parse("ftp -o - " \
	    "https://nationalnanpa.com/enas/nonGeoNpaServiceReport.do" \
	    " | sed -f nanpa.sed", 1, 0);
	parse("ftp -o - " \
	    "https://nationalnanpa.com/enas/plannedNpasNotInServiceReport.do" \
	    " | sed -f nanpa.sed", 1, 1);
	parse("na.phone.add", 0, 0);
	sort="sort -n";
	for (i in data)
		print i FS data[i] | sort
	close(sort);
}
