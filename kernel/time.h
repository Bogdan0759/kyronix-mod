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
$define %type int64_t as 64 bit signed
$define %type int as 32 bit signed
$define %type struct bintime as binary time with sec and 64 bit frac
$define %type struct timespec as seconds and nanoseconds
$define %type struct timeval as seconds and microseconds
$define %type struct timecounter as FreeBSD-style hardware counter
$define %type struct timehands as current timekeeping state

$define %func time_init as procedure with args void
$define %func time_tick as procedure with args void
$define %func time_windup_current as procedure with args void
$define %func et_clocksource_init as procedure with args void
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

*/

/* !SPACE!

$space %export time_init, time_tick, time_windup_current
$space %export et_clocksource_init
$space %export binuptime, nanouptime, microuptime
$space %export bintime, nanotime, microtime
$space %export getbinuptime, getnanouptime, getmicrouptime
$space %export getbintime, getnanotime, getmicrotime
$space %export bintime_add, bintime_sub, bintime_add_ns, bintime_sub_ns
$space %export time_second, time_uptime

*/

#pragma once

#include <stdint.h>

#include "time/clocksource.h"

#define NSEC_PER_SEC	1000000000ULL
#define USEC_PER_SEC	1000000ULL

struct bintime {
	uint64_t	sec;
	uint64_t	frac;
};

struct timespec {
	uint64_t	tv_sec;
	long		tv_nsec;
};

struct timeval {
	uint64_t	tv_sec;
	long		tv_usec;
};

struct timehands {
	struct bintime		th_offset;
	uint64_t		th_offset_count;
	uint64_t		th_scale;
	uint64_t		th_counter_mask;
	struct timecounter	*th_counter;
};

extern volatile uint64_t	time_second;
extern volatile uint64_t	time_uptime;
extern struct timehands		*timehands;

void		time_init(void);
void		time_tick(void);
void		time_windup_current(void);
void		et_clocksource_init(void);
void		binuptime(struct bintime *bt);
void		nanouptime(struct timespec *ts);
void		microuptime(struct timeval *tv);
void		bintime(struct bintime *bt);
void		nanotime(struct timespec *ts);
void		microtime(struct timeval *tv);
void		getbinuptime(struct bintime *bt);
void		getnanouptime(struct timespec *ts);
void		getmicrouptime(struct timeval *tv);
void		getbintime(struct bintime *bt);
void		getnanotime(struct timespec *ts);
void		getmicrotime(struct timeval *tv);
void		bintime_add(struct bintime *bt, const struct bintime *bt2);
void		bintime_sub(struct bintime *bt, const struct bintime *bt2);
void		bintime_add_ns(struct bintime *bt, uint64_t ns);
void		bintime_sub_ns(struct bintime *bt, uint64_t ns);

static inline uint64_t
bintime_frac_to_nsec(uint64_t frac)
{
	return ((unsigned __int128)frac * NSEC_PER_SEC >> 64);
}

static inline uint64_t
bintime_frac_to_usec(uint64_t frac)
{
	return ((unsigned __int128)frac * USEC_PER_SEC >> 64);
}
