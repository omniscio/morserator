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

#include "complex.h"
#include "morse.h"


#define ARRAY_SIZE(array)	(sizeof(array)/sizeof(*array))

//#define MORSE_DEBUG_ONOFF -- enable this in the Makefile if it's needed

#define MORSE_ASCII_MIN		' '
#define MORSE_ASCII_MAX		'\x7F'
#define MORSE_LENGTH_MAX	10

/*
 * Assuming 62.5mS samples, these are lengths in samples for 75WPM
 */
#define MORSE_DIT			1
#define MORSE_DAH			3
#define MORSE_TID			1
#define MORSE_LETTER		3
#define MORSE_WORD			7

// PARIS: .--. .- .-. .. ...    10xDit(+1) 4xDah(+3) 9xSpace(-1) 4xLspace(-3) 1xWspace(-7) -> 50 
#define MORSE_PARIS_DITS 50

#define MORSE_HISTOGRAM_MAX		0x1000

typedef enum 
{
/* 0123456789 */
	MORSE_CODE_0=0,MORSE_CODE_1,MORSE_CODE_2,MORSE_CODE_3,MORSE_CODE_4,
	MORSE_CODE_5,MORSE_CODE_6,MORSE_CODE_7,MORSE_CODE_8,MORSE_CODE_9,
/* ABCDEFGHIJKLMNOPQRSTUVWXYZ */
	MORSE_CODE_A,MORSE_CODE_B,MORSE_CODE_C,MORSE_CODE_D,MORSE_CODE_E,
	MORSE_CODE_F,MORSE_CODE_G,MORSE_CODE_H,MORSE_CODE_I,MORSE_CODE_J,
	MORSE_CODE_K,MORSE_CODE_L,MORSE_CODE_M,MORSE_CODE_N,MORSE_CODE_O,
	MORSE_CODE_P,MORSE_CODE_Q,MORSE_CODE_R,MORSE_CODE_S,MORSE_CODE_T,
	MORSE_CODE_U,MORSE_CODE_V,MORSE_CODE_W,MORSE_CODE_X,MORSE_CODE_Y,
	MORSE_CODE_Z,
/* .,?'/():=+-"@ */
	MORSE_CODE_PERIOD,
	MORSE_CODE_COMMA,
	MORSE_CODE_QUESTION,
	MORSE_CODE_APOSTROPHE,
	MORSE_CODE_SLASH,
	MORSE_CODE_BRACKET_OPEN,
	MORSE_CODE_BRACKET_CLOSE,
	MORSE_CODE_COLON,
	MORSE_CODE_EQUAL,
	MORSE_CODE_PLUS,
	MORSE_CODE_DASH,
	MORSE_CODE_QUOTE,
	MORSE_CODE_AT,
/* !&;_$ */
	MORSE_CODE_EXCLAMATION,
	MORSE_CODE_AMPERSAND,
	MORSE_CODE_SEMICOLON,
	MORSE_CODE_UNDERSCORE,
	MORSE_CODE_DOLLAR,

/* space is last */
	MORSE_CODE_SPACE,

#define MORSE_CODE_MAX	MORSE_CODE_NONE
	MORSE_CODE_NONE
}
morse_code_t;

static morse_code_t morse_code_from_ascii(char character);
static char morse_code_to_ascii(morse_code_t code);
static int morse_code_onoff(morse_decode_t *onoff, int onoff_length, morse_code_t code, morse_fist_t fist);
int morse_decode(morse_decode_t *output, int output_size, morse_time_t output_offset, const db_t *input, int input_size, db_t threshold, morse_fist_t fist);
static int morse_decode_fist(morse_fist_t fist, const morse_decode_t *input, int input_size);
static int morse_decode_length(morse_decode_t *output, int output_size);
static int morse_decode_onoff(morse_decode_t *output, int output_size, morse_time_t output_offset, const db_t *input, int input_size, db_t threshold);
static int morse_decode_text(morse_decode_t *output, int output_size, morse_fist_t fist);
static db_t morse_decode_threshold(const db_t *input, int input_size);
int morse_encode(db_t *output, int output_size, db_t mark, const char *string, morse_fist_t fist);
morse_fist_t morse_fist(void);
void morse_fist_dlete(morse_fist_t fist);
void morse_fist_wpm_set(morse_fist_t fist, int sample_per_min, int wpm, int farnsworth_wpm);
int morse_fist_wpm_get(morse_fist_t fist, int sample_per_min);

int morse_text(char *text, int text_size, const morse_decode_t *output, int output_size);
int morse_trim(morse_decode_t *output, int output_size, int trim_characters);
int morse_trim_age(morse_decode_t *output, int output_size, morse_time_t age);



static const char *morse_codes[MORSE_CODE_MAX]=
{
/* 0123456789 */
	"-----", ".----", "..---", "...--", "....-", 
	".....", "-....", "--...", "---..", "----.",
/* ABCDEFGHIJKLMNOPQRSTUVWXYZ */
	".-",   "-...", "-.-.", "-..",  ".",
	"..-.", "--.",  "....", "..",   ".---",
	"-.-",  ".-..", "--",   "-.",   "---",
	".--.", "--.-", ".-.",  "...",  "-",
	"..-",  "...-", ".--",  "-..-", "-.--",
	"--..",
/* .,?'/():=+-"@ */
	".-.-.-", "--..--", "..--..", ".----.", "-..-.",
	"-.--.", "-.--.-",  "---...", "-...-", ".-.-.",
	"-....-", ".-..-.", ".--.-.",
/* !&;_$ */
	"-.-.--", ".-...", "-.-.-.", "..--.-", "...-..-",

// TODO: prosigns

/* space is last */
	"/"
};

