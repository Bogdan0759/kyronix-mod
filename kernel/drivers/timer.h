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

$define %func timer_init as procedure with args uint32_t
$define %func timer_reinit as procedure with args uint32_t
$define %func timer_get_ticks as function with args void
$define %func timer_is_initialized as function with args void
$define %func timer_get_frequency as function with args void
$define %func timer_calibrate as function with args struct timer_calibrate *, uint32_t, uint32_t

*/

/* !SPACE!

$space %export timer_init, timer_reinit, timer_get_ticks
$space %export timer_is_initialized, timer_get_frequency, timer_calibrate
$space %export g_ticks

*/

#pragma once

#include <stdint.h>

struct timer_calibrate {
	uint64_t	(*read_count)(void *arg);
	void	*arg;
};

extern volatile uint64_t	g_ticks;

void		timer_init(uint32_t frequency);
void		timer_reinit(uint32_t frequency);
uint64_t	timer_get_ticks(void);
int		timer_is_initialized(void);
uint32_t	timer_get_frequency(void);
uint64_t	timer_calibrate(struct timer_calibrate *calib,
		    uint32_t ticks, uint32_t divider);
