/*
 * Copyright (c) 2014, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * liABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <arm_sys_counter.h>

#define WORD_SIZE 32

uint64_t arm_sys_counter_get_counter(void)
{
	uint64_t val;

#ifdef __aarch64__

	asm volatile("mrs %0, cntvct_el0" : "=r"(val));

#else

	uint32_t cntvct_low, cntvct_high;
	__asm__ volatile("mrrc	p15, 1, %0, %1, c14"
			: "=r"(cntvct_low), "=r"(cntvct_high));

	val = cntvct_low | ((uint64_t)cntvct_high << WORD_SIZE);

#endif

	return val;
}


uint32_t arm_sys_counter_get_frequency(void)
{
	uint32_t frq;

#ifdef __aarch64__

	asm volatile("mrs %0, cntfrq_el0" : "=r"(frq));

#else

	asm volatile("mrc	p15, 0, %0, c14, c0, 0" : "=r"(frq));

#endif

	return frq;
}
