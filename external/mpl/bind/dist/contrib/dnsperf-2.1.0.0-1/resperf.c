/*	$NetBSD: resperf.c,v 1.2 2018/08/12 13:02:32 christos Exp $	*/

/*
 * Copyright (C) 2000, 2001  Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (C) 2004 - 2015 Nominum, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND NOMINUM DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL NOMINUM BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/***
 ***	DNS Resolution Performance Testing Tool
 ***/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>

#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/list.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/types.h>

#include <dns/result.h>

#include "datafile.h"
#include "dns.h"
#include "log.h"
#include "net.h"
#include "opt.h"
#include "util.h"
#include "version.h"

/*
 * Global stuff
 */

#define DEFAULT_SERVER_NAME             "127.0.0.1"
#define DEFAULT_SERVER_PORT             53
#define DEFAULT_LOCAL_PORT              0
#define DEFAULT_SOCKET_BUFFER		32
#define DEFAULT_TIMEOUT                 45
#define DEFAULT_MAX_OUTSTANDING		(64 * 1024)

#define MAX_INPUT_DATA                  (4 * 1024)

struct query_info;

typedef ISC_LIST(struct query_info) query_list;

typedef struct query_info {
	isc_uint64_t sent_timestamp;
	/*
	 * This link links the query into the list of outstanding
	 * queries or the list of available query IDs.
	 */
	ISC_LINK(struct query_info) link;
	/*
	 * The list this query is on.
	 */
	query_list *list;
} query_info;

static query_list outstanding_list;
static query_list instanding_list;

static query_info *queries;

static isc_mem_t *mctx;

static isc_sockaddr_t server_addr;
static isc_sockaddr_t local_addr;
static unsigned int nsocks;
static int *socks;

static isc_uint64_t query_timeout;
static isc_boolean_t edns;
static isc_boolean_t dnssec;

static perf_datafile_t *input;

/* The target traffic level at the end of the ramp-up */
double max_qps = 100000.0;

/* The time period over which we ramp up traffic */
#define DEFAULT_RAMP_TIME 60
static isc_uint64_t ramp_time;

/* How long to send constant traffic after the initial ramp-up */
#define DEFAULT_SUSTAIN_TIME 0
static isc_uint64_t sustain_time;

/* How long to wait for responses after sending traffic */
static isc_uint64_t wait_time = 40 * MILLION;

/* Total duration of the traffic-sending part of the test */
static isc_uint64_t traffic_time;

/* Total duration of the test */
static isc_uint64_t end_time;

/* Interval between plot data points, in microseconds */
#define DEFAULT_BUCKET_INTERVAL 0.5
static isc_uint64_t bucket_interval;

/* The number of plot data points */
static int n_buckets;

/* The plot data file */
static const char *plotfile = "resperf.gnuplot";

/* The largest acceptable query loss when reporting max throughput */
static double max_loss_percent = 100.0;

/* The maximum number of outstanding queries */
static unsigned int max_outstanding;

static isc_uint64_t num_queries_sent;
static isc_uint64_t num_queries_outstanding;
static isc_uint64_t num_responses_received;
static isc_uint64_t num_queries_timed_out;
static isc_uint64_t rcodecounts[16];

static isc_uint64_t time_now;
static isc_uint64_t time_of_program_start;
static isc_uint64_t time_of_end_of_run;

/*
 * The last plot data point containing actual data; this can
 * be less than than (n_buckets - 1) if the traffic sending
 * phase is cut short
 */
static int last_bucket_used;

/*
 * The statistics for queries sent during one bucket_interval
 * of the traffic sending phase.
 */
typedef struct {
	int queries;
	int responses;
	int failures;
	double latency_sum;
} ramp_bucket;

/* Pointer to array of n_buckets ramp_bucket structures */
static ramp_bucket *buckets;