static morse_code_t morse_ascii[MORSE_ASCII_MAX - MORSE_ASCII_MIN + 1]=
{
/*  !"#$%&'()*+,-./ */
	MORSE_CODE_SPACE,
	MORSE_CODE_EXCLAMATION,
	MORSE_CODE_QUOTE,
	MORSE_CODE_NONE,
	MORSE_CODE_DOLLAR,
	MORSE_CODE_NONE,
	MORSE_CODE_AMPERSAND,
	MORSE_CODE_APOSTROPHE,
	MORSE_CODE_BRACKET_OPEN,
	MORSE_CODE_BRACKET_CLOSE,
	MORSE_CODE_NONE,
	MORSE_CODE_PLUS,
	MORSE_CODE_COMMA,
	MORSE_CODE_DASH,
	MORSE_CODE_PERIOD,
	MORSE_CODE_SLASH,

/* 0123456789:;<=>? */
	MORSE_CODE_0,
	MORSE_CODE_1,
	MORSE_CODE_2,
	MORSE_CODE_3,
	MORSE_CODE_4,
	MORSE_CODE_5,
	MORSE_CODE_6,
	MORSE_CODE_7,
	MORSE_CODE_8,
	MORSE_CODE_9,
	MORSE_CODE_COLON,
	MORSE_CODE_SEMICOLON,
	MORSE_CODE_BRACKET_OPEN,
	MORSE_CODE_EQUAL,
	MORSE_CODE_BRACKET_CLOSE,
	MORSE_CODE_QUESTION,

/* @ABCDEFGHIJKLMNO */
	MORSE_CODE_AT,
	MORSE_CODE_A,
	MORSE_CODE_B,
	MORSE_CODE_C,
	MORSE_CODE_D,
	MORSE_CODE_E,
	MORSE_CODE_F,
	MORSE_CODE_G,
	MORSE_CODE_H,
	MORSE_CODE_I,
	MORSE_CODE_J,
	MORSE_CODE_K,
	MORSE_CODE_L,
	MORSE_CODE_M,
	MORSE_CODE_N,
	MORSE_CODE_O,

/* PQRSTUVWXYZ[\]^_ */
	MORSE_CODE_P,
	MORSE_CODE_Q,
	MORSE_CODE_R,
	MORSE_CODE_S,
	MORSE_CODE_T,
	MORSE_CODE_U,
	MORSE_CODE_V,
	MORSE_CODE_W,
	MORSE_CODE_X,
	MORSE_CODE_Y,
	MORSE_CODE_Z,
	MORSE_CODE_BRACKET_OPEN,
	MORSE_CODE_NONE,
	MORSE_CODE_BRACKET_CLOSE,
	MORSE_CODE_NONE,
	MORSE_CODE_UNDERSCORE,

/* `abcdefghijklmno */
	MORSE_CODE_APOSTROPHE,
	MORSE_CODE_A,
	MORSE_CODE_B,
	MORSE_CODE_C,
	MORSE_CODE_D,
	MORSE_CODE_E,
	MORSE_CODE_F,
	MORSE_CODE_G,
	MORSE_CODE_H,
	MORSE_CODE_I,
	MORSE_CODE_J,
	MORSE_CODE_K,
	MORSE_CODE_L,
	MORSE_CODE_M,
	MORSE_CODE_N,
	MORSE_CODE_O,

/* pqrstuvwxyz{|}~\x7F  */
	MORSE_CODE_P,
	MORSE_CODE_Q,
	MORSE_CODE_R,
	MORSE_CODE_S,
	MORSE_CODE_T,
	MORSE_CODE_U,
	MORSE_CODE_V,
	MORSE_CODE_W,
	MORSE_CODE_X,
	MORSE_CODE_Y,
	MORSE_CODE_Z,
	MORSE_CODE_BRACKET_OPEN,
	MORSE_CODE_NONE,
	MORSE_CODE_BRACKET_CLOSE,
	MORSE_CODE_NONE,
	MORSE_CODE_NONE,
};



static morse_code_t morse_code_from_ascii(char character)
{
	if(character >= MORSE_ASCII_MIN && character <= MORSE_ASCII_MAX && morse_ascii[character - MORSE_ASCII_MIN] != MORSE_CODE_NONE)
	{
		return(morse_ascii[character - MORSE_ASCII_MIN]);
	}

// we assume all out-of-range ASCII is whitespace
	return(MORSE_CODE_SPACE);
}

static char morse_code_to_ascii(morse_code_t code)
{
	int i;

	if(code != MORSE_CODE_NONE)
	{
		for(i = 0; i < MORSE_ASCII_MAX - MORSE_ASCII_MIN; i++)
		{
			if(code == morse_ascii[i])
			{
				return(i + MORSE_ASCII_MIN);
			} 
		}
	}

	return(0);
}

