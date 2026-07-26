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
$define %type struct eventtimer as event timer descriptor
$define %type et_start_t as timer start callback signature
$define %type et_stop_t as timer stop callback signature
$define %type et_event_cb_t as event callback signature
$define %type et_deregister_cb_t as deregister callback signature

$define %func et_register as function with args struct eventtimer *
$define %func et_deregister as function with args struct eventtimer *
$define %func et_change_frequency as procedure with args struct eventtimer *, uint64_t
$define %func et_find as function with args const char *, int, int
$define %func et_init as function with args struct eventtimer *, et_event_cb_t *, et_deregister_cb_t *, void *
$define %func et_start as function with args struct eventtimer *, uint64_t, uint64_t
$define %func et_stop as function with args struct eventtimer *
$define %func et_ban as function with args struct eventtimer *
$define %func et_free as function with args struct eventtimer *
$define %func eventtimer_dispatch as procedure with args void

$define %macro ET_FLAGS_PERIODIC as 1
$define %macro ET_FLAGS_ONESHOT as 2
$define %macro ET_FLAGS_PERCPU as 4
$define %macro ET_FLAGS_C3STOP as 8
$define %macro ET_FLAGS_POW2DIV as 16

*/

/* !SPACE!

$space %export et_register, et_deregister, et_change_frequency
$space %export et_find, et_init, et_start, et_stop, et_ban, et_free
$space %export eventtimer_dispatch
$space %export ET_FLAGS_PERIODIC, ET_FLAGS_ONESHOT, ET_FLAGS_PERCPU
$space %export ET_FLAGS_C3STOP, ET_FLAGS_POW2DIV

*/

#pragma once

#include <stdint.h>

#define ET_FLAGS_PERIODIC	1
#define ET_FLAGS_ONESHOT	2
#define ET_FLAGS_PERCPU		4
#define ET_FLAGS_C3STOP		8
#define ET_FLAGS_POW2DIV	16

struct eventtimer;

typedef int	et_start_t(struct eventtimer *et, uint64_t first,
		    uint64_t period);
typedef int	et_stop_t(struct eventtimer *et);
typedef void	et_event_cb_t(struct eventtimer *et, void *arg);
typedef int	et_deregister_cb_t(struct eventtimer *et, void *arg);

struct eventtimer {
	struct eventtimer	*et_next;
	const char		*et_name;
	int			et_flags;
	int			et_quality;
	int			et_active;
	uint64_t		et_frequency;
	uint64_t		et_min_period;
	uint64_t		et_max_period;
	uint64_t		et_first;
	uint64_t		et_period;
	et_start_t		*et_start;
	et_stop_t		*et_stop;
	et_event_cb_t		*et_event_cb;
	et_deregister_cb_t	*et_deregister_cb;
	void			*et_arg;
	void			*et_priv;
};

#define ET_LOCK()
#define ET_UNLOCK()

int		et_register(struct eventtimer *et);
int		et_deregister(struct eventtimer *et);
void		et_change_frequency(struct eventtimer *et,
		    uint64_t newfreq);
struct eventtimer *et_find(const char *name, int check, int want);
int		et_init(struct eventtimer *et, et_event_cb_t *event,
		    et_deregister_cb_t *deregister, void *arg);
int		et_start(struct eventtimer *et, uint64_t first,
		    uint64_t period);
int		et_stop(struct eventtimer *et);
int		et_ban(struct eventtimer *et);
int		et_free(struct eventtimer *et);
void		eventtimer_dispatch(void);
int		et_get_count(void);
struct eventtimer *et_get_entry(int i);