enum phase {
	/*
	 * The ramp-up phase: we are steadily increasing traffic.
	 */
	PHASE_RAMP,
	/*
	 * The sustain phase: we are sending traffic at a constant
	 * rate.
	 */
	PHASE_SUSTAIN,
	/*
	 * The wait phase: we have stopped sending queries and are
	 * just waiting for any remaining responses.
	 */
	PHASE_WAIT
};
static enum phase phase = PHASE_RAMP;

/* The time when the sustain/wait phase began */
static isc_uint64_t sustain_phase_began, wait_phase_began;

static perf_dnstsigkey_t *tsigkey;

static char *
stringify(double value, int precision)
{
	static char buf[20];

	snprintf(buf, sizeof(buf), "%.*f", precision, value);
	return buf;
}

static void
setup(int argc, char **argv)
{
	const char *family = NULL;
	const char *server_name = DEFAULT_SERVER_NAME;
	in_port_t server_port = DEFAULT_SERVER_PORT;
	const char *local_name = NULL;
	in_port_t local_port = DEFAULT_LOCAL_PORT;
	const char *filename = NULL;
	const char *tsigkey_str = NULL;
	int sock_family;
	unsigned int bufsize;
	unsigned int i;
	isc_result_t result;

	result = isc_mem_create(0, 0, &mctx);
	if (result != ISC_R_SUCCESS)
		perf_log_fatal("creating memory context: %s",
			       isc_result_totext(result));

	dns_result_register();

	sock_family = AF_UNSPEC;
	server_port = DEFAULT_SERVER_PORT;
	local_port = DEFAULT_LOCAL_PORT;
	bufsize = DEFAULT_SOCKET_BUFFER;
	query_timeout = DEFAULT_TIMEOUT * MILLION;
	ramp_time = DEFAULT_RAMP_TIME * MILLION;
	sustain_time = DEFAULT_SUSTAIN_TIME * MILLION;
	bucket_interval = DEFAULT_BUCKET_INTERVAL * MILLION;
	max_outstanding = DEFAULT_MAX_OUTSTANDING;
	nsocks = 1;

	perf_opt_add('f', perf_opt_string, "family",
		     "address family of DNS transport, inet or inet6", "any",
		     &family);
	perf_opt_add('s', perf_opt_string, "server_addr",
		     "the server to query", DEFAULT_SERVER_NAME, &server_name);
	perf_opt_add('p', perf_opt_port, "port",
		     "the port on which to query the server",
		     stringify(DEFAULT_SERVER_PORT, 0), &server_port);
	perf_opt_add('a', perf_opt_string, "local_addr",
		     "the local address from which to send queries", NULL,
		     &local_name);
	perf_opt_add('x', perf_opt_port, "local_port",
		     "the local port from which to send queries",
		     stringify(DEFAULT_LOCAL_PORT, 0), &local_port);
	perf_opt_add('d', perf_opt_string, "datafile",
		     "the input data file", "stdin", &filename);
	perf_opt_add('t', perf_opt_timeval, "timeout",
		     "the timeout for query completion in seconds",
		     stringify(DEFAULT_TIMEOUT, 0), &query_timeout);
	perf_opt_add('b', perf_opt_uint, "buffer_size",
		     "socket send/receive buffer size in kilobytes", NULL,
		     &bufsize);
	perf_opt_add('e', perf_opt_boolean, NULL,
		     "enable EDNS 0", NULL, &edns);
	perf_opt_add('D', perf_opt_boolean, NULL,
		     "set the DNSSEC OK bit (implies EDNS)", NULL, &dnssec);
	perf_opt_add('y', perf_opt_string, "[alg:]name:secret",
		     "the TSIG algorithm, name and secret", NULL,
		     &tsigkey_str);
	perf_opt_add('i', perf_opt_timeval, "plot_interval",
		     "the time interval between plot data points, in seconds",
		     stringify(DEFAULT_BUCKET_INTERVAL, 1), &bucket_interval);
	perf_opt_add('m', perf_opt_double, "max_qps",
		     "the maximum number of queries per second",
		     stringify(max_qps, 0), &max_qps);
	perf_opt_add('P', perf_opt_string, "plotfile",
		     "the name of the plot data file", plotfile, &plotfile);
	perf_opt_add('r', perf_opt_timeval, "ramp_time",
		     "the ramp-up time in seconds",
		     stringify(DEFAULT_RAMP_TIME, 0), &ramp_time);
	perf_opt_add('c', perf_opt_timeval, "constant_traffic_time",
		     "how long to send constant traffic, in seconds",
		     stringify(DEFAULT_SUSTAIN_TIME, 0), &sustain_time);
	perf_opt_add('L', perf_opt_double, "max_query_loss",
		     "the maximum acceptable query loss, in percent",
		     stringify(max_loss_percent, 0), &max_loss_percent);
	perf_opt_add('C', perf_opt_uint, "clients",
		     "the number of clients to act as", NULL,
		     &nsocks);
	perf_opt_add('q', perf_opt_uint, "num_outstanding",
		     "the maximum number of queries outstanding",
		     stringify(DEFAULT_MAX_OUTSTANDING, 0), &max_outstanding);

	perf_opt_parse(argc, argv);

	if (max_outstanding > nsocks * DEFAULT_MAX_OUTSTANDING)
		perf_log_fatal("number of outstanding packets (%u) must not "
			       "be more than 64K per client", max_outstanding);

        if (ramp_time + sustain_time == 0)
            perf_log_fatal("rampup_time and constant_traffic_time must not "
                           "both be 0");

	ISC_LIST_INIT(outstanding_list);
	ISC_LIST_INIT(instanding_list);
	queries = isc_mem_get(mctx, max_outstanding * sizeof(query_info));
	if (queries == NULL)
		perf_log_fatal("out of memory");
	for (i = 0; i < max_outstanding; i++) {
		ISC_LINK_INIT(&queries[i], link);
		ISC_LIST_APPEND(instanding_list, &queries[i], link);
		queries[i].list = &instanding_list;
	}


	if (family != NULL)
		sock_family = perf_net_parsefamily(family);
	perf_net_parseserver(sock_family, server_name, server_port,
			     &server_addr);
	perf_net_parselocal(isc_sockaddr_pf(&server_addr),
			    local_name, local_port, &local_addr);

	input = perf_datafile_open(mctx, filename);

	if (dnssec)
		edns = ISC_TRUE;

	if (tsigkey_str != NULL)
		tsigkey = perf_dns_parsetsigkey(tsigkey_str, mctx);

	socks = isc_mem_get(mctx, nsocks * sizeof(int));
	if (socks == NULL)
		perf_log_fatal("out of memory");
	for (i = 0; i < nsocks; i++)
		socks[i] = perf_net_opensocket(&server_addr, &local_addr, i,
					       bufsize);

}

