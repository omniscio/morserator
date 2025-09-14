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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

static FILE *config_fopen(const char *path, const char *filename, const char *mode);
const char *config_get(config_t key);
int config_load(const char *path, const char *filename);
int config_save(const char *path, const char *filename);
void config_set(config_t key, const char *value);

static const char *config_keys[CONFIG_COUNT] 
= 
{
	"version",
	"audio_in",
	"audio_out"
};

static const char *config_values[CONFIG_COUNT];

static FILE *config_fopen(const char *path, const char *filename, const char *mode)
{
	char name[PATH_MAX + 1];


	if(path && filename)
	{
		snprintf(name, sizeof(name)/sizeof(char), "%s/%s", path, filename);
	}
	else if(path)
	{
		strncpy(name, path, sizeof(name)/sizeof(char));
	}
	else if(filename)
	{
		strncpy(name, filename, sizeof(name)/sizeof(char));
	}
	else
	{
		return(0);
	}

	return(fopen(name, mode));
}

const char *config_get(config_t key)
{
	return(config_values[key]);
}

int config_load(const char *path, const char *filename)
{
	char linebuf[1000];
	FILE *file = config_fopen(path, filename, "r");
	char *split = 0;
	int key;

	if(!file) return(-1);
	
	for(key = 0; key < CONFIG_COUNT; key++)
	{
		if(config_values[key])
		{
			free((void *) config_values[key]);
			config_values[key] = 0;
		}
	}

	while(!feof(file) && !ferror(file))
	{
		if(fgets(linebuf, sizeof(linebuf)/sizeof(int) - 1, file))
		{
			split = strstr(linebuf, ": ");
			if(split)
			{
				*(split++) = 0;
				*(split++) = 0;
				
				while(split[strlen(split) - 1] < ' ')
				{
					split[strlen(split) - 1] = 0;
				}

				for(key = 0; key < CONFIG_COUNT && strcmp(linebuf, config_keys[key]); key++)
					;

				if(key < CONFIG_COUNT)
				{
					config_values[key] = strdup(split);
				}
			}
		}		
	}

	fclose(file);

	return(0);
}

int config_save(const char *path, const char *filename)
{
	FILE *file = config_fopen(path, filename, "w");
	int i;

	
	if(!file) return(-1);

	for(i = 0; i < CONFIG_COUNT; i++)
	{
		if(config_keys[i] && config_values[i])
		{
			fprintf(file, "%s: %s\n", config_keys[i], config_values[i]);
		}
	}

	fclose(file);

	return(0);
}

void config_set(config_t key, const char *value)
{
	if(config_values[key])
	{
		free((void *) config_values[key]);
		config_values[key] = 0;
	}
	if(value)
	{
		config_values[key] = strdup(value);
	}
}


#if defined(TEST)

static int assert_errors = 0;

#define ASSERT(test)	{if(!(test)) {fprintf(stderr, "%s:%d: test \"%s\" failed\n", __FILE__, __LINE__, #test); assert_errors++;}}

#define TEST_PATH		"/tmp/morserator"
#define TEST_FILENAME	"morserator.conf"

int main(void)
{
	ASSERT(!config_get(CONFIG_VERSION));
	ASSERT(!config_get(CONFIG_AUDIO_INPUT));
	ASSERT(!config_get(CONFIG_AUDIO_OUTPUT));
	config_set(CONFIG_VERSION, "foo");
	config_set(CONFIG_AUDIO_INPUT, "bar");
	config_set(CONFIG_AUDIO_OUTPUT, "baz");
	ASSERT(!strcmp(config_get(CONFIG_VERSION), "foo"));
	ASSERT(!strcmp(config_get(CONFIG_AUDIO_INPUT), "bar"));
	ASSERT(!strcmp(config_get(CONFIG_AUDIO_OUTPUT), "baz"));

	ASSERT(!config_save(TEST_PATH, TEST_FILENAME));
	ASSERT(!config_save(TEST_PATH "/" TEST_FILENAME, 0));
	ASSERT(!config_save(0, TEST_PATH "/" TEST_FILENAME));

	config_set(CONFIG_VERSION, 0);
	config_set(CONFIG_AUDIO_INPUT, 0);
	config_set(CONFIG_AUDIO_OUTPUT, 0);

	ASSERT(!config_get(CONFIG_VERSION));
	ASSERT(!config_get(CONFIG_AUDIO_INPUT));
	ASSERT(!config_get(CONFIG_AUDIO_OUTPUT));

	ASSERT(!config_load(TEST_PATH, TEST_FILENAME));

	ASSERT(!strcmp(config_get(CONFIG_VERSION), "foo"));
	ASSERT(!strcmp(config_get(CONFIG_AUDIO_INPUT), "bar"));
	ASSERT(!strcmp(config_get(CONFIG_AUDIO_OUTPUT), "baz"));

	return(assert_errors);
}

#endif