static int morse_code_onoff(morse_decode_t *onoff, int onoff_length, morse_code_t code, morse_fist_t fist)
{
	morse_decode_t temp[MORSE_LENGTH_MAX];
	int ret = 0, i;


	if(!onoff || onoff_length < MORSE_LENGTH_MAX)
	{
		onoff = temp;
		onoff_length = ARRAY_SIZE(temp);
	}

	if(code == MORSE_CODE_NONE)
	{
		code = MORSE_CODE_QUESTION;
	}

	if(code != MORSE_CODE_SPACE)
	{
		for(i = 0; morse_codes[code][i]; i++)
		{
			switch(morse_codes[code][i])
			{
			case '.':
				onoff[ret].mark = fist->dit;
				onoff[ret].space = fist->tid;
				break;

			case '-':
				onoff[ret].mark = fist->dah;
				onoff[ret].space = fist->tid;
				break;

			default:
				break;
			}
			ret++;
		}

		if(ret > 0)
		{
			onoff[ret - 1].space += fist->letter;
		}
	}

	return(ret);
}


int morse_decode(morse_decode_t *output, int output_size, morse_time_t output_offset, const db_t *input, int input_size, db_t threshold, morse_fist_t fist)
{
	struct morse_fist_struct x;
	int ret;


	if(!threshold) threshold = morse_decode_threshold(input, input_size);

	ret = morse_decode_onoff(output, output_size, output_offset, input, input_size, threshold);

	if(ret > 10)
	{
		if(!fist)
		{
			bzero(&x, sizeof(x));
			fist = &x;
		}

		if(!fist->dit || !fist->dah || !fist->tid || !fist->letter/* || !fist->word*/)
		{
			morse_decode_fist(fist, output, ret);
		}

		if(morse_decode_text(output, output_size, fist))
		{
			morse_decode_fist(fist, output, ret);
			morse_decode_text(output, output_size, fist);
		}
	}

	return(ret);
}

static int morse_decode_fist(morse_fist_t fist, const morse_decode_t *input, int input_size)
{
	unsigned int histogram[MORSE_HISTOGRAM_MAX];
	unsigned int i, count, average;


	if(fist)
	{
		bzero(fist, sizeof(*fist));

// histogram of marks gives dits/dahs
		bzero(histogram, sizeof(histogram));
		average = 0; count = 0;
		for(i = 0; i < (unsigned int) input_size && input[i].age; i++)
		{
			if(input[i].mark)
			{
				if(input[i].mark < ARRAY_SIZE(histogram)) histogram[input[i].mark]++;
				average += input[i].mark;
				count++;
			}
		}

//fprintf(stderr, "average=%d,score1=%d\n", average, score1);
		average = (average + count / 2) / count;
		for(i = ARRAY_SIZE(histogram) - 1; i > 0; i--)
		{
			if(i <= average && histogram[i] >= histogram[fist->dit])
			{
//fprintf(stderr, "average=%d,histogram[%d]=%d,score1=%d,fist->dit=%d\n", average, i, histogram[i], score1, (int) fist->dit);
				fist->dit = i;
			}
			else if(i > average && histogram[i] >= histogram[fist->dah])
			{
//fprintf(stderr, "average=%d,histogram[%d]=%d,score2=%d,fist->dah=%d\n", average, i, histogram[i], score2, (int) fist->dah);
				fist->dah = i;
			}
		}

/* sanity check marks */
		if(!fist->dit)
		{
			if(fist->dah > 1)
			{
				fist->dit = (fist->dah + 2) / 3;
			}
			else
			{
				fist->dah = 0;
			}
		}
		else if(!fist->dah)
		{
			fist->dah = fist->dit * 3;
		}
		else if(fist->dah < fist->dit * 3)
		{
			fist->dah = fist->dit * 3;
		}

		if(!fist->dit || !fist->dah)
		{
			return(0);
		}

		bzero(histogram, sizeof(histogram));
		average = 0; count = 0;
		for(i = 0; i < (unsigned int) input_size && input[i].age; i++)
		{
			if(input[i].space && input[i].space < fist->dit + fist->dah)
			{
				if(input[i].space < ARRAY_SIZE(histogram)) histogram[input[i].space]++;
				average += input[i].space;
				count++;
			}
		}

//fprintf(stderr, "average=%d,score1=%d\n", average, score1);
		average = (average + count / 2) / count;
		for(i = ARRAY_SIZE(histogram) - 1; i > 0; i--)
		{
			if(i < average && histogram[i] > histogram[fist->tid])
			{
//fprintf(stderr, "average=%d,histogram[%d]=%d,score1=%d,fist->dit=%d\n", average, i, histogram[i], score1, (int) fist->dit);
				fist->tid = i;
			}
			else if(i >= average && histogram[i] >= histogram[fist->letter])
			{
//fprintf(stderr, "average=%d,histogram[%d]=%d,score2=%d,fist->dah=%d\n", average, i, histogram[i], score2, (int) fist->dah);
				fist->letter = i;
			}
		}

/* sanity check spaces */
		if(!fist->tid)
		{
			fist->tid = fist->dit;
		}

		if(fist->letter < fist->tid * 3)
		{
			fist->letter = fist->tid * 3;
		}

// overwrite spaces with mark-derived values because it gives better decodes of real people :-)
fist->tid = fist->dit;
fist->letter = (fist->tid * 5 + 1) / 2;
	}

	return(0);
}