static void
cleanup(void)
{
	unsigned int i;

	perf_datafile_close(&input);
	for (i = 0; i < nsocks; i++)
		(void) close(socks[i]);
	isc_mem_put(mctx, socks, nsocks * sizeof(int));
	isc_mem_put(mctx, queries, max_outstanding * sizeof(query_info));
	isc_mem_put(mctx, buckets, n_buckets * sizeof(ramp_bucket));
}

/* Find the ramp_bucket for queries sent at time "when" */

static ramp_bucket *
find_bucket(isc_uint64_t when) {
	isc_uint64_t sent_at = when - time_of_program_start;
	int i = (int) ((n_buckets * sent_at) / traffic_time);
	/*
	 * Guard against array bounds violations due to roundoff
	 * errors or scheduling jitter
	 */
	if (i < 0)
		i = 0;
	if (i > n_buckets - 1)
		i = n_buckets - 1;
	return &buckets[i];
}

/*
 * print_statistics:
 *   Print out statistics based on the results of the test
 */
static void
print_statistics(void) {
	int i;
	double max_throughput;
	double loss_at_max_throughput;
	isc_boolean_t first_rcode;
	isc_uint64_t run_time = time_of_end_of_run - time_of_program_start;

	printf("\nStatistics:\n\n");

	printf("  Queries sent:         %" ISC_PRINT_QUADFORMAT "u\n",
               num_queries_sent);
	printf("  Queries completed:    %" ISC_PRINT_QUADFORMAT "u\n",
               num_responses_received);
	printf("  Queries lost:         %" ISC_PRINT_QUADFORMAT "u\n",
	       num_queries_sent - num_responses_received);
	printf("  Response codes:       ");
	first_rcode = ISC_TRUE;
	for (i = 0; i < 16; i++) {
		if (rcodecounts[i] == 0)
			continue;
		if (first_rcode)
			first_rcode = ISC_FALSE;
		else
			printf(", ");
		printf("%s %" ISC_PRINT_QUADFORMAT "u (%.2lf%%)",
		       perf_dns_rcode_strings[i], rcodecounts[i],
		       (rcodecounts[i] * 100.0) / num_responses_received);
	}
	printf("\n");
	printf("  Run time (s):         %u.%06u\n",
	       (unsigned int)(run_time / MILLION),
	       (unsigned int)(run_time % MILLION));

	/* Find the maximum throughput, subject to the -L option */
	max_throughput = 0.0;
	loss_at_max_throughput = 0.0;
	for (i = 0; i <= last_bucket_used; i++) {
		ramp_bucket *b = &buckets[i];
		double responses_per_sec =
			b->responses / (bucket_interval / (double) MILLION);
		double loss = b->queries ?
			(b->queries - b->responses) / (double) b->queries : 0.0;
		double loss_percent = loss * 100.0;
		if (loss_percent > max_loss_percent)
			break;
		if (responses_per_sec > max_throughput) {
			max_throughput = responses_per_sec;
			loss_at_max_throughput = loss_percent;
		}
	}
	printf("  Maximum throughput:   %.6lf qps\n", max_throughput);
	printf("  Lost at that point:   %.2f%%\n", loss_at_max_throughput);
}

