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
		printf("ClientOpenSession: start=0x%" PRIx64 ", enter_ta=0x%" PRIx64 ", return=0x%" PRIx64 "\n",
				counter_start, counter_ta, counter_end);
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
		printf("ClientInvokeCommand: start=0x%" PRIx64 ", enter_ta=0x%" PRIx64 ", return=0x%" PRIx64 "\n",
				counter_start, counter_ta, counter_end);
	}

	return res;
}

TEEC_Result measure_time_of_secure_sys_call(TEEC_Session *sess)
{
	TEEC_Operation op;
	TEEC_Result res;
	uint32_t err_origin;
	uint64_t start_svc_clock_count = 0;
	uint64_t enter_svc_clock_count = 0;
	uint64_t return_svc_clock_count = 0;


	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
						TEEC_MEMREF_TEMP_OUTPUT,
						TEEC_MEMREF_TEMP_OUTPUT,
						TEEC_NONE);

	op.params[0].tmpref.buffer = (void *)&start_svc_clock_count;
	op.params[0].tmpref.size = sizeof(start_svc_clock_count);

	op.params[1].tmpref.buffer = (void *)&enter_svc_clock_count;
	op.params[1].tmpref.size = sizeof(enter_svc_clock_count);

	op.params[2].tmpref.buffer = (void *)&return_svc_clock_count;
	op.params[2].tmpref.size = sizeof(return_svc_clock_count);

	res = TEEC_InvokeCommand(sess, TAF_MEASURE_SYS_CALL_TIME, &op, &err_origin);

	if (res != TEEC_SUCCESS) {
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);
	} else {
		printf("SecureSysCall: start=0x%" PRIx64 ", enter_svc=0x%" PRIx64 ", return=0x%" PRIx64 "\n",
				start_svc_clock_count, enter_svc_clock_count, return_svc_clock_count);
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

	uint32_t freq = arm_sys_counter_get_frequency();
	printf("CounterTimerFrequency: 0x%x\n", freq);

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
