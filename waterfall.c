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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "waterfall.h"

#define ARRAY_SIZE(array)	(sizeof(array)/sizeof(*array))

//#define WATERFALL_QUALITY	230
#define WATERFALL_THRESHOLD_DB			+8
#define WATERFALL_THRESHOLD_ONOFF		3
#define WATERFALL_THRESHOLD_COEFFICIENT	9

//#define WATERFALL_FILTER_SIZE	4
#define WATERFALL_FILTER_COEFFICIENT 	20

#define FFT_COS12_TABLE_BITS			12

#define COS12(x)	cos12[(x) & ((1 << FFT_COS12_TABLE_BITS) - 1)]
#define SIN12(x)	COS12((x) - (1 << (FFT_COS12_TABLE_BITS - 2)))

struct waterfall_channel_struct
{
	morse_fist_t fist;
	db_t *colours, *inputs;
	morse_decode_t *decodes;
	char *text;
	int start, text_end;
#if defined(WATERFALL_FILTER_SIZE)
	db_t filter[WATERFALL_FILTER_SIZE];
#endif
	unsigned char updates;
	db_t threshold;
};

struct waterfall_struct
{
	int subchannels, first_subchannel, input_sampling_power_of_two, buffer_count, samples, rows, cols;
	waterfall_input_t *buffer;
	db_integer_t average;
	struct waterfall_channel_struct channels[];
};


waterfall_t waterfall(int input_sampling_power_of_two, int samples, int first_channel, int last_channel, int rows, int cols);
void waterfall_clear(waterfall_t waterfall, int subchannel);
const db_t *waterfall_colours(waterfall_t waterfall, int subchannel);
static void waterfall_cos12_start(void);
void waterfall_dlete(waterfall_t waterfall);
static db_t waterfall_fft_row(const waterfall_input_t *in, int logcount, int row);
const morse_fist_t waterfall_fist(waterfall_t waterfall, int subchannel);
db_t waterfall_get(db_t *colours, int colours_size, waterfall_t waterfall, int subchannel);
int waterfall_start(waterfall_t waterfall, int subchannel);
const morse_decode_t *waterfall_symbols(waterfall_t waterfall, int subchannel);
int waterfall_sync(waterfall_t waterfall, int subchannel);
const char *waterfall_text(waterfall_t waterfall, int subchannel);
int waterfall_text_lines(waterfall_t waterfall, int subchannel);
void waterfall_update(waterfall_t waterfall, const waterfall_input_t *input, int input_count);
static db_integer_t waterfall_update_block(waterfall_t waterfall, const waterfall_input_t *block);

static const int *cos12;



waterfall_t waterfall(int input_sampling_power_of_two, int samples, int first_channel, int last_channel, int rows, int cols)
{
	struct waterfall_channel_struct *c = 0;
	waterfall_t waterfall = 0;
	int first_subchannel, subchannels;
	int i;

	if(input_sampling_power_of_two <= 2 || samples <= 0) return(0);

#if defined(WATERFALL_COMPLEX_INPUT)
	if(first_channel <= -(1 << (input_sampling_power_of_two - 1)) || first_channel >= (1 << (input_sampling_power_of_two - 1))
	|| last_channel <= -(1 << (input_sampling_power_of_two - 1)) || last_channel >= (1 << (input_sampling_power_of_two - 1)))
	{
		return(0);
	}
#else
	if(first_channel < 0 || first_channel >= (1 << (input_sampling_power_of_two - 1))
	|| last_channel < 0 || last_channel >= (1 << (input_sampling_power_of_two - 1)))
	{
		return(0);
	}
#endif

	if(!cos12) waterfall_cos12_start();

	if(first_channel > last_channel)
	{
		first_subchannel = last_channel;
		subchannels = 1 + first_channel - last_channel;
	}
	else
	{
		first_subchannel = first_channel;
		subchannels = 1 + last_channel - first_channel;
	}


	waterfall = (waterfall_t) calloc(1, sizeof(struct waterfall_struct) + subchannels * sizeof(struct waterfall_channel_struct));

	waterfall->buffer = (waterfall_input_t *) calloc(1 << input_sampling_power_of_two, sizeof(waterfall_input_t));

	waterfall->first_subchannel = first_subchannel;
	waterfall->subchannels = subchannels;
	waterfall->input_sampling_power_of_two = input_sampling_power_of_two;
	waterfall->samples = samples;
	waterfall->rows = rows;
	waterfall->cols = cols;
	
	for(i = 0; i < waterfall->subchannels; i++)
	{
		c = waterfall->channels + i;

		c->fist = morse_fist();
		c->colours = (db_t *) calloc(waterfall->samples, sizeof(db_t));
		c->decodes = (morse_decode_t *) calloc(waterfall->samples, sizeof(morse_decode_t));
		c->inputs = (db_t *) calloc(waterfall->samples, sizeof(db_t));
		c->text = (char *) calloc(waterfall->rows * waterfall->cols, sizeof(char));
		c->start = waterfall->samples;
	}

//	waterfall->working_fist = morse_fist();

	return(waterfall);
}