static int morse_decode_length(morse_decode_t *output, int output_size)
{
	int ret;


	for(ret = 0; (ret < output_size || output_size <= 0) && (output[ret].mark || output[ret].space); ret++)
		;

	return(ret);
}


static int morse_decode_onoff(morse_decode_t *output, int output_size, morse_time_t output_offset, const db_t *input, int input_size, db_t threshold)
{
	int i, mark = 0, ret = 0;
	db_t last;


	if(!threshold) threshold = 3;

	if(output_size > 0)
	{
		ret = morse_decode_length(output, output_size);
		bzero(output + ret, (output_size - ret) * sizeof(*output));
	}
	
	if(!ret)
	{
		last = *input;
		mark = 1;
	}
	else
	{
		last = output[--ret].snr;

		if(output[ret].space)
		{
			mark = 0;
		}
		else
		{
			mark = 1;
		}
	}

	for(i = 0; i < input_size; i++)
	{
		if(input[i] > threshold)
		{
			if(!mark) ret++;
			mark = 1;
		}
		else
		{
			mark = 0;
		}
	
		last = input[i];

		if(ret < output_size)
		{
			if(mark)
			{
				output[ret].mark++;
			}
			else
			{
				output[ret].space++;
			}
			
			output[ret].snr = last;
			output[ret].age = output[ret].mark + output[ret].space;
		}
	}


	for(i = ret; i >= 0; i--)
	{
		if(i + 1 < output_size)
		{
			output[i].age = output[i].mark + output[i].space + output[i + 1].age;
		}
		else
		{
			output[i].age = output[i].mark + output[i].space;
		}
	}

	return(ret + 1);
}

static int morse_decode_text(morse_decode_t *output, int output_size, morse_fist_t fist)
#if !defined(MORSE_DEBUG_ONOFF)
{
	morse_decode_t matches[MORSE_CODE_MAX][MORSE_LENGTH_MAX];
	int matchlengths[MORSE_CODE_MAX];
	int count = 0, ret = 0, i, j, best, tones = 0, letters;
	morse_time_t x, score, this_score;
	int average_length = 0, hits = 0;


	bzero(matches, sizeof(matches));

	for(i = 0; i < output_size; i++)
	{
		output[i].text = 0;
		output[i].whitespace = 0;
	}

	for(i = 0; i < MORSE_CODE_MAX; i++)
	{
		matchlengths[i] = morse_code_onoff(matches[i], MORSE_LENGTH_MAX, (morse_code_t) i, fist);
	}

	if(output_size > 0)
	{
		count = morse_decode_length(output, output_size);
	}

	for(tones = 0; tones < count; tones++)
	{
		best = MORSE_CODE_MAX;
		
		for(letters = 0; letters < MORSE_LENGTH_MAX && tones + letters < count && output[tones + letters].space < fist->letter; letters++)
			;

		letters++;
		
		for(i = 0; i < MORSE_CODE_MAX && matchlengths[i]; i++)
		{
			this_score = 0;

			if(matchlengths[i] == letters && tones + matchlengths[i] <= count)
			{
				for(j = 0; j < matchlengths[i]; j++)
				{
					if(output[tones + j].mark > matches[i][j].mark)
					{
						x = output[tones + j].mark - matches[i][j].mark;
					}
					else
					{
						x = matches[i][j].mark - output[tones + j].mark;
					}
					this_score += x * x;
//					if(j < matchlengths[i - 1])
					{
						if(output[tones + j].space > matches[i][j].space)
						{
							x = output[tones + j].space - matches[i][j].space;
						}
						else
						{
							x = matches[i][j].space - output[tones + j].space;
						}
						this_score += x * x;
					}
				}

				if(this_score < score || best == MORSE_CODE_MAX)
				{
					score = this_score;
					best = i;
				}
			}
		}


		if(best < MORSE_CODE_MAX)
		{
			average_length += matchlengths[best];
			hits++;
//for(j = 0; j < matchlengths[best]; j++) fprintf(stderr, " output[tones+%d=%d]={mark=%d,space=%d}\n", j, tones + j, (int) output[tones + j].mark, (int) output[tones + j].space);
			tones += matchlengths[best] - 1;
//fprintf(stderr, "tones=%d, morse_code_to_ascii(best=%d)=\'%c\',score=%d, output[tones=%d].space=%d\n", tones, best, morse_code_to_ascii((morse_code_t) best), (int) score, tones, (int) output[tones].space);
			output[tones].text = morse_code_to_ascii((morse_code_t) best);
			x = output[tones].space;
			if(x > fist->letter * 4)
			{
				output[tones].whitespace = '\n';
			}
			else if(x >= fist->letter * 2)
			{
				output[tones].whitespace = ' ';
			}
		}
		else
		{
			ret++;
		}
	}
	
	if(average_length / hits < 3)
	{
		ret = -1;
	}

	return(ret);
}
#else
{
	int i;


	for(i = 0; i < output_size; i++)
	{
		output[i].text = '|';
		output[i].whitespace = 0;
	}

	return(0);
}
#endif


