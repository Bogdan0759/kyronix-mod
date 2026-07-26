/*
 * Copyright (c) 2026, otsos team
 * Copyright (c) 2026 Kyronix Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* !DEFINES!

$define %type uint64_t as 64 bit unsigned
$define %type int as 32 bit signed
$define %type struct bintime as binary time with sec and 64 bit frac
$define %type struct timespec as seconds and nanoseconds
$define %type struct timeval as seconds and microseconds
$define %type struct timecounter as FreeBSD-style hardware counter
$define %type struct timehands as current timekeeping state

$define %func time_init as procedure with args void
$define %func time_tick as procedure with args void
$define %func time_windup_current as procedure with args void
$define %func binuptime as procedure with args struct bintime *
$define %func nanouptime as procedure with args struct timespec *
$define %func microuptime as procedure with args struct timeval *
$define %func bintime as procedure with args struct bintime *
$define %func nanotime as procedure with args struct timespec *
$define %func microtime as procedure with args struct timeval *
$define %func getbinuptime as procedure with args struct bintime *
$define %func getnanouptime as procedure with args struct timespec *
$define %func getmicrouptime as procedure with args struct timeval *
$define %func getbintime as procedure with args struct bintime *
$define %func getnanotime as procedure with args struct timespec *
$define %func getmicrotime as procedure with args struct timeval *
$define %func bintime_add as procedure with args struct bintime *, const struct bintime *
$define %func bintime_sub as procedure with args struct bintime *, const struct bintime *
$define %func bintime_add_ns as procedure with args struct bintime *, uint64_t
$define %func bintime_sub_ns as procedure with args struct bintime *, uint64_t
$define %func bintime_add_delta as procedure with args struct bintime *, uint64_t, uint64_t
$define %func bintime_from_counter as procedure with args struct bintime *
$define %func bintime_to_timespec as procedure with args const struct bintime *, struct timespec *
$define %func bintime_to_timeval as procedure with args const struct bintime *, struct timeval *
$define %func time_windup as procedure with args void

*/

/* !SPACE!

$space %internal bintime_add_delta, bintime_from_counter
$space %internal bintime_to_timespec, bintime_to_timeval
$space %internal time_windup
$space %export time_init, time_tick, time_windup_current
$space %export binuptime, nanouptime, microuptime
$space %export bintime, nanotime, microtime
$space %export getbinuptime, getnanouptime, getmicrouptime
$space %export getbintime, getnanotime, getmicrotime
$space %export bintime_add, bintime_sub, bintime_add_ns, bintime_sub_ns
$space %export time_second, time_uptime

*/

#include "arch/x86_64/pit.h"
#include "lib/log.h"
#include "lib/string.h"
#include "time.h"

static struct bintime	boottime;
static struct timehands	th0;
struct timehands	*timehands = &th0;
volatile uint64_t	time_second;
volatile uint64_t	time_uptime;
static int		time_initialized;
#define BINTIME_NS_SCALE	((~0ULL / NSEC_PER_SEC) + 1)
#define BINTIME_US_SCALE	((~0ULL / USEC_PER_SEC) + 1)

static void
bintime_add_delta(struct bintime *bt, uint64_t delta, uint64_t scale)
{
	unsigned __int128	prod;
	uint64_t		frac_inc;
	uint64_t		sec_inc;
	prod = (unsigned __int128)delta * scale;
	sec_inc = (uint64_t)(prod >> 64);
	frac_inc = (uint64_t)prod;
	bt->sec += sec_inc;
	bt->frac += frac_inc;
	if (bt->frac < frac_inc) {
		bt->sec++;
	}
}

static void
bintime_from_counter(struct bintime *bt)
{
	struct timehands	*th;
	struct timecounter	*tc;
	uint64_t		now;
	uint64_t		delta;

	th = timehands;
	tc = th->th_counter;
	if (tc == NULL) {
		bt->sec = 0;
		bt->frac = 0;
		return;
	}

	now = tc->tc_get_timecount(tc);
	delta = (now - th->th_offset_count) & th->th_counter_mask;

	*bt = th->th_offset;
	bintime_add_delta(bt, delta, th->th_scale);
}

void
bintime_add(struct bintime *bt, const struct bintime *bt2)
{
	uint64_t	frac;

	frac = bt->frac;
	bt->frac += bt2->frac;
	bt->sec += bt2->sec;
	if (bt->frac < frac) {
		bt->sec++;
	}
}

void
bintime_sub(struct bintime *bt, const struct bintime *bt2)
{
	uint64_t	frac;

	frac = bt->frac;
	bt->frac -= bt2->frac;
	bt->sec -= bt2->sec;
	if (bt->frac > frac) {
		bt->sec--;
	}
}