void waterfall_clear(waterfall_t waterfall, int subchannel)
{
	struct waterfall_channel_struct *c = 0;


	if(subchannel < waterfall->first_subchannel || subchannel >= waterfall->first_subchannel + waterfall->subchannels) return;

	c = waterfall->channels + subchannel - waterfall->first_subchannel;

	bzero(c->text, waterfall->rows * waterfall->cols * sizeof(char));
	
	c->text_end = 0;
}

const db_t *waterfall_colours(waterfall_t waterfall, int subchannel)
{
	struct waterfall_channel_struct *c = 0;


	if(subchannel < waterfall->first_subchannel || subchannel >= waterfall->first_subchannel + waterfall->subchannels) return(0);

	c = waterfall->channels + subchannel - waterfall->first_subchannel;

	return(c->colours);
}
static void waterfall_cos12_start(void)
{
	int i, *cos_array = 0;


	if(cos12) return;
	
	cos_array = (int *) calloc(1 << FFT_COS12_TABLE_BITS, sizeof(int));

	for(i = 0; i < (1 << FFT_COS12_TABLE_BITS); i++)
	{
		cos_array[i] = 0x0FFF * cos((2.0 * M_PI * i) / (1 << FFT_COS12_TABLE_BITS));
	}

	cos12 = cos_array;
}

void waterfall_dlete(waterfall_t waterfall)
{
	struct waterfall_channel_struct *c = 0;
	int i;


	for(i = 0; i < waterfall->subchannels; i++)
	{
		c = waterfall->channels + i;

		if(c->fist)
		{
			morse_fist_dlete(c->fist);
			c->fist = 0;
		}

		if(c->colours)
		{
			free(c->colours);
			c->colours = 0;
		}

		if(c->inputs)
		{
			free(c->inputs);
			c->inputs = 0;
		}

		if(c->decodes)
		{
			free(c->decodes);
			c->decodes = 0;
		}

		if(c->text)
		{
			free(c->text);
			c->text = 0;
		}
	}

	if(waterfall->buffer)
	{
		free(waterfall->buffer);
		waterfall->buffer = 0;
	}

	free(waterfall);
}

static db_t waterfall_fft_row(const waterfall_input_t *in, int logcount, int row)
{
	register const waterfall_input_t *in_ptr = in;
	register int angle;
	long long rl = 0, im = 0;
	int i;
	db_integer_t ret;


	for(i = (1 << logcount) - 1; i; i--)
	{
		angle = (0x0FFF * row * i) >> logcount;
#if defined(WATERFALL_COMPLEX_INPUT)
		rl += in_ptr->real * COS12(angle) - in_ptr->imag * SIN12(angle);
		im += in_ptr->imag * COS12(angle) + in_ptr->real * SIN12(angle);
#else
		rl += *(in_ptr) * COS12(angle);
		im += *(in_ptr) * SIN12(angle);
#endif
		in_ptr++;
	}

	ret = (rl * rl + im * im) >> (2 * logcount + 2 * FFT_COS12_TABLE_BITS);

	return(db_from_integer(ret));
}

const morse_fist_t waterfall_fist(waterfall_t waterfall, int subchannel)
{
	struct waterfall_channel_struct *c = 0;


	if(subchannel < waterfall->first_subchannel || subchannel >= waterfall->first_subchannel + waterfall->subchannels) return(0);

	c = waterfall->channels + subchannel - waterfall->first_subchannel;
	
	if(!c->fist) return(0);

	return(c->fist);
}

int waterfall_start(waterfall_t waterfall, int subchannel)
{
	struct waterfall_channel_struct *c = 0;


	if(subchannel < waterfall->first_subchannel || subchannel >= waterfall->first_subchannel + waterfall->subchannels) return(-1);

	c = waterfall->channels + subchannel - waterfall->first_subchannel;

	return(c->start);
}

const morse_decode_t *waterfall_symbols(waterfall_t waterfall, int subchannel)
{
	struct waterfall_channel_struct *c = 0;


	if(subchannel < waterfall->first_subchannel || subchannel >= waterfall->first_subchannel + waterfall->subchannels) return(0);

	c = waterfall->channels + subchannel - waterfall->first_subchannel;

	return(c->decodes);
}