static db_t morse_decode_threshold(const db_t *input, int input_size)
{
	int total = 0, i;
	int hi = 0, hi_best = 0, lo = 0, lo_best = 0;
	db_t histogram[1<<(sizeof(db_t)*8)];


	bzero(histogram, sizeof(histogram));

	for(i = 0; i < input_size; i++)
	{
		histogram[input[i]]++;
		total += input[i];
	}
	
	total = (total + input_size / 2) / input_size;

	for(i = 0; i < (int) (sizeof(histogram)/sizeof(*histogram)); i++)
	{
		if(i < total)
		{
			if(histogram[i] > lo_best)
			{
				lo_best = histogram[i];
				lo = i;
			}
		}
		else
		{
			if(histogram[i] > hi_best)
			{
				hi_best = histogram[i];
				hi = i;
			}
		}
	}


//fprintf(stderr, "morse_decode_threshold(input,%d) returns %d,hi=%d,lo=%d\n", input_size, (3 * (hi - lo)) / 4, hi, lo);
//	return((db_t) (3 * (hi - lo)) / 4);	

//fprintf(stderr, "morse_decode_threshold(input,%d) returns %d,hi=%d,lo=%d\n", input_size, ((hi + lo) / 2), hi, lo);
	return((db_t) ((hi + lo) / 2));
}

int morse_encode(db_t *output, int output_size, db_t mark, const char *string, morse_fist_t fist)
{
	int ret = 0;
	unsigned int i, j, k;
	morse_code_t code;
	const char *morse_pattern = 0;


	if(string && fist)
	{
		for(k = 0; string[k]; k++)
		{
			code = morse_code_from_ascii(string[k]);

			if(k && code == MORSE_CODE_SPACE)
			{
				for(i = 0; i < fist->tid + fist->letter; i++)
				{
					if(ret < output_size) output[ret] = 0;
					ret++;
				}
			}
			else
			{
				morse_pattern = morse_codes[code];
				
				if(morse_pattern)
				{
					for(j = 0; morse_pattern[j]; j++)
					{
						if(j)
						{
							for(i = 0; i < fist->tid; i++)
							{
								if(ret < output_size) output[ret] = 0;
								ret++;
							}
						}

						switch(morse_pattern[j])
						{
						case '.':
							for(i = 0; i < fist->dit; i++)
							{
								if(ret < output_size) output[ret] = mark;
								ret++;
							}
						break;

						case '-':
							for(i = 0; i < fist->dah; i++)
							{
								if(ret < output_size) output[ret] = mark;
								ret++;
							}
						break;

						default:
							break;
						}
					}
				}

				for(i = 0; i < fist->letter; i++)
				{
					if(ret < output_size) output[ret] = 0;
					ret++;
				}
			}
		}
	}

	return(ret);
}


morse_fist_t morse_fist(void)
{
	morse_fist_t fist = (morse_fist_t) calloc(1, sizeof(struct morse_fist_struct));

	if(fist)
	{
		morse_fist_wpm_set(fist, MORSE_PARIS_DITS, 1, 1);
	}

	return(fist);
}

void morse_fist_dlete(morse_fist_t fist)
{
	free(fist);
}

void morse_fist_wpm_set(morse_fist_t fist, int sample_per_min, int wpm, int farnsworth_wpm)
{
	if(fist)
	{
		fist->dit = (MORSE_DIT * sample_per_min) / (wpm * MORSE_PARIS_DITS);
		fist->dah = (MORSE_DAH * sample_per_min) / (wpm * MORSE_PARIS_DITS);
		fist->tid = (MORSE_TID * sample_per_min) / (wpm * MORSE_PARIS_DITS);
// TODO: this calculation is not right
		fist->letter = (MORSE_LETTER * sample_per_min) / (farnsworth_wpm * MORSE_PARIS_DITS);
//		fist->word = (MORSE_WORD * sample_per_min) / (farnsworth_wpm * MORSE_PARIS_DITS);
	}
}


int morse_fist_wpm_get(morse_fist_t fist, int sample_per_min)
{
//fprintf(stderr, "fist={dit=%d,dah=%d,tid=%d,letter=%d,word=%d}\n", fist->dit, fist->dah, fist->tid, fist->letter, fist->word);

	if(fist && fist->dit > 0 && fist->dah > 0 && fist->letter > 0 /*&& fist->word > 0*/)
	{
// PARIS: .--. .- .-. .. ...    10xDit(+1) 4xDah(+3) 9xSpace(-1) 4xLspace(-3) 1xWspace(-7) -> 50 
		return((sample_per_min)/(fist->dit * 10 + fist->dah * 4 + fist->tid * (9 + 1) + fist->letter * (4 + 2)));
	}

	return(0);
}

int morse_text(char *text, int text_size, const morse_decode_t *output, int output_size)
{
	int i, ret = 0;


	bzero(text, text_size * sizeof(*text));

	for(i = 0; i < output_size; i++)
	{
		if(output[i].text)
		{
			if(ret + 1 < text_size) text[ret] = output[i].text;
			ret++;
			if(output[i].whitespace)
			{
				if(ret + 1 < text_size) text[ret] = output[i].whitespace;
				ret++;
			}
			
		}
	}

	return(ret);
}

