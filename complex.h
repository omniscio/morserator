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

/*
 * complex8 represents a complex value range +1,-1 to 8 bit precision
 */

#if !defined(COMPLEX8)
#define COMPLEX8

typedef struct complex8_struct
{
	signed char real;
	signed char imag;
} complex8_t;

#define COMPLEX8_MAC(a,x,y) \
	{ (a).real += ((((x).real)*((y).real) - ((x).imag)*((y).imag)) >> 8); \
	  (a).imag += ((((x).real)*((y).imag) + ((x).imag)*((y).real)) >> 8); }

#define COMPLEX8_SCALE(a,x) \
	{ (a).real = ((a).real * (x)); \
	  (a).imag = ((a).imag * (x)); }

#define COMPLEX8_SET(a,r,i) \
	{ (a).real = ((r) * 0xFF); \
	  (a).imag = ((i) * 0xFF); }

#define COMPLEX8_COPY(a,b) \
	{ (a).real = ((b).real); \
	  (a).imag = ((b).imag); }

#define COMPLEX8_REAL(a)	(((a).real)*(1.0/255.0))
#define COMPLEX8_IMAG(a)	(((a).imag)*(1.0/255.0))

#define COMPLEX8_POW2(a)	(((a).real * (a).real + (a).imag * (a).imag))

void complex8_unitvect(complex8_t *vector, int angle10);

#endif