int waterfall_sync(waterfall_t waterfall, int subchannel)
{
	struct waterfall_channel_struct *c = 0;
	unsigned int threshold, onoff_count, updates;
	int i;


	if(subchannel < waterfall->first_subchannel || subchannel >= waterfall->first_subchannel + waterfall->subchannels) return(0);
	
	threshold = db_from_integer(waterfall->average) + WATERFALL_THRESHOLD_DB;

	c = waterfall->channels + subchannel - waterfall->first_subchannel;
	
	if(c->updates)
	{
		do
		{
			updates = c->updates;

			memmove(c->colours, c->inputs, waterfall->samples * sizeof(*c->colours));
			
			while(waterfall_text_lines(waterfall, subchannel) >= waterfall->rows)
			{
				for(i = 0; c->text[i] && i < waterfall->cols && c->text[i] >= ' '; i++)
					;

//fprintf(stderr, "c->text=\"%s\",c->text+%d=\"%s\"\n", c->text, i + 1, c->text + i);

				strcpy(c->text, c->text + i + 1);
				c->text_end -= i + 1;
			}


			c->threshold = threshold;

			onoff_count = morse_decode(c->decodes, waterfall->samples, updates, c->colours + waterfall->samples - updates, updates, c->threshold, c->fist);
			
			if(onoff_count < WATERFALL_THRESHOLD_ONOFF)
			{
				bzero(c->fist, sizeof(*c->fist));
				onoff_count = morse_decode(c->decodes, waterfall->samples, 0, c->colours + waterfall->samples - updates, 0, c->threshold, c->fist);
			}
			
			if(onoff_count < WATERFALL_THRESHOLD_ONOFF)
			{
				for(i = 0; i < waterfall->samples && c->decodes[i].age; i++)
				{
					c->decodes[i].text = 0;
					c->decodes[i].whitespace = 0;
				}
				c->text[c->text_end] = 0;
			}
			else
			{
				morse_text(c->text + c->text_end, waterfall->rows * waterfall->cols - c->text_end, c->decodes, waterfall->samples);
				c->text_end += morse_trim_age(c->decodes, waterfall->samples, waterfall->samples);
			}

			c->updates -= updates;
		}
		while(c->updates);
	}
	
	return(0);
}

const char *waterfall_text(waterfall_t waterfall, int subchannel)
{
	struct waterfall_channel_struct *c = 0;


	if(subchannel < waterfall->first_subchannel || subchannel >= waterfall->first_subchannel + waterfall->subchannels) return(0);

	c = waterfall->channels + subchannel - waterfall->first_subchannel;

	return(c->text);
}

int waterfall_text_lines(waterfall_t waterfall, int subchannel)
{
	const char *text = waterfall_text(waterfall, subchannel);
	int ret = 1, i, cols = 0;


	if(!text || !*text) return(0);

	for(i = 0; text[i]; i++)
	{
		if(text[i] == '\n' || cols >= waterfall->cols)
		{
			ret++;
			cols = 0;
		}
		else if(text[i] >= ' ')
		{
			cols++;
		}
	}

	return(ret);
}

void waterfall_update(waterfall_t waterfall, const waterfall_input_t *input, int input_count)
{
	struct waterfall_channel_struct *c = 0;
	int i, subchannel, blocksize, sample_count = 0; //, characters = waterfall->rows * waterfall->cols, onoff_count;
	db_integer_t threshold = 0;


	if(!waterfall || !input || input_count <= 0) return;

	blocksize = 1 << waterfall->input_sampling_power_of_two;

	if(waterfall->buffer_count)
	{
		if(waterfall->buffer_count < blocksize)
		{
			i = input_count;
			if(i > blocksize - waterfall->buffer_count)
			{
				i = blocksize - waterfall->buffer_count;
			}
			memmove(waterfall->buffer + waterfall->buffer_count, input, i * sizeof(waterfall_input_t));
			input += i;
			input_count -= i;
		}

		if(waterfall->samples == blocksize)
		{
			waterfall_update_block(waterfall, waterfall->buffer);
			sample_count++;
		}
	}

	while(input_count >= blocksize)
	{
		waterfall_update_block(waterfall, input);
		sample_count++;
		input += blocksize;
		input_count -= blocksize;
	}

	if(input_count)
	{
		memmove(waterfall->buffer, input, input_count * sizeof(waterfall_input_t));
	}

	if(sample_count)
	{
		threshold = 0;

		for(subchannel = 0; subchannel < waterfall->subchannels; subchannel++)
		{
			c = waterfall->channels + subchannel;

			for(i = 0; i < waterfall->samples; i++)
			{
				if(c->inputs[i] > threshold) threshold = c->inputs[i];
			}
		}

//fprintf(stderr, "waterfall->threshold=%d\n", (int) waterfall->threshold);
	}
}