static ramp_bucket *
init_buckets(int n) {
	ramp_bucket *p;
	int i;

	p = isc_mem_get(mctx, n * sizeof(*p));
	if (p == NULL)
		perf_log_fatal("out of memory");
	for (i = 0; i < n; i++) {
		p[i].queries = p[i].responses = p[i].failures = 0;
		p[i].latency_sum = 0.0;
	}
	return p;
}

/*
 * Send a query based on a line of input.
 * Return ISC_R_NOMORE if we ran out of query IDs.
 */
static isc_result_t
do_one_line(isc_buffer_t *lines, isc_buffer_t *msg) {
	query_info *q;
	unsigned int qid;
	unsigned int sock;
	isc_region_t used;
	unsigned char *base;
	unsigned int length;
	isc_result_t result;

	isc_buffer_clear(lines);
	result = perf_datafile_next(input, lines, ISC_FALSE);
	if (result != ISC_R_SUCCESS)
		perf_log_fatal("ran out of query data");
	isc_buffer_usedregion(lines, &used);

	q = ISC_LIST_HEAD(instanding_list);
	if (! q)
		return (ISC_R_NOMORE);
	qid = (q - queries) / nsocks;
	sock = (q - queries) % nsocks;

	isc_buffer_clear(msg);
	result = perf_dns_buildrequest(NULL, (isc_textregion_t *) &used,
				       qid, edns, dnssec, tsigkey, msg);
	if (result != ISC_R_SUCCESS)
		return (result);

	q->sent_timestamp = time_now;

	base = isc_buffer_base(msg);
	length = isc_buffer_usedlength(msg);
	if (sendto(socks[sock], base, length, 0,
		   &server_addr.type.sa,
		   server_addr.length) < 1)
	{
		perf_log_warning("failed to send packet: %s",
				 strerror(errno));
		return (ISC_R_FAILURE);
	}

	ISC_LIST_UNLINK(instanding_list, q, link);
	ISC_LIST_PREPEND(outstanding_list, q, link);
	q->list = &outstanding_list;

	num_queries_sent++;
	num_queries_outstanding++;

	return ISC_R_SUCCESS;
}

