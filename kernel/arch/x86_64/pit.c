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
$define %type uint16_t as 16 bit unsigned
$define %type uint8_t as 8 bit unsigned
$define %type int as 32 bit signed

$define %func pit_init as procedure with args void
$define %func cmos_read as function with args uint8_t
$define %func bcd2bin as function with args uint8_t
$define %func rtc_read_unix as function with args void
$define %func pit_ns_to_divisor as function with args uint64_t
$define %func pit_start as function with args struct eventtimer *, uint64_t, uint64_t
$define %func pit_stop as function with args struct eventtimer *

$define %const PIT_FREQUENCY as 1193182
$define %const PIT_CMD as 0x43
$define %const PIT_CHANNEL0 as 0x40
$define %const PIT_MAX_DIVISOR as 65535

*/

/* !SPACE!

$space %internal cmos_read, bcd2bin, rtc_read_unix
$space %internal pit_ns_to_divisor, pit_start, pit_stop
$space %export pit_init, g_epoch_base

*/

#include "pit.h"
#include "cpu.h"
#include "pic.h"
#include "drivers/eventtimer.h"

uint64_t		g_epoch_base;

static struct eventtimer	pit_et;

static uint8_t
cmos_read(uint8_t reg)
{
	outb(0x70, reg);
	io_wait();
	return (inb(0x71));
}

static uint8_t
bcd2bin(uint8_t v)
{
	return ((uint8_t)((v >> 4) * 10 + (v & 0xf)));
}

static uint64_t
rtc_read_unix(void)
{
	static const uint16_t	mdays[12] = {
		0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};
	uint32_t	year, y, ly, days;
	uint8_t		sec, min, hr, day, mon, yr, cen, sb;
	int		leap;

	while (cmos_read(0x0A) & 0x80) {
		cpu_relax();
	}
	sec = cmos_read(0x00);
	min = cmos_read(0x02);
	hr = cmos_read(0x04);
	day = cmos_read(0x07);
	mon = cmos_read(0x08);
	yr = cmos_read(0x09);
	cen = cmos_read(0x32);
	sb = cmos_read(0x0B);

	if (!(sb & 0x04)) {
		sec = bcd2bin(sec);
		min = bcd2bin(min);
		hr = bcd2bin(hr & 0x7f) | (hr & 0x80);
		day = bcd2bin(day);
		mon = bcd2bin(mon);
		yr = bcd2bin(yr);
		cen = bcd2bin(cen);
	}
	if (!(sb & 0x02) && (hr & 0x80)) {
		hr = (uint8_t)(((hr & 0x7fu) % 12u) + 12u);
	}
	year = (uint32_t)(cen ? cen * 100u : 2000u) + yr;
	y = year - 1970;
	ly = (year - 1) / 4 - (year - 1) / 100 +
	    (year - 1) / 400 - (1969 / 4 - 1969 / 100 + 1969 / 400);
	leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
	days = y * 365u + ly + mdays[mon - 1] + (day - 1u);
	if (leap && mon > 2) {
		days++;
	}

	return ((uint64_t)days * 86400u + hr * 3600u + min * 60u + sec);
}

static uint64_t
pit_ns_to_divisor(uint64_t period_ns)
{
	uint64_t	divisor;

	divisor = (PIT_FREQUENCY * period_ns + 500000000ULL) /
	    1000000000ULL;
	if (divisor < 1) {
		divisor = 1;
	} else if (divisor > PIT_MAX_DIVISOR) {
		divisor = PIT_MAX_DIVISOR;
	}
	return (divisor);
}

static int
pit_start(struct eventtimer *et, uint64_t first, uint64_t period)
{
	uint64_t	divisor;
	uint8_t		l, h;

	(void)first;
	(void)et;

	divisor = pit_ns_to_divisor(period);
	l = (uint8_t)(divisor & 0xFF);
	h = (uint8_t)((divisor >> 8) & 0xFF);

	outb(PIT_CMD, 0x36);
	outb(PIT_CHANNEL0, l);
	outb(PIT_CHANNEL0, h);
	return (0);
}

static int
pit_stop(struct eventtimer *et)
{
	(void)et;

	outb(PIT_CMD, 0x36);
	outb(PIT_CHANNEL0, 0xFF);
	outb(PIT_CHANNEL0, 0xFF);
	return (0);
}

void
pit_init(void)
{
	uint64_t	max_period_ns;

	max_period_ns = (uint64_t)PIT_MAX_DIVISOR * 1000000000ULL /
	    PIT_FREQUENCY;

	g_epoch_base = rtc_read_unix();
	pit_et.et_name = "i8254";
	pit_et.et_flags = ET_FLAGS_PERIODIC;
	pit_et.et_quality = 100;
	pit_et.et_frequency = PIT_FREQUENCY;
	pit_et.et_min_period = 1;
	pit_et.et_max_period = max_period_ns;
	pit_et.et_start = pit_start;
	pit_et.et_stop = pit_stop;
	pit_et.et_event_cb = NULL;
	pit_et.et_deregister_cb = NULL;
	pit_et.et_arg = NULL;
	pit_et.et_priv = NULL;

	et_register(&pit_et);
	pic_unmask_irq(0);
}
