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
#include <simple_ta.h>
#include <arm_sys_counter.h>

#define TIME_IN_US 1000000.0
static uint64_t convert_to_time_in_us(uint32_t freq, uint64_t cnt)
{
	float one_clock_time_in_us = TIME_IN_US / (float)freq;
	return (uint64_t)(cnt * one_clock_time_in_us);
}

TEEC_Result measure_time_of_open_session(TEEC_UUID *uuid, TEEC_Context *ctx, TEEC_Session *sess)
{
	TEEC_Operation op;
	TEEC_Result res;

	/*
	 * This is what makes the call communicate with a certain Trusted
	 * Application. In this case, the Simple TA (see also
	 * simple_ta.h, those must be the same UUID).
	 */
	uint32_t err_origin;
	uint64_t counter_start = 0;
	uint64_t counter_ta = 0;
	uint64_t counter_end = 0;


	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, ctx);
	if (res != TEEC_SUCCESS) {
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);
		return res;
	}

	/*
	 * Open a session to the Simple TA (this corresponds to the
	 * function TA_OpenSessionEntryPoint() in the TA).
	 */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
						TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE);

	op.params[0].tmpref.buffer = (void *)&counter_ta;
	op.params[0].tmpref.size = sizeof(counter_ta);

	counter_start = arm_sys_counter_get_counter();

	res = TEEC_OpenSession(ctx, sess, uuid,
			       TEEC_LOGIN_PUBLIC, NULL, &op, &err_origin);

	counter_end = arm_sys_counter_get_counter();

	if (res != TEEC_SUCCESS) {
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);
	} else {
		uint32_t freq = arm_sys_counter_get_frequency();

		uint64_t one_way_clock_count = counter_ta - counter_start;
		uint64_t round_trip_clock_count = counter_end - counter_start;
		uint64_t one_way_time = convert_to_time_in_us(freq, counter_ta - counter_start);
		uint64_t round_trip_time = convert_to_time_in_us(freq, counter_end - counter_start);

		printf("Performance Measurement: [Host app. to Trusted app. Open session]\n");
		printf("\tSystem counter frequency = %d\n", freq);
		printf("\tCounter: start=%" PRId64 " ta=%" PRId64 " end=%" PRId64 "\n", counter_start, counter_ta, counter_end);
		printf("\tOne way trip clock count = %" PRId64 " time = %" PRId64 " us\n", one_way_clock_count, one_way_time);
		printf("\tRound trip clock count = %" PRId64 " time = %" PRId64 " us\n", round_trip_clock_count, round_trip_time);
	}

	return res;
}

TEEC_Result measure_time_of_invoke_command(TEEC_Session *sess)
{
	TEEC_Operation op;
	TEEC_Result res;
	uint32_t err_origin;
	uint64_t counter_start = 0;
	uint64_t counter_ta = 0;
	uint64_t counter_end = 0;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE,
					 TEEC_NONE,
					 TEEC_NONE);

	op.params[0].tmpref.buffer = (void *)&counter_ta;
	op.params[0].tmpref.size = sizeof(counter_ta);

	counter_start = arm_sys_counter_get_counter();

	res = TEEC_InvokeCommand(sess, TAF_REPORT_TIMESTAMP, &op, &err_origin);

	counter_end = arm_sys_counter_get_counter();

	if (res != TEEC_SUCCESS) {
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);
	} else {
		uint32_t freq = arm_sys_counter_get_frequency();

		uint64_t one_way_clock_count = counter_ta - counter_start;
		uint64_t round_trip_clock_count = counter_end - counter_start;
		uint64_t one_way_time = convert_to_time_in_us(freq, counter_ta - counter_start);
		uint64_t round_trip_time = convert_to_time_in_us(freq, counter_end - counter_start);

		printf("Performance Measurement: [Host app. to Trusted app. Invoke command]\n");
		printf("\tSystem counter frequency = %d\n", freq);
		printf("\tCounter: start=%" PRId64 " ta=%" PRId64 " end=%" PRId64 "\n", counter_start, counter_ta, counter_end);
		printf("\tOne way trip clock count = %" PRId64 " time = %" PRId64 " us\n", one_way_clock_count, one_way_time);
		printf("\tRound trip clock count = %" PRId64 " time = %" PRId64 " us\n", round_trip_clock_count, round_trip_time);
	}

	return res;
}

TEEC_Result measure_time_of_secure_sys_call(TEEC_Session *sess)
{
	TEEC_Operation op;
	TEEC_Result res;
	uint32_t err_origin;
	uint64_t sys_call_one_way_clock_count = 0;
	uint64_t sys_call_round_trip_clock_count = 0;


	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
						TEEC_MEMREF_TEMP_OUTPUT,
						TEEC_NONE,
						TEEC_NONE);

	op.params[0].tmpref.buffer = (void *)&sys_call_one_way_clock_count;
	op.params[0].tmpref.size = sizeof(sys_call_one_way_clock_count);

	op.params[1].tmpref.buffer = (void *)&sys_call_round_trip_clock_count;
	op.params[1].tmpref.size = sizeof(sys_call_round_trip_clock_count);


	res = TEEC_InvokeCommand(sess, TAF_MEASURE_SYS_CALL_TIME, &op, &err_origin);

	if (res != TEEC_SUCCESS) {
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);
	} else {
		uint32_t freq = arm_sys_counter_get_frequency();

		uint64_t one_way_time = convert_to_time_in_us(freq, sys_call_one_way_clock_count);
		uint64_t round_trip_time = convert_to_time_in_us(freq, sys_call_round_trip_clock_count);

		printf("Performance Measurement: [Secure system call]\n");
		printf("\tSystem counter frequency = %d\n", freq);
		printf("\tOne way trip clock count = %" PRId64 " time = %" PRId64 " us\n", sys_call_one_way_clock_count, one_way_time);
		printf("\tRound trip clock count = %" PRId64 " time = %" PRId64 " us\n", sys_call_round_trip_clock_count, round_trip_time);
	}

	return res;
}

void close_session_and_finalize_context(TEEC_Context *ctx, TEEC_Session *sess)
{
	/* We're done with the TA, close the session ... */
	TEEC_CloseSession(sess);

	/* ... and destroy the context. */
	TEEC_FinalizeContext(ctx);
}


void measure_performance(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;

	/*
	 * This is what makes the call communicate with a certain Trusted
	 * Application. In this case, the Simple TA (see also
	 * simple_ta.h, those must be the same UUID).
	 */
	TEEC_UUID uuid = SIMPLE_TA_UUID;

	res = measure_time_of_open_session(&uuid, &ctx, &sess);
	if (res != TEEC_SUCCESS) {
		close_session_and_finalize_context(&ctx, &sess);
		return;
	}

	res = measure_time_of_invoke_command(&sess);
	if (res != TEEC_SUCCESS) {
		close_session_and_finalize_context(&ctx, &sess);
		return;
	}

	res = measure_time_of_secure_sys_call(&sess);
	if (res != TEEC_SUCCESS) {
		close_session_and_finalize_context(&ctx, &sess);
		return;
	}

	close_session_and_finalize_context(&ctx, &sess);
}

int main(int argc, char *argv[])
{
	measure_performance();

	return 0;
}