void
bintime_add_ns(struct bintime *bt, uint64_t ns)
{
	unsigned __int128	prod;
	uint64_t		frac_inc;
	uint64_t		sec_inc;

	prod = (unsigned __int128)ns * BINTIME_NS_SCALE;
	sec_inc = (uint64_t)(prod >> 64);
	frac_inc = (uint64_t)prod;

	bt->sec += sec_inc;
	bt->frac += frac_inc;
	if (bt->frac < frac_inc) {
		bt->sec++;
	}
}

void
bintime_sub_ns(struct bintime *bt, uint64_t ns)
{
	unsigned __int128	prod;
	uint64_t		frac_dec;
	uint64_t		sec_dec;
	uint64_t		frac;

	prod = (unsigned __int128)ns * BINTIME_NS_SCALE;
	sec_dec = (uint64_t)(prod >> 64);
	frac_dec = (uint64_t)prod;

	bt->sec -= sec_dec;
	frac = bt->frac;
	bt->frac -= frac_dec;
	if (bt->frac > frac) {
		bt->sec--;
	}
}

static void
bintime_to_timespec(const struct bintime *bt, struct timespec *ts)
{
	ts->tv_sec = bt->sec;
	ts->tv_nsec = (long)bintime_frac_to_nsec(bt->frac);
}

static void
bintime_to_timeval(const struct bintime *bt, struct timeval *tv)
{
	tv->tv_sec = bt->sec;
	tv->tv_usec = (long)bintime_frac_to_usec(bt->frac);
}

void
binuptime(struct bintime *bt)
{
	bintime_from_counter(bt);
}

void
nanouptime(struct timespec *ts)
{
	struct bintime	bt;

	binuptime(&bt);
	bintime_to_timespec(&bt, ts);
}

void
microuptime(struct timeval *tv)
{
	struct bintime	bt;

	binuptime(&bt);
	bintime_to_timeval(&bt, tv);
}

void
bintime(struct bintime *bt)
{
	binuptime(bt);
	bintime_add(bt, &boottime);
}

void
nanotime(struct timespec *ts)
{
	struct bintime	bt;

	bintime(&bt);
	bintime_to_timespec(&bt, ts);
}

void
microtime(struct timeval *tv)
{
	struct bintime	bt;

	bintime(&bt);
	bintime_to_timeval(&bt, tv);
}

void
getbinuptime(struct bintime *bt)
{
	binuptime(bt);
}

void
getnanouptime(struct timespec *ts)
{
	nanouptime(ts);
}

void
getmicrouptime(struct timeval *tv)
{
	microuptime(tv);
}

void
getbintime(struct bintime *bt)
{
	bintime(bt);
}

void
getnanotime(struct timespec *ts)
{
	nanotime(ts);
}

void
getmicrotime(struct timeval *tv)
{
	microtime(tv);
}

void
time_tick(void)
{
	struct timehands	*th;
	struct timecounter	*tc;
	uint64_t		now;
	uint64_t		delta;

	if (!time_initialized) {
		return;
	}

	th = timehands;
	tc = th->th_counter;
	if (tc == NULL) {
		return;
	}

	now = tc->tc_get_timecount(tc);
	delta = (now - th->th_offset_count) & th->th_counter_mask;

	bintime_add_delta(&th->th_offset, delta, th->th_scale);
	th->th_offset_count = now;

	time_uptime = th->th_offset.sec;
	time_second = boottime.sec + time_uptime;
}

static void
time_windup(void)
{
	struct timecounter	*tc;
	struct timehands	*th;
	struct bintime		bt;

	th = timehands;
	tc = tc_get_current();
	if (tc == NULL) {
		return;
	}

	if (th->th_counter != tc) {
		if (th->th_counter != NULL) {
			bintime_from_counter(&bt);
			th->th_offset = bt;
		}
		th->th_counter = tc;
		th->th_offset_count = tc->tc_get_timecount(tc);
		th->th_counter_mask = tc->tc_counter_mask;
		th->th_scale = (~0ULL / tc->tc_frequency) + 1;
		log_info("windup with %s: scale=0x%lx",
		    tc->tc_name, th->th_scale);
	}
}

void
time_init(void)
{
	if (time_initialized) {
		return;
	}
	boottime.sec = g_epoch_base;
	boottime.frac = 0;
	memset(&th0, 0, sizeof(th0));
	timehands = &th0;
	time_second = boottime.sec;
	time_uptime = 0;
	time_initialized = 1;
	time_windup();
	if (tc_get_current() != NULL) {
		log_info("inited with %s",
		    tc_get_current()->tc_name);
	} else {
		log_info("waiting for clocksource");
	}
}

void
time_windup_current(void)
{
	if (time_initialized) {
		time_windup();
	}
}
