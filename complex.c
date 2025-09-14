
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

#include "complex.h"

#include <math.h>


static signed char complex8_math_cosine(int angle10);
//static signed char complex8_math_sine(int angle10);
void complex8_unitvect(complex8_t *vector, int angle10);

signed char complex8_math_cosine(int angle10)
{
	static signed char cosine8[256];
	int i;


	if(!cosine8[0])
	{
		for(i = 0; i < (int) sizeof(cosine8); i++)
		{
			cosine8[i] = 127 * cos((M_PI * i)/(sizeof(cosine8) * 2.0));
		}
	}

	angle10 &= 0x3FF;
	
	if(angle10 < 0x100)
	{
		return(cosine8[angle10]);
	}
	else if(angle10 < 0x200)
	{
		return(-cosine8[0x1FF-angle10]);
	}
	else if(angle10 < 0x300)
	{
		return(-cosine8[angle10 - 0x200]);
	}

	return(cosine8[0x3FF - angle10]);
}

//signed char complex8_math_sine(int angle10)
//{
//	return(complex8_math_cosine(angle10 - 0x300));
//}

void complex8_unitvect(complex8_t *vector, int angle10)
{
	vector->real = complex8_math_cosine(angle10 + 0x300);
	vector->imag = complex8_math_cosine(angle10);
}


#if defined(TEST)

#include <stdio.h>
#include <string.h>

static int assert_errors = 0;

#define ASSERT(test)	{if(!(test)) {fprintf(stderr, "%s:%d: test \"%s\" failed\n", __FILE__, __LINE__, #test); assert_errors++;}}

int main(void)
{
	complex8_t test;
	int i;


// basic tests of sine/cosine function (just sin2(x) + cos2(x) = 1

//fprintf(stderr, "DB_MAX=%ld\n", (long) DB_MAX);

	for(i = 0; i < 1024; i++)
	{
		complex8_unitvect(&test, i);
		ASSERT(test.real * test.real + test.imag * test.imag > 15800);
		ASSERT(test.real * test.real + test.imag * test.imag < 16384);
		if(assert_errors)
		{
			fprintf(stderr, "complex8_unitvect(test, %d): test={real=%d,imag=%d}\n", i, (int) test.real, (int) test.imag);
		}
	}
		
	return(assert_errors);
}

#endif


