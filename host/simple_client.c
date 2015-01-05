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
#include <stdio.h>
#include <err.h>
#include <simple_client.h>
#include <tee_client_api.h>
#include <limits.h>
#include <stdarg.h>
#include <simple_ta.h>

#define WORD_SIZE 32
#define TEE_TIME_MILLIS_BASE    1000

typedef unsigned long long mpa_dword_t;
typedef uint32_t mpa_word_t;

typedef struct {
	uint32_t seconds;
	uint32_t millis;
} TEE_Time;

void dump_hash(const char *message, uint8_t *hash, size_t len)
{
	size_t i;

	printf("Hash of message: '%s' is:\n", message);
	for (i = 0; i < len; i++)
		printf("%02x", hash[i]);
	printf("\n");
}


static mpa_dword_t __mpa_soft_div(mpa_dword_t num, mpa_word_t den_in,
				  mpa_word_t *rem)
{
	mpa_dword_t quot = 0, qbit = 1;
	mpa_dword_t den = den_in;

	while ((int64_t) den >= 0) {
		den <<= 1;
		qbit <<= 1;
	}

	while (qbit) {
		if (den <= num) {
			num -= den;
			quot += qbit;
		}
		den >>= 1;
		qbit >>= 1;
	}

	if (rem)
		*rem = (mpa_word_t) num;

	return quot;
}


mpa_word_t __mpa_div_dword(mpa_word_t n0,
			   mpa_word_t n1, mpa_word_t d, mpa_word_t *r)
{
	mpa_dword_t n;
	/*    mpa_dword_t tmp_q; */

	n = ((mpa_dword_t) n1 << WORD_SIZE) + n0;
	return __mpa_soft_div(n, d, r);
	/*    tmp_q = n / d; */
	/*    *r = (mpa_word_t)(n % d); */
	/*    return tmp_q; */
}


static uint32_t do_div(uint64_t *dividend, uint32_t divisor)
{
	mpa_word_t remainder = 0, n0, n1;
	n0 = (*dividend) & UINT_MAX;
	n1 = ((*dividend) >> WORD_SIZE) & UINT_MAX;
	*dividend = __mpa_div_dword(n0, n1, divisor, &remainder);
	return remainder;
}

static uint64_t read_cntpct(void)
{
	uint64_t val;
	uint32_t low, high;
	__asm__ volatile("mrrc	p15, 0, %0, %1, c14\n"
		: "=r"(low), "=r"(high)
		:
		: "memory");
	val = low | ((uint64_t)high << WORD_SIZE);
	return val;
}

static uint32_t read_cntfrq(void)
{
	uint32_t frq;
	__asm__ volatile("mrc	p15, 0, %0, c14, c0, 0\n"
		: "=r"(frq)
		:
		: "memory");
	return frq;
}

static void arm_get_sys_time(TEE_Time *time)
{
	uint64_t cntpct = read_cntpct();
	uint32_t cntfrq = read_cntfrq();
	uint32_t remainder;

	remainder = do_div(&cntpct, cntfrq);

	time->seconds = (uint32_t)cntpct;
	time->millis = remainder / (cntfrq / TEE_TIME_MILLIS_BASE);
}


static void print_with_timestamp(const char *fmt, ...)
{
	va_list ap;

	TEE_Time currentTime;
	arm_get_sys_time(&currentTime);
	printf("[%5d.%03d] ", currentTime.seconds, currentTime.millis);

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

static int32_t time_diff(TEE_Time start, TEE_Time end)
{
	return (end.seconds*1000 + end.millis) - (start.seconds*1000 + start.millis);
}

TEEC_Result measure_performance(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;

	/*
	 * This is what makes the call communicate with a certain Trusted
	 * Application. In this case, the Simple TA (see also
	 * simple_ta.h, those must be the same UUID).
	 */
	TEEC_UUID uuid = SIMPLE_TA_UUID;
	uint32_t err_origin;
	TEE_Time host_time;
	TEE_Time ta_time;
	TEE_Time *p_ta_time = &ta_time;
	int32_t internal_api_round_trip_time = 0;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/*
	 * Open a session to the Simple TA (this corresponds to the
	 * function TA_OpenSessionEntryPoint() in the TA).
	 */
	print_with_timestamp("Opening the session to Simple TA\n");
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);

	print_with_timestamp("Returned from TEEC_OpenSession()\n");


	memset(&op, 0, sizeof(op));
	/*
	 * Here we are stating that the first parameter should be a buffer and
	 * it should be considered as an input buffer, this is where we provide
	 * the message to be hashed. The second parameter is also a buffer,
	 * however this is configured as an output buffer and this will be used
	 * for storing the final digest (i.e, the resulting hash).
	 */
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE,
					 TEEC_NONE,
					 TEEC_NONE);

	op.params[0].tmpref.buffer = (void *)&internal_api_round_trip_time;
	op.params[0].tmpref.size = sizeof(internal_api_round_trip_time);

	print_with_timestamp("Invoking TAF_MEASURE_SECURE_API_TIME in Simple TA\n");
	res = TEEC_InvokeCommand(&sess, TAF_MEASURE_SECURE_API_TIME, &op, &err_origin);

	if (res != TEEC_SUCCESS) {
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);
	} else {
		print_with_timestamp("Internal API round trip time = %d ms\n", internal_api_round_trip_time);
	}

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE,
					 TEEC_NONE,
					 TEEC_NONE);

	op.params[0].tmpref.buffer = (void *)p_ta_time;
	op.params[0].tmpref.size = sizeof(TEE_Time);

	print_with_timestamp("Invoking TAF_REPORT_TIMESTAMP in Simple TA\n");
	arm_get_sys_time(&host_time);
	res = TEEC_InvokeCommand(&sess, TAF_REPORT_TIMESTAMP, &op, &err_origin);

	if ((res != TEEC_SUCCESS) || (op.params[0].tmpref.size != sizeof(TEE_Time))) {
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);
	} else {
		int32_t single_trip_time = time_diff(host_time, ta_time);
		print_with_timestamp("Host App to TA single trip time for TEEC_InvokeCommand() = %d ms\n", single_trip_time);
	}

	/* We're done with the TA, close the session ... */
	TEEC_CloseSession(&sess);

	/* ... and destroy the context. */
	TEEC_FinalizeContext(&ctx);

	return res;
}

int main(int argc, char *argv[])
{
	measure_performance();

	return 0;
}
