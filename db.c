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
#include "db.h"

db_t db_from_complex8(complex8_t x);
db_t db_from_integer(db_integer_t x);
db_integer_t db_to_integer(db_t db);


unsigned char db_from_complex8(complex8_t x)
{
	return(db_from_integer(COMPLEX8_POW2(x)));
}

// return a power that is 3 x log2(x) -- approx 10 x log10(x)
db_t db_from_integer(db_integer_t pow)
{
	db_t ret;


	switch(pow)
	{
	case 0: return(0);
	case 1: return(0);
	case 2: return(3);
	case 3: return(5);
	case 4: return(6);
	}

	for(ret = 0; ret < (DB_MAX/3) - 1 && (pow >> ret); ret++)
		;

	pow >>= (ret - 3);
	ret = (ret - 1) * 3;

	if(pow >= 6) // 4 * 2 ^ (2/3) = 6.35...
	{
		ret += 2;
	}
	else if(pow >= 5) // 4 * 2 ^ (1/3) = 5.03...
	{
		ret += 1;
	}

	return(ret);
}

db_integer_t db_to_integer(db_t db)
{
	db_integer_t ret;
	static unsigned int logtable_x100[] = { 100, 126, 158, 200, 251, 316, 399, 501, 631, 794 };
	
	ret = logtable_x100[db % 10];

	if(db < 10)
	{
		ret = (ret + 50) / 100;
	}
	else if(db < 20)
	{
		ret = (ret + 5) / 10;
	}
	else
	{
		while(db > 20)
		{
			ret *= 10;
			db -= 10;
		}
	}

	return(ret);
}



#if defined(TEST)

#include <stdio.h>
#include <string.h>

static int assert_errors = 0;

#define ASSERT(test)	{if(!(test)) {fprintf(stderr, "%s:%d: test \"%s\" failed\n", __FILE__, __LINE__, #test); assert_errors++;}}

#define TEST_COUNT 10
static struct test_db_struct 
{
	db_integer_t integer;
	db_t db;
} test_values[TEST_COUNT] = {
	1, 0, 
	2, 3, 
	10, 10, 
	16, 12,
	20, 13, 
	25, 14, 
	32, 15, 
	40, 16, 
	50, 17, 
	100, 20, 
};

int main(void)
{
	int i;

	for(i = 0; i < TEST_COUNT; i++)
	{
		ASSERT(db_from_integer(test_values[i].integer) == test_values[i].db);
		ASSERT(db_to_integer(test_values[i].db) == test_values[i].integer);

		if(assert_errors)
		{
			fprintf(stderr, "test %d: db_from_integer(%d)=%d should be %d, db_to_integer(%d)=%d should be %d\n", i, 
					(int) test_values[i].integer, (int) db_from_integer(test_values[i].integer), (int) test_values[i].db,
					(int) test_values[i].db, (int) db_to_integer(test_values[i].db), (int) test_values[i].integer);
		}
	}
		
	return(assert_errors);
}

#endif


