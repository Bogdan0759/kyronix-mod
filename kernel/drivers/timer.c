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
$define %type uint32_t as 32 bit unsigned
$define %type int as 32 bit signed
$define %type struct eventtimer as event timer descriptor

$define %func timer_init as procedure with args uint32_t
$define %func timer_reinit as procedure with args uint32_t
$define %func timer_get_ticks as function with args void
$define %func timer_is_initialized as function with args void
$define %func timer_get_frequency as function with args void
$define %func timer_calibrate as function with args struct timer_calibrate *, uint32_t, uint32_t

*/

/* !SPACE!

$space %internal timer_event_cb
$space %export timer_init, timer_reinit, timer_get_ticks
$space %export timer_is_initialized, timer_get_frequency, timer_calibrate
$space %export g_ticks

*/

#include "arch/x86_64/cpu.h"
#include "drivers/eventtimer.h"
#include "drivers/timer.h"
#include "lib/log.h"
#include "time.h"

volatile uint64_t	g_ticks;

static struct eventtimer	*timer_et;
static uint32_t			timer_frequency;
static int			timer_initialized;

static void
timer_event_cb(struct eventtimer *et, void *arg)
{
	(void)et;
	(void)arg;

	__atomic_fetch_add(&g_ticks, 1, __ATOMIC_RELAXED);
	time_tick();
}

void
timer_init(uint32_t frequency)
{
	struct eventtimer	*et;
	uint64_t		period_ns;

	if (frequency == 0) {
		log_warn("TIMER: refusing zero frequency");
		return;
	}

	et = et_find(NULL, ET_FLAGS_PERIODIC, ET_FLAGS_PERIODIC);
	if (et == NULL) {
		log_warn("TIMER: no periodic event timer available");
		return;
	}

	if (et_init(et, timer_event_cb, NULL, NULL) != 0) {
		log_warn("TIMER: failed to init event timer");
		return;
	}

	period_ns = 1000000000ULL / frequency;
	if (et_start(et, 0, period_ns) != 0) {
		log_warn("TIMER: failed to start event timer");
		et_free(et);
		return;
	}

	timer_et = et;
	timer_frequency = frequency;
	timer_initialized = 1;
	log_info("TIMER: using event timer \"%s\" at %u Hz",
	    et->et_name, timer_frequency);
}

void
timer_reinit(uint32_t frequency)
{
	if (timer_et != NULL) {
		et_stop(timer_et);
		et_free(timer_et);
	}
	timer_et = NULL;
	timer_initialized = 0;
	timer_init(frequency);
}

uint64_t
timer_get_ticks(void)
{
	return (__atomic_load_n(&g_ticks, __ATOMIC_RELAXED));
}

int
timer_is_initialized(void)
{
	return (timer_initialized);
}

uint32_t
timer_get_frequency(void)
{
	return (timer_frequency);
}

uint64_t
timer_calibrate(struct timer_calibrate *calib, uint32_t ticks,
    uint32_t divider)
{
	uint64_t	tick_start, tick_end, tick_delta;
	uint64_t	count_start, count_end, count_delta;
	uint64_t	freq;
	uint32_t	tries;

	if (!timer_initialized || calib == NULL ||
	    calib->read_count == NULL) {
		return (0);
	}
	if (ticks == 0 || divider == 0) {
		return (0);
	}

	tries = 0;
	do {
		tick_start = timer_get_ticks();
		count_start = calib->read_count(calib->arg);

		while (timer_get_ticks() - tick_start < ticks) {
			cpu_relax();
		}

		count_end = calib->read_count(calib->arg);
		tick_end = timer_get_ticks();
		tries++;
	} while (count_start <= count_end && tries < 5);

	if (count_start <= count_end) {
		return (0);
	}

	tick_delta = tick_end - tick_start;
	count_delta = count_start - count_end;
	if (tick_delta == 0 || count_delta == 0) {
		return (0);
	}

	freq = count_delta * (uint64_t)divider * timer_frequency /
	    tick_delta;
	return (freq);
}