static db_integer_t waterfall_update_block(waterfall_t waterfall, const waterfall_input_t *block)
{
	struct waterfall_channel_struct *c = 0;
	db_integer_t ret = 0;
	int i, blocksize = (1 << waterfall->input_sampling_power_of_two);
#if defined(WATERFALL_FILTER_SIZE)
	int j;
#endif


	for(i = 0; i < blocksize; i++)
	{
#if defined(WATERFALL_COMPLEX_INPUT)
		ret += block[i].real * block[i].real + block[i].imag * block[i].imag;
#else
		ret += block[i] * block[i] * 2;
#endif
	}

	for(i = 0; i < waterfall->subchannels; i++)
	{
		c = waterfall->channels + i;
		memmove(c->inputs, c->inputs + 1, (waterfall->samples - 1) * sizeof(*c->inputs));
#if defined(WATERFALL_FILTER_SIZE)
		memmove(c->filter, c->filter + 1, (ARRAY_SIZE(c->filter) - 1) * sizeof(*c->filter));
		c->filter[ARRAY_SIZE(c->filter) - 1] = waterfall_fft_row(block, waterfall->input_sampling_power_of_two, i + waterfall->first_subchannel);
		c->inputs[waterfall->samples - 1] = 0;
		for(j = 0; j < (int) ARRAY_SIZE(c->filter); j++)
		{
			c->inputs[waterfall->samples - 1] = (c->inputs[waterfall->samples - 1] + (WATERFALL_FILTER_COEFFICIENT - 1) * c->filter[j]) / WATERFALL_FILTER_COEFFICIENT;
		}
#else
		c->inputs[waterfall->samples - 1] = waterfall_fft_row(block, waterfall->input_sampling_power_of_two, i + waterfall->first_subchannel);
#endif
		c->updates++;
	}
	
	ret /= (blocksize * waterfall->subchannels);

	waterfall->average = (WATERFALL_THRESHOLD_COEFFICIENT * waterfall->average + ret) / (WATERFALL_THRESHOLD_COEFFICIENT + 1);

//fprintf(stderr, "ret=%d,waterfall->average=%d\n", (int) ret, (int) waterfall->average);

	return(ret);
}



#if defined(TEST)

#include <stdio.h>

#include <SDL2/SDL.h>

static int assert_errors = 0;

#define ASSERT(test)	{if(!(test)) {fprintf(stderr, "%s:%d: test \"%s\" failed\n", __FILE__, __LINE__, #test); assert_errors++;}}


#define TEST_STRING_13 "MAJESTIC THIRTEEN"
#define TEST_STRING_23 "TWENTY THREE SKIDOO"

#define TEST_SAMPLES_PER_MIN	((60*6400)/128)

#define TEST_ONOFF_MAX		1000
#define TEST_SAMPLES_MAX	10000

#define TEST_WATERFALL_SAMPLES	100
#define TEST_UPDATE_SAMPLES		41

#define TEST_SAMPLE_LOG_BLOCK_SIZE	6
#define TEST_SAMPLE_BLOCK_SIZE		(1 << (TEST_SAMPLE_LOG_BLOCK_SIZE))