int morse_trim(morse_decode_t *output, int output_size, int trim_characters)
{
	int i, last = -1, chars = 0;


	for(i = 0; i < output_size && (output[i].mark || output[i].space); i++)
	{
		if(output[i].text)
		{
			if(chars < trim_characters)
			{
				last = i;
			}
			chars++;
			if(output[i].whitespace)
			{
				chars++;
			}
		}
	}
	
	if(!trim_characters || last < 0)
	{
		return(chars);
	}

	if(chars <= trim_characters)
	{ // all characters must be trimmed
		bzero(output, output_size * sizeof(*output));
		return(0);
	}

//	if(output[last].whitespace) chars--;
	memmove(output, output + last + 1, (output_size - last - 1) * sizeof(output));
	bzero(output + output_size - last - 1, (last + 1) * sizeof(*output));

	return(chars - trim_characters);
}

int morse_trim_age(morse_decode_t *output, int output_size, morse_time_t age)
{
	int i, ret = 0;


	for(i = 0; i < output_size && output[i].age > age; i++)
	{
		if(output[i].text)
		{
			ret++;
			if(output[i].whitespace)
			{
				ret++;
			}
		}
	}

	if(ret)
	{
		morse_trim(output, output_size, ret);
	}

	return(ret);
}


#if defined(TEST)

#include <stdio.h>
#include <string.h>

static int assert_errors = 0;

#define ASSERT(test)	{if(!(test)) {fprintf(stderr, "%s:%d: test \"%s\" failed\n", __FILE__, __LINE__, #test); assert_errors++;}}

#define TEST_STRING "The quick brown fox jumps over the lazy dog.\n"\
					"THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG!\n"\
					"The first numbers in the Fibonacci sequence are:"\
					" 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610\n"

#define TEST_OUT 	"THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG. "\
					"THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG! "\
					"THE FIRST NUMBERS IN THE FIBONACCI SEQUENCE ARE:"\
					" 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610 "

#define TEST_SAMPLES_PER_MIN	((60*8000)/128)

#define TEST_DECODE_MAX	10000
static morse_decode_t test_decode[TEST_DECODE_MAX];
static db_t test_db[TEST_DECODE_MAX];
static char test_string[TEST_DECODE_MAX];

static void test_print_onoff(FILE *f, const db_t *output, db_t threshold, int count, const morse_decode_t *decode);

static void test_print_decode(FILE *f, morse_decode_t *decode, int length, int mag)
{
	int i, j;


	if(mag < 1) mag = 1;

	fprintf(f, "%d: |", length);

	for(i = 0; i < length; i++)
	{
		for(j = 0; j < (int) decode[i].mark; j += mag)
		{
			fputc('-', f);
		}
		for(j = 0; j < (int) decode[i].space; j += mag)
		{
			if(!j && decode[i].text)
			{
				fputc(decode[i].text, f);
			}
			else
			{
				fputc(' ', f);
			}
		}
		
		if(!decode[i].mark && !decode[i].space) break;
	}

	fputc('|', f);
	fputc('\n', f);
}

static const char *test_callsign(void)
{
	static char callsign[10];
	int ptr = 0;

	callsign[ptr++] = 'A' + (random() % 26);
	if((random() & 0x4000) < 250)
	{
		callsign[ptr++] = 'A' + (random() % 26);
	}
	callsign[ptr++] = '0' + (random() % 10);
	callsign[ptr++] = 'A' + (random() % 26);
	callsign[ptr++] = 'A' + (random() % 26);
	if((random() & 0x4000) < 900)
	{
		callsign[ptr++] = 'A' + (random() % 26);
	}
	callsign[ptr++] = ' ';
	callsign[ptr] = 0;

	return(callsign);
}

#define TEST_MAX (8000 * 4)
#define TEST_NOISE_FACTOR(n)	((n) * (random() & 0xFF))
#define TEST_NOISE_COUNT		100

static int test_noise(int percent_noise)
{
	db_t signal[TEST_MAX], signal_noise[TEST_MAX];
	morse_decode_t decode[TEST_MAX];
	unsigned int i, j, decoded = 0;
	const char *tx_callsign = 0;
	char rx_callsign[10];
	morse_fist_t tx_fist = morse_fist(), rx_fist = morse_fist();
	unsigned int length;


	tx_fist->dit = 3;
	tx_fist->dah = tx_fist->dit * 3;
	tx_fist->tid = tx_fist->dit;
	tx_fist->letter = tx_fist->dit * 3;
//	tx_fist->word = tx_fist->dit * 7;

	for(i = 0; i < TEST_NOISE_COUNT; i++)
	{
		tx_callsign = test_callsign();
		length = morse_encode(signal, ARRAY_SIZE(signal), 0xFF, tx_callsign, tx_fist);

		for(j = 0; j < length; j++)
		{
			signal_noise[j] = db_from_integer(signal[j] * 100 + (0xFF & random()) * percent_noise);
		}
		
		bzero(rx_fist, sizeof(*rx_fist));
		bzero(decode, sizeof(decode));

		length = morse_decode(decode, ARRAY_SIZE(decode), 0, signal_noise, length, 0, rx_fist);
//fprintf(stderr, "fist={dit=%d,dah=%d,tid=%d,letter=%d,word=%d}\n", (int) rx_fist->dit, (int) rx_fist->dah, (int) rx_fist->tid, (int) rx_fist->letter, (int) rx_fist->word); 
		
		morse_text(rx_callsign, ARRAY_SIZE(rx_callsign), decode, length);

//		ASSERT(!strcmp(rx_callsign, tx_callsign));

if(assert_errors) fprintf(stderr, "tx_callsign=\"%s\",rx_callsign=\"%s\"\n", tx_callsign, rx_callsign);
if(assert_errors) test_print_decode(stderr, decode, ARRAY_SIZE(decode), 1);
		
//if(assert_errors) test_print_onoff(stderr, signal, 100, ARRAY_SIZE(signal), decode);
//if(assert_errors) fprintf(stderr, "Test %d: tx_fist={dit=%d,dah=%d,tid=%d,letter=%d,word=%d}\n", i, (int) tx_fist->dit, (int) tx_fist->dah, (int) tx_fist->tid, (int) tx_fist->letter, (int) tx_fist->word);
//if(assert_errors) fprintf(stderr, "Test %d: rx_fist={dit=%d,dah=%d,tid=%d,letter=%d,word=%d}\n", i, (int) rx_fist->dit, (int) rx_fist->dah, (int) rx_fist->tid, (int) rx_fist->letter, (int) rx_fist->word);
//if(assert_errors) fprintf(stderr, "Test %d: threshold=%d,tx_callsign=\"%s\",rx_callsign=\"%s\"\n", i, threshold, tx_callsign, rx_callsign);

		if(!strcmp(rx_callsign, tx_callsign))
		{
			decoded++;
		}
	}

//fprintf(stderr, "SNR=%d,decoded=%d%%\n", 20 - db_from_integer(percent_noise), (decoded * 100) / TEST_NOISE_COUNT); 

	morse_fist_dlete(rx_fist);
	morse_fist_dlete(tx_fist);

	return((decoded * 1000) / TEST_NOISE_COUNT);
}