static void
enter_sustain_phase(void) {
	phase = PHASE_SUSTAIN;
	if (sustain_time != 0.0)
		printf("[Status] Ramp-up done, sending constant traffic\n");
	sustain_phase_began = time_now;
}

static void
enter_wait_phase(void) {
	phase = PHASE_WAIT;
	printf("[Status] Waiting for more responses\n");
	wait_phase_began = time_now;
}

/*
 * try_process_response:
 *
 *   Receive from the given socket & process an individual response packet.
 *   Remove it from the list of open queries (status[]) and decrement the
 *   number of outstanding queries if it matches an open query.
 */
static void
try_process_response(unsigned int sockindex) {
	unsigned char packet_buffer[MAX_EDNS_PACKET];
	isc_uint16_t *packet_header;
	isc_uint16_t qid, rcode;
	query_info *q;
	double latency;
	ramp_bucket *b;
	int n;

	packet_header = (isc_uint16_t *) packet_buffer;
	n = recvfrom(socks[sockindex], packet_buffer, sizeof(packet_buffer),
		     0, NULL, NULL);
	if (n < 0) {
		if (errno == EAGAIN || errno == EINTR) {
			return;
		} else {
			perf_log_fatal("failed to receive packet: %s",
				       strerror(errno));
		}
	} else if (n < 4) {
		perf_log_warning("received short response");
		return;
	}

	qid = ntohs(packet_header[0]);
	rcode = ntohs(packet_header[1]) & 0xF;

	q = &queries[qid * nsocks + sockindex];
	if (q->list != &outstanding_list) {
		perf_log_warning("received a response with an "
				 "unexpected id: %u", qid);
		return;
	}

	ISC_LIST_UNLINK(outstanding_list, q, link);
	ISC_LIST_APPEND(instanding_list, q, link);
	q->list = &instanding_list;

	num_queries_outstanding--;

	latency = (time_now - q->sent_timestamp) / (double)MILLION;
	b = find_bucket(q->sent_timestamp);
	b->responses++;
	if (!(rcode == dns_rcode_noerror || rcode == dns_rcode_nxdomain))
		b->failures++;
	b->latency_sum += latency;
	num_responses_received++;
	rcodecounts[rcode]++;
}

static void
retire_old_queries(void)
{
	query_info *q;

	while (ISC_TRUE) {
		q = ISC_LIST_TAIL(outstanding_list);
		if (q == NULL ||
		    (time_now - q->sent_timestamp) < query_timeout)
			break;
		ISC_LIST_UNLINK(outstanding_list, q, link);
		ISC_LIST_APPEND(instanding_list, q, link);
		q->list = &instanding_list;

		num_queries_outstanding--;
		num_queries_timed_out++;
	}
}

static inline int
num_scheduled(isc_uint64_t time_since_start)
{
	if (phase == PHASE_RAMP) {
		return 0.5 * max_qps *
		       (double)time_since_start * time_since_start /
		       (ramp_time * MILLION);
	} else { /* PHASE_SUSTAIN */
		return 0.5 * max_qps * (ramp_time / (double)MILLION) +
		       max_qps *
		       (time_since_start - ramp_time) / (double)MILLION;
	}
}

