#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <dev/usb/usb.h>

#define USBDEV "/dev/usb"

int verbose;

void usage __P((void));
void usbdev __P((int f, int a, int rec));
void usbdump __P((int f));
void dumpone __P((char *name, int f, int addr));
int main __P((int, char **));

void
usage()
{
	extern char *__progname;

	fprintf(stderr, "Usage: %s [-a addr] [-f dev] [-v]\n", __progname);
	exit(1);
}

char done[USB_MAX_DEVICES];
int indent;

void
usbdev(f, a, rec)
	int f;
	int a;
	int rec;
{
	struct usb_device_info di;
	int e, p;

	di.addr = a;
	e = ioctl(f, USB_DEVICEINFO, &di);
	if (e)
		return;
	done[a] = 1;
	printf("addr %d: ", di.addr);
	if (verbose) {
		if (di.lowspeed)
			printf("low speed, ");
		if (di.power)
			printf("power %d mA, ", di.power);
		else
			printf("self powered, ");
		if (di.config)
			printf("config %d, ", di.config);
		else
			printf("unconfigured, ");
	}
	printf("%s, %s", di.product, di.vendor);
	if (verbose)
		printf(", rev %s", di.revision);
	printf("\n");
	if (!rec)
		return;
	for (p = 0; p < di.nports; p++) {
		int s = di.ports[p];
		if (s >= USB_MAX_DEVICES) {
			if (verbose) {
				printf("%*sport %d %s\n", indent+1, "", p+1,
				       s == USB_PORT_ENABLED ? "enabled" :
				       s == USB_PORT_SUSPENDED ? "suspended" : 
				       s == USB_PORT_POWERED ? "powered" :
				       s == USB_PORT_DISABLED ? "disabled" :
				       "???");
				
			}
			continue;
		}
		indent++;
		printf("%*s", indent, "");
		if (verbose)
			printf("port %d ", p+1);
		usbdev(f, di.ports[p], 1);
		indent--;
	}
}

void
usbdump(f)
	int f;
{
	int a;

	for (a = 1; a < USB_MAX_DEVICES; a++) {
		if (!done[a])
			usbdev(f, a, 1);
	}
}

void
dumpone(name, f, addr)
	char *name;
	int f;
	int addr;
{
	if (verbose)
		printf("Controller %s:\n", name);
	indent = 0;
	memset(done, 0, sizeof done);
	if (addr)
		usbdev(f, addr, 0);
	else
		usbdump(f);
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch, i, f;
	char buf[50];
	extern int optind;
	extern char *optarg;
	char *dev = 0;
	int addr = 0;

	while ((ch = getopt(argc, argv, "a:f:v")) != -1) {
		switch(ch) {
		case 'a':
			addr = atoi(optarg);
			break;
		case 'f':
			dev = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (dev == 0) {
		for (i = 0; i < 10; i++) {
			sprintf(buf, "%s%d", USBDEV, i);
			f = open(buf, O_RDONLY);
			if (f >= 0) {
				dumpone(buf, f, addr);
				close(f);
			}
		}
	} else {
		f = open(dev, O_RDONLY);
		if (f >= 0)
			dumpone(dev, f, addr);
		else
			err(1, "%s", dev);
	}
	exit(0);
}
