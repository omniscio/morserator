/*
 * BSD 3-Clause License

 * Copyright (c) 2025, Omniscio
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include "morse.h"

#if defined(WATERFALL_COMPLEX_INPUT)
typedef struct waterfall_complex_struct
{
	signed short real;
	signed short imag;
} waterfall_input_t;
#else
typedef signed short waterfall_input_t;
#endif

typedef struct waterfall_struct *waterfall_t;

//waterfall_t waterfall(int input_sampling_power_of_two, int samples, int first_channel, int last_channel);
waterfall_t waterfall(int input_sampling_power_of_two, int samples, int first_channel, int last_channel, int rows, int cols);

void waterfall_dlete(waterfall_t waterfall);
void waterfall_update(waterfall_t waterfall, const waterfall_input_t *input, int input_count);
void waterfall_clear(waterfall_t waterfall, int subchannel);

int waterfall_sync(waterfall_t waterfall, int subchannel);

const db_t *waterfall_colours(waterfall_t waterfall, int subchannel);
const morse_fist_t waterfall_fist(waterfall_t waterfall, int subchannel);
const morse_decode_t *waterfall_symbols(waterfall_t waterfall, int subchannel);
int waterfall_start(waterfall_t waterfall, int subchannel);
const char *waterfall_text(waterfall_t waterfall, int subchannel);
int waterfall_text_lines(waterfall_t waterfall, int subchannel);