int
main(int argc, char **argv) {
	int i;
	FILE *plotf;
	isc_buffer_t lines, msg;
	char input_data[MAX_INPUT_DATA];
	unsigned char outpacket_buffer[MAX_EDNS_PACKET];
	unsigned int max_packet_size;
	unsigned int current_sock;
	isc_result_t result;

	printf("DNS Resolution Performance Testing Tool\n"
	       "Nominum Version " VERSION "\n\n");

	setup(argc, argv);

	isc_buffer_init(&lines, input_data, sizeof(input_data));

	max_packet_size = edns ? MAX_EDNS_PACKET : MAX_UDP_PACKET;
	isc_buffer_init(&msg, outpacket_buffer, max_packet_size);

	traffic_time = ramp_time + sustain_time;
	end_time = traffic_time + wait_time;

	n_buckets = (traffic_time + bucket_interval - 1) / bucket_interval;
	buckets = init_buckets(n_buckets);

	time_now = get_time();
	time_of_program_start = time_now;

	printf("[Status] Command line: %s", isc_file_basename(argv[0]));
	for (i = 1; i < argc; i++) {
		printf(" %s", argv[i]);
	}
	printf("\n");

	printf("[Status] Sending\n");

	current_sock = 0;
	for (;;) {
		int should_send;
		isc_uint64_t time_since_start = time_now -
						time_of_program_start;
		switch (phase) {
		case PHASE_RAMP:
			if (time_since_start >= ramp_time)
				enter_sustain_phase();
			break;
		case PHASE_SUSTAIN:
			if (time_since_start >= traffic_time)
				enter_wait_phase();
			break;
		case PHASE_WAIT:
			if (time_since_start >= end_time ||
                            ISC_LIST_EMPTY(outstanding_list))
				goto end_loop;
			break;
		}
		if (phase != PHASE_WAIT) {
			should_send = num_scheduled(time_since_start) -
				      num_queries_sent;
			if (should_send >= 1000) {
				printf("[Status] Fell behind by %d queries, "
				       "ending test at %.0f qps\n",
				       should_send,
					(max_qps * time_since_start) /
					ramp_time);
				enter_wait_phase();
			}
			if (should_send > 0) {
				result = do_one_line(&lines, &msg);
				if (result == ISC_R_SUCCESS)
					find_bucket(time_now)->queries++;
				if (result == ISC_R_NOMORE) {
					printf("[Status] Reached %u outstanding queries\n",
					       max_outstanding);
					enter_wait_phase();
				}
			}
		}
		try_process_response(current_sock++);
		current_sock = current_sock % nsocks;
		retire_old_queries();
		time_now = get_time();
	}
 end_loop:
	time_now = get_time();
	time_of_end_of_run = time_now;

	printf("[Status] Testing complete\n");

	plotf = fopen(plotfile, "w");
	if (! plotf) {
		perf_log_fatal("could not open %s: %s", plotfile,
			       strerror(errno));
	}

	/* Print column headers */
	fprintf(plotf, "# time target_qps actual_qps "
		"responses_per_sec failures_per_sec "
		"avg_latency\n");

	/* Don't print unused buckets */
	last_bucket_used = find_bucket(wait_phase_began) - buckets;

	/* Don't print a partial bucket at the end */
	if (last_bucket_used > 0)
		--last_bucket_used;

	for (i = 0; i <= last_bucket_used; i++) {
		double t = (i + 0.5) * traffic_time /
			   (n_buckets * (double)MILLION);
		double ramp_dtime = ramp_time / (double)MILLION;
		double target_qps =
			t <= ramp_dtime ? (t / ramp_dtime) * max_qps : max_qps;
		double latency = buckets[i].responses ?
			buckets[i].latency_sum / buckets[i].responses : 0;
		double interval = bucket_interval / (double) MILLION;
		fprintf(plotf, "%7.3f %8.2f %8.2f %8.2f %8.2f %8.6f\n",
			t,
			target_qps,
			buckets[i].queries / interval,
			buckets[i].responses / interval,
			buckets[i].failures / interval,
			latency);
	}

	fclose(plotf);
	print_statistics();
	cleanup();

	return 0;
}