static void test_print_onoff(FILE *f, const db_t *output, db_t threshold, int count, const morse_decode_t *decode)
{
	int i, j;


	fprintf(f, "count=%d: ", count);
	
	for(i = 0; i < count; i++)
	{
		if(output[i] >= threshold)
		{
			putc('_', f);
		}
		else
		{
			putc(' ', f);
		}
	}

	fputc('\n', f);

	fprintf(f, "count=%d: ", count);

	if(decode)
	{
		for(i = 0; decode[i].mark || decode[i].space; i++)
		{
			for(j = 0; j < (int) decode[i].mark; j++)
			{
				putc('_', f);
			}

			for(j = 0; j < (int) decode[i].space; j++)
			{
				if(!j && decode[i].text)
				{
					putc(decode[i].text, f);
				}
				else
				{
					putc(' ', f);
				}
			}
		}

		fputc('\n', f);
	}
}

int main(void)
{
	morse_fist_t fist = morse_fist();
	unsigned int count, i, fragment;


// Test MORSE_LENGTH_MAX is valid

	for(count = MORSE_CODE_0; count < MORSE_CODE_NONE; count++)
	{
		ASSERT(strlen(morse_codes[count]) < MORSE_LENGTH_MAX);
	}


// Test fist functions
	ASSERT(fist);
	ASSERT(morse_fist_wpm_get(fist, TEST_SAMPLES_PER_MIN) == 75);
	morse_fist_wpm_set(fist, TEST_SAMPLES_PER_MIN, 25, 25);
	ASSERT(morse_fist_wpm_get(fist, TEST_SAMPLES_PER_MIN) == 25);
	morse_fist_wpm_set(fist, TEST_SAMPLES_PER_MIN, 75, 75);
	ASSERT(morse_fist_wpm_get(fist, TEST_SAMPLES_PER_MIN) == 75);


// Morse coding/decoding tests	
	
	count = morse_encode(test_db, ARRAY_SIZE(test_db), 0x80, TEST_STRING, fist);
	
	ASSERT(count < TEST_DECODE_MAX && count > 0);
	
if(assert_errors) test_print_onoff(stdout, test_db, 0x40, count, 0); 

	bzero(fist, sizeof(struct morse_fist_struct));
	
	bzero(test_decode, sizeof(test_decode));
	count = morse_decode(test_decode, ARRAY_SIZE(test_decode), 0, test_db, count, 0, fist);
	
	ASSERT(count < TEST_DECODE_MAX && count > 0);	
	ASSERT(fist->dit == MORSE_DIT);
	ASSERT(fist->dah == MORSE_DAH);
	ASSERT(fist->tid == MORSE_TID);
	ASSERT(fist->letter == MORSE_LETTER);
//	ASSERT(fist->word == MORSE_WORD);

if(assert_errors) fprintf(stderr, "fist={dit=%d,dah=%d,tid=%d,letter=%d}\n", (int) fist->dit, (int) fist->dah, (int) fist->tid, (int) fist->letter);
if(assert_errors) test_print_onoff(stderr, test_db, 0x40, count, test_decode); 

	ASSERT(count = morse_text(test_string, ARRAY_SIZE(test_string), test_decode, ARRAY_SIZE(test_decode))); 
	ASSERT(count == strlen(TEST_OUT));
	ASSERT(!strcmp(test_string, TEST_OUT));

//return(1);

// test chopping the input

	count = morse_encode(test_db, ARRAY_SIZE(test_db), 0x80, TEST_STRING, fist);
	ASSERT(count < TEST_DECODE_MAX && count > 0);	
	bzero(test_decode, sizeof(test_decode));	
	i = 0;
	do
	{
		fragment = (0x0F & random());
		if(fragment + i > count)
		{
			fragment = count - i;
		}
		morse_decode(test_decode, ARRAY_SIZE(test_decode), 0, test_db + i, fragment, 3, fist);
		i += fragment;
	}
	while(i < count);
	ASSERT(count = morse_text(test_string, ARRAY_SIZE(test_string), test_decode, ARRAY_SIZE(test_decode))); 
	ASSERT(count == strlen(TEST_OUT));
	ASSERT(!strcmp(test_string, TEST_OUT));
if(assert_errors) test_print_onoff(stderr, test_db, 0x80, ARRAY_SIZE(test_db), test_decode); 

// test trimming the output

	ASSERT(morse_trim(test_decode, ARRAY_SIZE(test_decode), 3));
	ASSERT(count - 4 == (unsigned int) morse_text(test_string, ARRAY_SIZE(test_string), test_decode, ARRAY_SIZE(test_decode))); 
	ASSERT(!strcmp(test_string, TEST_OUT + 4));
if(assert_errors) fprintf(stderr, "ASSERT(!strcmp(test_string=\"%s\",\n\"%s\") failed\n", test_string, TEST_OUT + 4);

	ASSERT(morse_trim(test_decode, ARRAY_SIZE(test_decode), 6));
	ASSERT(count - 10 == (unsigned int) morse_text(test_string, ARRAY_SIZE(test_string), test_decode, ARRAY_SIZE(test_decode))); 
	ASSERT(!strcmp(test_string, TEST_OUT + 10));
if(assert_errors) fprintf(stderr, "ASSERT(!strcmp(test_string=\"%s\",\n\"%s\") failed\n", test_string, TEST_OUT + 10);

	ASSERT(morse_trim(test_decode, ARRAY_SIZE(test_decode), 6));
	ASSERT(count - 16 == (unsigned int) morse_text(test_string, ARRAY_SIZE(test_string), test_decode, ARRAY_SIZE(test_decode))); 
	ASSERT(!strcmp(test_string, TEST_OUT + 16));
if(assert_errors) fprintf(stderr, "ASSERT(!strcmp(test_string=\"%s\",\n\"%s\") failed\n", test_string, TEST_OUT + 16);

	ASSERT(!morse_trim(test_decode, ARRAY_SIZE(test_decode), strlen(TEST_OUT + 16)));
	ASSERT(0 == morse_text(test_string, ARRAY_SIZE(test_string), test_decode, count)); 
	ASSERT(!strlen(test_string));
if(assert_errors) fprintf(stderr, "test_string=\"%s\"\n", test_string);

if(assert_errors) fprintf(stderr, "morse_decode returned %d, \"%s\"\n", count, test_string); 

// test aging the text

	count = morse_encode(test_db, ARRAY_SIZE(test_db), 0x80, TEST_STRING, fist);
	bzero(test_decode, sizeof(test_decode));
	count = morse_decode(test_decode, ARRAY_SIZE(test_decode), 0, test_db, count, 0, fist);
	
if(assert_errors) test_print_onoff(stdout, test_db, 0x40, count, test_decode); 

	ASSERT(count < TEST_DECODE_MAX && count > 0);	
	ASSERT(count = morse_text(test_string, ARRAY_SIZE(test_string), test_decode, ARRAY_SIZE(test_decode))); 
	ASSERT(count == strlen(TEST_OUT));
	ASSERT(!strcmp(test_string, TEST_OUT));

	count = morse_trim_age(test_decode, ARRAY_SIZE(test_decode), 100000);
	ASSERT(!count);
	ASSERT(morse_text(test_string, ARRAY_SIZE(test_string), test_decode, ARRAY_SIZE(test_decode))); 
	ASSERT(!strcmp(test_string, TEST_OUT));

if(assert_errors) test_print_onoff(stdout, test_db, 0x40, count, test_decode); 

	count = morse_trim_age(test_decode, ARRAY_SIZE(test_decode), 2000);
	ASSERT(count);
	ASSERT(morse_text(test_string, ARRAY_SIZE(test_string), test_decode, ARRAY_SIZE(test_decode))); 
	ASSERT(!strcmp(test_string, TEST_OUT + count));

if(assert_errors) test_print_onoff(stdout, test_db, 0x40, count, test_decode); 

// test noise performance

	ASSERT(test_noise(0) > 990);
	ASSERT(test_noise(1) > 990);
	ASSERT(test_noise(3) > 990);
	ASSERT(test_noise(5) > 990);
	ASSERT(test_noise(7) > 990);
	ASSERT(test_noise(10) > 900);
	ASSERT(test_noise(15) > 900);
	ASSERT(test_noise(20) > 900);
	ASSERT(test_noise(25) > 900);
	ASSERT(test_noise(30) > 900);
	ASSERT(test_noise(50) > 900);
	ASSERT(test_noise(70) > 600);
	ASSERT(test_noise(90) > 500);

	morse_fist_dlete(fist);

	return(assert_errors);
}

/*{ Quick call for various ASCII values to show they all work
	int i;

	for(i = 0; i < 255; i++)
	{
		fprintf(stdout, "morse_encode_code(%d \'%c\')=\"%s\"\n", i, i, morse_encode_code(i));
	}

	return(1);
}*/


#endif
