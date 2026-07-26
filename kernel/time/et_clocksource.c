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
$define %type struct timecounter as FreeBSD-style hardware time source

$define %func et_tc_get_timecount as function with args struct timecounter *
$define %func et_clocksource_init as procedure with args void

*/

/* !SPACE!

$space %internal et_tc_get_timecount
$space %export et_clocksource_init

*/

#include "drivers/timer.h"
#include "time.h"
#include "time/clocksource.h"

static uint64_t
et_tc_get_timecount(struct timecounter *tc)
{
	(void)tc;
	return (timer_get_ticks());
}

static struct timecounter	et_timecounter = {
	.tc_next		= NULL,
	.tc_counter_mask	= ~0ULL,
	.tc_frequency		= 0,
	.tc_get_timecount	= et_tc_get_timecount,
	.tc_quality		= 100,
	.tc_name		= "et_timer_ticks",
	.tc_priv		= NULL,
};

void
et_clocksource_init(void)
{
	et_timecounter.tc_frequency = timer_get_frequency();
	tc_register(&et_timecounter);
	time_windup_current();
}
