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

#include "db.h"

// morse times are ages -- how many samples since this happened?
typedef unsigned long morse_time_t;

typedef struct morse_fist_struct
{
	morse_time_t dit, dah;
	morse_time_t tid, letter;
} *morse_fist_t;

// each of these represents a mark then a space
typedef struct morse_decode_struct
{
	morse_time_t age, mark, space;
	db_t snr;
	char text, whitespace;
} morse_decode_t;

int morse_encode(db_t *output, int output_size, db_t mark, const char *string, morse_fist_t fist);
int morse_decode(morse_decode_t *output, int output_size, morse_time_t output_offset, const db_t *input, int input_size, db_t threshold, morse_fist_t fist);
//int morse_decode_fist(morse_fist_t fist, const morse_decode_t *input, int input_size);


int morse_text(char *text, int text_size, const morse_decode_t *output, int output_size);
int morse_trim(morse_decode_t *output, int output_size, int trim_characters);
int morse_trim_age(morse_decode_t *output, int output_size, morse_time_t age);

morse_fist_t morse_fist(void);
void morse_fist_dlete(morse_fist_t fist);
void morse_fist_wpm_set(morse_fist_t fist, int sample_per_min, int wpm, int farnsworth_wpm);
int morse_fist_wpm_get(morse_fist_t fist, int sample_per_min);