int test_encode_string(waterfall_input_t *samples, morse_fist_t fist)
{
	db_t cw13[TEST_SAMPLES_MAX];
	db_t cw23[TEST_SAMPLES_MAX];
	int count;


	if(!cos12) waterfall_cos12_start();
	
	count = morse_encode(cw13, ARRAY_SIZE(cw13), 1, TEST_STRING_13, fist);
	ASSERT(count < TEST_ONOFF_MAX);
	count = morse_encode(cw23, ARRAY_SIZE(cw23), 1, TEST_STRING_23, fist);
	ASSERT(count < TEST_ONOFF_MAX);

	for(count = 0; count < TEST_SAMPLES_MAX; count++)
	{
#if defined(WATERFALL_COMPLEX_INPUT)
		samples[count].real = ( 1 * cw13[count >> TEST_SAMPLE_LOG_BLOCK_SIZE] * COS12((count * 4096 * 13) >> TEST_SAMPLE_LOG_BLOCK_SIZE) 
						 + 100 * COS12((count * 4096 * 19) >> TEST_SAMPLE_LOG_BLOCK_SIZE)
						 + 2 * cw23[count >> TEST_SAMPLE_LOG_BLOCK_SIZE] * COS12((count * 4096 * 23) >> TEST_SAMPLE_LOG_BLOCK_SIZE)
						 + (random() & 0x00)) >> 15;
		samples[count].imag = ( 1 * cw13[count >> TEST_SAMPLE_LOG_BLOCK_SIZE] * SIN12((count * 4096 * 13) >> TEST_SAMPLE_LOG_BLOCK_SIZE) 
						 + 100 * SIN12((count * 4096 * 19) >> TEST_SAMPLE_LOG_BLOCK_SIZE)
						 + 2 * cw23[count >> TEST_SAMPLE_LOG_BLOCK_SIZE] * SIN12((count * 4096 * 23) >> TEST_SAMPLE_LOG_BLOCK_SIZE)
						 + (random() & 0x00)) >> 15;
		ASSERT(samples[count].real < +127);
		ASSERT(samples[count].real > -127);
		ASSERT(samples[count].imag < +127);
		ASSERT(samples[count].imag > -127);
if(assert_errors) fprintf(stderr, "samples[%d]={%d,%d}\n", count, samples[count].real, samples[count].imag);
#else
		samples[count] = ( 1 * cw13[count >> TEST_SAMPLE_LOG_BLOCK_SIZE] * SIN12((count * 4096 * 13) >> TEST_SAMPLE_LOG_BLOCK_SIZE) 
						 + 100 * COS12((count * 4096 * 19) >> TEST_SAMPLE_LOG_BLOCK_SIZE)
						 + 2 * cw23[count >> TEST_SAMPLE_LOG_BLOCK_SIZE] * SIN12((count * 4096 * 23) >> TEST_SAMPLE_LOG_BLOCK_SIZE)
						 + (random() & 0x00)) >> 15;
		ASSERT(samples[count] < +127);
		ASSERT(samples[count] > -127);
if(assert_errors) fprintf(stderr, "samples[%d]=%d\n", count, samples[count]);
#endif
	}

	return(count);
}

void test_print_colours(db_t *samples, int length)
{
	int i;

	for(i = 0; i < length; i++)
	{
		if(samples[i] > 127)
		{
			fputc('_', stderr);
		}
		else
		{
			fputc(' ', stderr);
		}
	}

	fputc('\n', stderr);		
}

void test_print_samples(const signed short *samples, int length)
{
	int i;

	for(i = 0; i < length; i++)
	{
		if(samples[i] > 0)
		{
			fputc('^', stderr);
		}
		else
		{
			fputc('v', stderr);
		}
	}

	fputc('\n', stderr);		
}

void test_print_symbols(int start, const morse_decode_t *symbols, int length)
{
	int i;


	for(i = 0; i < length && (symbols[i].mark || symbols[i].space); i++)
	{
		if(symbols[i].text)
		{
			fputc(symbols[i].text, stderr);
		}
		if(symbols[i].whitespace)
		{
//			fputc(symbols[i].whitespace, stderr);
			fputc(' ', stderr);
		}
	}

	fputc('\n', stderr);		
}

int main(void)
{
	morse_fist_t fist = morse_fist();
	waterfall_input_t samples[TEST_SAMPLES_MAX];
//	const unsigned char *colours = 0;
	waterfall_t w = 0;
	int count, i = 0;


// Set up received signal

	morse_fist_wpm_set(fist, TEST_SAMPLES_PER_MIN, 15, 15);
	
	test_encode_string(samples, fist);


//fprintf(stderr, "count=%d\n", count);

// actual tests

	w = waterfall(TEST_SAMPLE_LOG_BLOCK_SIZE, TEST_WATERFALL_SAMPLES, 12, 24, 80, 25);
	
	ASSERT(w);
	
	ASSERT(!waterfall_text(w, 0));
	ASSERT(!waterfall_text(w, 11));
	ASSERT(waterfall_text(w, 12));
	ASSERT(!*waterfall_text(w, 12));
	ASSERT(waterfall_text(w, 24));
	ASSERT(!waterfall_text(w, 25));

	count = 0;
	while(count < (int) ARRAY_SIZE(samples))
	{
		i = random() & 0xFF;

		if(count + i > (int) ARRAY_SIZE(samples))
		{
			i = (int) ARRAY_SIZE(samples) - count;
		}

		waterfall_update(w, samples + count, i);
		count += i;
	}

	waterfall_dlete(w);

	return(assert_errors);
}

#endif
