/* $NetBSD: skeytest.c,v 1.2 2000/07/31 12:22:39 mjl Exp $ */

/* This is a regression test for the S/Key implementation
	against the data set from Appendix C of RFC2289 */

#include <stdio.h>
#include <string.h>
#include "skey.h"

struct regRes {
	char *algo, *zero, *one, *nine;
	};

struct regPass {
	char *passphrase, *seed;
	struct regRes res[4];
	} regPass[] = {
		{ "This is a test.", "TeSt", { 
			{ "md4", "D1854218EBBB0B51", "63473EF01CD0B444", "C5E612776E6C237A" }, 
			{ "md5", "9E876134D90499DD", "7965E05436F5029F", "50FE1962C4965880" },
			{ "sha1","BB9E6AE1979D8FF4", "63D936639734385B", "87FEC7768B73CCF9" },
			{ NULL } } },
		{ "AbCdEfGhIjK", "alpha1", { 
			{ "md4", "50076F47EB1ADE4E", "65D20D1949B5F7AB", "D150C82CCE6F62D1" }, 
			{ "md5", "87066DD9644BF206", "7CD34C1040ADD14B", "5AA37A81F212146C" },
			{ "sha1","AD85F658EBE383C9", "D07CE229B5CF119B", "27BC71035AAF3DC6" },
			{ NULL } } },
		{ "OTP's are good", "correct", { 
			{ "md4", "849C79D4F6F55388", "8C0992FB250847B1", "3F3BF4B4145FD74B" },
			{ "md5", "F205753943DE4CF9", "DDCDAC956F234937", "B203E28FA525BE47" },
			{ "sha1","D51F3E99BF8E6F0B", "82AEB52D943774E4", "4F296A74FE1567EC" },
			{ NULL } } },
		{ NULL }
	};

int main()
	{
	char data[16], prn[64];
	struct regPass *rp;
	int i = 0;
	int errors = 0;
	int j;
	
	for(rp = regPass; rp->passphrase; rp++)
		{
		struct regRes *rr;

		i++;
		for(rr = rp->res; rr->algo; rr++)
			{
			skey_set_algorithm(rr->algo);

			keycrunch(data, rp->seed, rp->passphrase);
			btoa8(prn, data);

			if(strcasecmp(prn, rr->zero))
				{
				errors++;
				printf("Set %d, round 0, %s: Expected %s and got %s\n", i, rr->algo, rr->zero, prn);
				}

			f(data);
			btoa8(prn, data);

			if(strcasecmp(prn, rr->one))
				{
				errors++;
				printf("Set %d, round 1, %s: Expected %s and got %s\n", i, rr->algo, rr->one, prn);
				}

			for(j=1; j<99; j++)
				f(data);
			btoa8(prn, data);

			if(strcasecmp(prn, rr->nine))
				{
				errors++;
				printf("Set %d, round 99, %s: Expected %s and got %s\n", i, rr->algo, rr->nine, prn);
				}

			}
		}

	printf("%d errors\n", errors);
	return(errors ? 1 : 0);
	}
