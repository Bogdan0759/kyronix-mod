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

$define %type int as 32 bit signed
$define %type struct timecounter as FreeBSD-style hardware time source

$define %func tc_register as procedure with args struct timecounter *
$define %func tc_deregister as procedure with args struct timecounter *
$define %func tc_get_current as function with args void
$define %func tc_best as function with args void
$define %func quality_is_better as function with args struct timecounter *, struct timecounter *
$define %func find_best as function with args void

*/

/* !SPACE!

$space %internal quality_is_better, find_best
$space %export tc_register, tc_deregister, tc_get_current, tc_best

*/

#include "lib/log.h"
#include "time/clocksource.h"

static struct timecounter	*timecounter_list;
static struct timecounter	*timecounter_current;
static int
quality_is_better(struct timecounter *a, struct timecounter *b)
{
	if (a == NULL) {
		return (0);
	}
	if (b == NULL) {
		return (1);
	}
	return (a->tc_quality > b->tc_quality);
}

static struct timecounter *
find_best(void)
{
	struct timecounter	*tc;
	struct timecounter	*best;

	best = NULL;
	for (tc = timecounter_list; tc != NULL; tc = tc->tc_next) {
		if (quality_is_better(tc, best)) {
			best = tc;
		}
	}
	return (best);
}

void
tc_register(struct timecounter *tc)
{
	struct timecounter	*tc_best_new;

	if (tc == NULL || tc->tc_get_timecount == NULL) {
		log_warn("refusing register invalid source");
		return;
	}

	if (tc->tc_counter_mask == 0) {
		tc->tc_counter_mask = ~0ULL;
	}

	if (tc->tc_frequency == 0) {
		log_warn("%s has zero freq",
		    tc->tc_name);
		return;
	}
	tc->tc_next = timecounter_list;
	timecounter_list = tc;

	log_info("registeres %s: freq=%lu Hz and quality=%d",
	    tc->tc_name, tc->tc_frequency, tc->tc_quality);
	tc_best_new = find_best();
	if (tc_best_new != timecounter_current) {
		if (timecounter_current != NULL) {
			log_info("switching %s to %s",
			    timecounter_current->tc_name,
			    tc_best_new->tc_name);
		} else {
			log_info("selecting %s",
			    tc_best_new->tc_name);
		}
		timecounter_current = tc_best_new;
	}
}

void
tc_deregister(struct timecounter *tc)
{
	struct timecounter	**tcpp;
	struct timecounter	*tc_best_new;

	if (tc == NULL) {
		return;
	}

	for (tcpp = &timecounter_list; *tcpp != NULL;
	    tcpp = &(*tcpp)->tc_next) {
		if (*tcpp == tc) {
			*tcpp = tc->tc_next;
			break;
		}
	}

	if (timecounter_current == tc) {
		tc_best_new = find_best();
		timecounter_current = tc_best_new;
		if (tc_best_new != NULL) {
			log_info("fallback to  %s",
			    tc_best_new->tc_name);
		}
	}
}

struct timecounter *
tc_best(void)
{
	return (find_best());
}

struct timecounter *
tc_get_current(void)
{
	return (timecounter_current);
}
