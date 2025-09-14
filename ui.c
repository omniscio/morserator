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

#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_ttf.h>

#include "complex.h"
#include "config.h"
//#include "fft.h"
#include "waterfall.h"

#define ARRAY_SIZE(x)		(sizeof(x)/sizeof(*x))

#define UI_SOUND_RATE		6400
#define UI_SAMPLE_SECONDS	3
#define UI_SAMPLE_POW2		7
#define UI_SAMPLE_RATE		(UI_SOUND_RATE >> UI_SAMPLE_POW2)
#define UI_SAMPLES			(UI_SAMPLE_RATE * UI_SAMPLE_SECONDS)

#define UI_REFRESH_MSEC		50

#define UI_SUBCHANNEL_START	6
#define UI_SUBCHANNELS		56

#define UI_FONT_PATH		"/usr/share/fonts/truetype"
#define UI_FONT				"freefont/FreeSansBold.ttf"
#define UI_FONT_HEIGHT		20
#define UI_FONT_WIDTH		10

#define UI_TILE_HEIGHT		5
#define UI_TILE_WIDTH		3

#define UI_BORDER_LEFT		(UI_FONT_WIDTH*6)
#define UI_BORDER_RIGHT		(UI_FONT_WIDTH*20)
#define UI_BORDER_TOP		(UI_FONT_HEIGHT)
#define UI_BORDER_BOTTOM	(UI_FONT_HEIGHT)

#define UI_WINDOW_HEIGHT(rows) (UI_BORDER_TOP + UI_TILE_HEIGHT * (rows) + UI_BORDER_BOTTOM)
#define UI_WINDOW_WIDTH		(UI_BORDER_LEFT + UI_TILE_WIDTH * UI_SAMPLES + UI_BORDER_RIGHT)

// #define UI_TEXT_HEIGHT		(UI_ROW_HEIGHT * 24)
// #define UI_TEXT_WIDTH		(UI_ROW_HEIGHT * 24)


#define UI_BORDER_COLOUR	0x808080
#define UI_TEXT_COLOUR		0x000000
#define UI_TEXTBOX_COLOUR	0xFFFFFF
#define UI_DECODE_COLOUR	0xFF00FF
#define UI_MARK_COLOUR		0xFFFF00

#define UI_DECODE_CONFIDENCE 1

#define UI_COLOUR_R(x)		(0xFF & ((x) >> 16))
#define UI_COLOUR_G(x)		(0xFF & ((x) >> 8))
#define UI_COLOUR_B(x)		(0xFF & ((x)))


struct ui_row_data_struct
{
	db_t colours[UI_SUBCHANNELS][UI_SAMPLE_RATE * UI_SAMPLE_SECONDS];
	morse_decode_t decodes[UI_SUBCHANNELS][UI_SAMPLE_RATE * UI_SAMPLE_SECONDS];
	morse_fist_t fist;
	char *text;
};

static struct ui_struct
{
	SDL_AudioDeviceID audioDevice;
	SDL_AudioSpec audioSpec;
	SDL_Renderer *renderer;
	SDL_Texture **ui_glyph_cache_text;
	SDL_Texture **ui_glyph_cache_decode;
	SDL_TimerID refresh_timer;
	int refresh_ms;
	TTF_Font *font;
	waterfall_t waterfall;
	int waterfall_rows;
	int cursor;
	SDL_Window *window;
	int sound_ptr;
	signed short sound[UI_SOUND_RATE * UI_SAMPLE_SECONDS];
//	complex8_t sound[UI_SOUND_RATE * UI_SAMPLE_SECONDS];
	struct ui_row_data_struct *row_data;
} *ui_data = 0;


static SDL_Texture **ui_glyph_cache(TTF_Font *font, SDL_Renderer *renderer, unsigned int ink);
static void ui_print(SDL_Texture **glyph_cache, SDL_Rect *cursor, char c);
static void ui_print_textbox(SDL_Texture **glyph_cache, SDL_Rect box, SDL_Color paper, const char *string);
static int ui_sound_begin(const char *in_device, const char *out_device);
static void ui_sound_callback(void *blob, Uint8 *stream, int len);
static void ui_sound_end(void);
static int ui_waterfall_begin(int rows);
//static Uint32 ui_waterfall_callback(Uint32 interval, void *blob);
static void ui_waterfall_clear(struct ui_struct *ui_data);
static void ui_waterfall_end(void);
static void ui_waterfall_redraw(struct ui_struct *ui_data);
static void ui_waterfall_redraw_row_power(struct ui_struct *ui_data, int row, int subchannel, int single);
static void ui_waterfall_redraw_row_text(struct ui_struct *ui_data, int row, int subchannel, int single);
static void ui_waterfall_redraw_text(struct ui_struct *ui_data, int subchannel);
int ui(const char *config_path, const char *config_file);

static SDL_Texture **ui_glyph_cache(TTF_Font *font, SDL_Renderer *renderer, unsigned int ink)
{
	SDL_Texture **ret = 0;
	SDL_Surface *glyphSurface = 0;
	SDL_Color ink_color;
	char string[2];


	ink_color.r = UI_COLOUR_R(ink);
	ink_color.g = UI_COLOUR_G(ink);
	ink_color.b = UI_COLOUR_B(ink);
	ink_color.a = 0xFF;

	ret = (SDL_Texture **) calloc('\x7f' - ' ' + 1, sizeof(SDL_Texture *));

	string[1] = 0;
	
	for(string[0] = ' '; string[0] < '\x7F'; string[0]++)
	{
		glyphSurface = TTF_RenderText_Blended(font, string, ink_color);
		ret[string[0] - ' '] = SDL_CreateTextureFromSurface(renderer, glyphSurface);
	}

	return(ret);
}

static void ui_print(SDL_Texture **glyph_cache, SDL_Rect *cursor, char c)
{
	SDL_Rect box;


	if(c < ' ') c = ' ';

	if(c >= ' ' && c < '\x7f') 
	{
		switch(c)
		{
		case 'i': case 'I': case '1': case '!': case '|': case '.': case ',': case ';': case ':':
			box.x = cursor->x + ((cursor->w + 1) >> 2);
			box.y = cursor->y;
			box.w = (cursor->w + 1) >> 1;
			box.h = cursor->h;
			SDL_RenderCopy(ui_data->renderer, glyph_cache[c - ' '], 0, &box);
			break;
		default:
			SDL_RenderCopy(ui_data->renderer, glyph_cache[c - ' '], 0, cursor);
		}
			
		cursor->x += cursor->w;
	}
}

static void ui_print_textbox(SDL_Texture **glyph_cache, SDL_Rect box, SDL_Color paper, const char *string)
{
	SDL_Rect cursor;
	int i;


	cursor.w = UI_FONT_WIDTH;
	cursor.h = UI_FONT_HEIGHT;
	cursor.x = box.x;
	cursor.y = box.y;

	SDL_SetRenderDrawColor(ui_data->renderer, paper.r, paper.g, paper.b, paper.a);
	SDL_RenderFillRect(ui_data->renderer, &box);	

	if(string)
	{
		for(i = 0; string[i] && cursor.y + cursor.h <= box.y + box.h; i++)
		{
			if(string[i] == '\n')
			{
				cursor.x = box.x;
				cursor.y += cursor.h;
			}
			else if(string[i] >= ' ')
			{
				ui_print(ui_data->ui_glyph_cache_text, &cursor, string[i]);
				if(cursor.x + cursor.w > box.x + box.w)
				{
					cursor.x = box.x;
					cursor.y += cursor.h;
				}
			}
		}
	}
}

static int ui_sound_begin(const char *in_device, const char *out_device)
{
	int sound_input_count, sound_output_count;
	int i;


	sound_output_count = SDL_GetNumAudioDevices(0);
	sound_input_count = SDL_GetNumAudioDevices(1);

	if(sound_output_count <= 0 && sound_input_count <= 0)
	{
		fprintf(stderr, "Cannot find input or output devices\n");
		return(0);
	}

	if(sound_input_count > 0)
	{
		for(i = 0; i < sound_input_count 
		&& (!SDL_GetAudioDeviceName(i, 1) || strcmp(config_get(CONFIG_AUDIO_INPUT), SDL_GetAudioDeviceName(i,1))); i++)
			;

		if(i >= sound_input_count)
		{
			fprintf(stderr, "Invalid sound input \"%s\" -- options are:", config_get(CONFIG_AUDIO_INPUT));
			for(i = 0; i < sound_input_count; i++)
			{
				if(SDL_GetAudioDeviceName(i, 1))
				{
					fprintf(stderr, " \"%s\"", SDL_GetAudioDeviceName(i, 1));
				}
			}
			fprintf(stderr, "\n");

			return(0);
		}

		ui_data->audioSpec.freq = 8000;
		ui_data->audioSpec.format = AUDIO_S8;
		ui_data->audioSpec.channels = 1;
		ui_data->audioSpec.silence = 0;
		ui_data->audioSpec.samples = 800;
		ui_data->audioSpec.padding = 0;
		ui_data->audioSpec.size = 800;
		ui_data->audioSpec.callback = ui_sound_callback;
		ui_data->audioSpec.userdata = (void *) ui_data;

		SDL_ClearError();

		ui_data->audioDevice = SDL_OpenAudioDevice(config_get(CONFIG_AUDIO_INPUT), 1, &(ui_data->audioSpec), &(ui_data->audioSpec), SDL_AUDIO_ALLOW_ANY_CHANGE);
		
		if(*SDL_GetError())
		{
			fprintf(stderr, "Setting up audio, SDL_GetError() returned \"%s\"\n", SDL_GetError());
			return(0);
		} 
	}


	switch(ui_data->audioSpec.freq)
	{
	case 8000:
	case 16000:
	case 32000:
	case 44100:
		return(UI_SUBCHANNELS);

	default:
		break;
	}

	return(0);
}

static void ui_sound_callback(void *blob, Uint8 *stream, int len)
{
	struct ui_struct *ui_data = (struct ui_struct *) blob;
	int i, sound_bytes, sound_count; //, j;


//fprintf(stderr, "ui_sound_callback(blob,stream,len=%d)\n", len);

	sound_bytes = ui_data->audioSpec.channels * (((ui_data->audioSpec.format & 0x00FF) + 7) >> 3);
	sound_count = len / sound_bytes;

//fprintf(stderr, "sound_bytes=%d,sound_count=%d\n", sound_bytes, sound_count);

// TODO: check for overflows

	i = 0;	// TODO: this should age to the actual channel byte(s)
	while(i < sound_count)
	{
		if(ui_data->sound_ptr + 16 > (int) ARRAY_SIZE(ui_data->sound))
		{
			waterfall_update(ui_data->waterfall, ui_data->sound, ui_data->sound_ptr);
			ui_data->sound_ptr = 0;
		}

// decimation to get to 6400 samples/second
		switch(ui_data->audioSpec.freq)
		{
		case 8000:
			ui_data->sound[ui_data->sound_ptr] = stream[i * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +1] = stream[(i + 1) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +2] = stream[(i + 2) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +3] = stream[(i + 3) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound_ptr += 4;
			i += 5;
			break;

		case 16000:
			ui_data->sound[ui_data->sound_ptr] = stream[i * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +1] = stream[(i + 2) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound_ptr += 2;
			i += 5;
			break;

		case 32000:
			ui_data->sound[ui_data->sound_ptr] = stream[i * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound_ptr += 1;
			i += 5;
			break;

		case 41200:
			ui_data->sound[ui_data->sound_ptr] = stream[i * sound_bytes * ui_data->audioSpec.channels] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +1] = stream[(i + 6) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +2] = stream[(i + 13) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +3] = stream[(i + 19) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +4] = stream[(i + 26) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +5] = stream[(i + 32) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +6] = stream[(i + 39) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +7] = stream[(i + 45) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +8] = stream[(i + 52) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +9] = stream[(i + 58) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +10] = stream[(i + 64) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +11] = stream[(i + 71) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +12] = stream[(i + 77) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +13] = stream[(i + 84) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +14] = stream[(i + 90) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound[ui_data->sound_ptr +15] = stream[(i + 97) * sound_bytes] - ui_data->audioSpec.silence;
			ui_data->sound_ptr += 16;
			i += 103;
			break;

		default:
			// TODO: something intelligent, not just bail
			return;
		}
	}

	waterfall_update(ui_data->waterfall, ui_data->sound, ui_data->sound_ptr);
	ui_data->sound_ptr = 0;
}

static void ui_sound_end(void)
{
	SDL_PauseAudioDevice(ui_data->audioDevice, 1);
	SDL_CloseAudioDevice(ui_data->audioDevice);
}


static int ui_waterfall_begin(int rows)
{
	ui_data->waterfall_rows  = rows;
	ui_data->waterfall = waterfall(UI_SAMPLE_POW2, UI_SAMPLES, UI_SUBCHANNEL_START, UI_SUBCHANNEL_START + rows, (UI_WINDOW_HEIGHT(ui_data->waterfall_rows)) / UI_FONT_HEIGHT - 4, UI_WINDOW_WIDTH / UI_FONT_WIDTH - 2);

	ui_data->font = TTF_OpenFont(UI_FONT_PATH "/" UI_FONT, UI_FONT_HEIGHT);
	
	if(!ui_data->font)
	{
		fprintf(stderr, "Cannot open font file \"%s\"\n", UI_FONT_PATH "/" UI_FONT);
		return(-2);
	}
	
	if(!ui_data->waterfall)
	{
		fprintf(stderr, "waterfall(input_sampling_power_of_two=%d,samples=%d,first_channel=%d,last_channel=%d,rows=%d,cols=%d) failed\n", UI_SAMPLE_POW2, UI_SAMPLES, UI_SUBCHANNEL_START, UI_SUBCHANNEL_START + rows, rows - 2, UI_WINDOW_WIDTH / UI_FONT_WIDTH - 2);
		return(-3);
	}
	

	ui_data->window = SDL_CreateWindow("Morserator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
				UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT(rows), 0);

	if(ui_data->window)
	{
		ui_data->renderer = SDL_CreateRenderer(ui_data->window, -1, 0);
		
		if(ui_data->renderer)
		{
			ui_data->ui_glyph_cache_text = ui_glyph_cache(ui_data->font, ui_data->renderer, UI_TEXT_COLOUR);
			ui_data->ui_glyph_cache_decode = ui_glyph_cache(ui_data->font, ui_data->renderer, UI_DECODE_COLOUR);
			
			SDL_SetRenderDrawColor(ui_data->renderer, UI_COLOUR_R(UI_BORDER_COLOUR), UI_COLOUR_G(UI_BORDER_COLOUR), UI_COLOUR_B(UI_BORDER_COLOUR), 0xFF);
			SDL_RenderClear(ui_data->renderer);
			SDL_RenderPresent(ui_data->renderer);

//			ui_data->refresh_ms = UI_REFRESH_MSEC;
//			ui_data->refresh_timer = SDL_AddTimer(ui_data->refresh_ms, ui_waterfall_callback, (void *) ui_data);

			return(0);
		}
		
		SDL_DestroyWindow(ui_data->window);
		ui_data->window = 0;
	}

	return(-1);
}
/*
static Uint32 ui_waterfall_callback(Uint32 interval, void *blob)
{
	ui_waterfall_redraw((struct ui_struct *) blob);
	
	return(ui_data->refresh_ms);
}
*/
static void ui_waterfall_clear(struct ui_struct *ui_data)
{
	SDL_Rect r;

	
	r.w = UI_WINDOW_WIDTH;
	r.h = UI_WINDOW_HEIGHT(ui_data->waterfall_rows);
	r.x = 0;
	r.y = 0;

	SDL_SetRenderDrawColor(ui_data->renderer, UI_COLOUR_R(UI_BORDER_COLOUR), UI_COLOUR_G(UI_BORDER_COLOUR), UI_COLOUR_B(UI_BORDER_COLOUR), 0xFF);
	SDL_RenderFillRect(ui_data->renderer, &r);
}

static void ui_waterfall_end(void)
{
	int refresh_ms = ui_data->refresh_ms;

	ui_data->refresh_ms = 0;
	usleep(refresh_ms * 10);
	waterfall_dlete(ui_data->waterfall);
}

static void ui_waterfall_redraw(struct ui_struct *ui_data)
{
	int i;


	for(i = 1; i <= ui_data->waterfall_rows; i++)
	{
		waterfall_sync(ui_data->waterfall, i + UI_SUBCHANNEL_START);
	}

	if(ui_data->cursor > 0)
	{
//morse_fist_t fist = waterfall_fist(ui_data->waterfall, ui_data->cursor);
//printf("fist={dit=%d,dah=%d,tid=%d,letter=%d,word=%d}\n", (int) fist->dit, (int) fist->dah, (int) fist->tid, (int) fist->letter, (int) fist->word);
		ui_waterfall_redraw_text(ui_data, ui_data->cursor);
		ui_waterfall_redraw_row_power(ui_data, ui_data->waterfall_rows - 1, ui_data->cursor, 1);
		ui_waterfall_redraw_row_text(ui_data, ui_data->waterfall_rows - 1, ui_data->cursor, 1);
//printf("waterfall_text()=\"%s\"\n", waterfall_text(ui_data->waterfall, 1));
	}
	else
	{
		for(i = 1; i <= ui_data->waterfall_rows; i++)
		{
			ui_waterfall_redraw_row_power(ui_data, ui_data->waterfall_rows - i, i, 0);
		}
		for(i = 1; i <= ui_data->waterfall_rows; i++)
		{
			ui_waterfall_redraw_row_text(ui_data, ui_data->waterfall_rows - i, i, 0);
		}
	}

// TODO: Update text over waterfall

	SDL_RenderPresent(ui_data->renderer);

}

static void ui_waterfall_redraw_row_power(struct ui_struct *ui_data, int row, int subchannel, int single)
{
	const morse_decode_t *symbols = 0;
	const db_t *colours = 0;
	SDL_Rect r;
#if defined(UI_MARK_COLOUR)
	int x1, x2, y;
#endif
	unsigned int i;

	colours = waterfall_colours(ui_data->waterfall, subchannel + UI_SUBCHANNEL_START);

	r.w = UI_TILE_WIDTH;
	r.h = UI_TILE_HEIGHT;
	
	r.y = UI_BORDER_TOP + UI_TILE_HEIGHT * (row - 0);
	y = r.y + UI_TILE_HEIGHT / 2;
	
	for(i = 0; i < UI_SAMPLES; i++)
	{
		r.x = UI_BORDER_LEFT + i * UI_TILE_WIDTH;

		if(colours[i] < 16)
		{
			SDL_SetRenderDrawColor(ui_data->renderer, 0, 0, colours[i] << 4, 0xFF);
		}
		else if(colours[i] < 32)
		{
			SDL_SetRenderDrawColor(ui_data->renderer, 0, (colours[i] - 16) << 4, 0xFF, 0xFF);
		}
		else
		{
			SDL_SetRenderDrawColor(ui_data->renderer, (colours[i] - 32) << 4, 0xFF, 0xFF, 0xFF);
		}

		SDL_RenderFillRect(ui_data->renderer, &r);
	}

	symbols = waterfall_symbols(ui_data->waterfall, subchannel + UI_SUBCHANNEL_START);
	waterfall_start(ui_data->waterfall, subchannel + UI_SUBCHANNEL_START);

	r.w = UI_FONT_WIDTH;
	
#if defined(UI_MARK_COLOUR)
	for(i = 0; symbols && symbols[i].age; i++)
	{
		if(symbols[i].mark && symbols[i].age)
		{
			x1 = UI_SAMPLES - symbols[i].age;
			x2 = UI_SAMPLES - symbols[i].age + symbols[i].mark;
			if(x1 < 0) x1 = 0;
			if(x2 < 0) x2 = 0;
			if(x1 > UI_SAMPLES) x1 = UI_SAMPLES;
			if(x2 > UI_SAMPLES) x2 = UI_SAMPLES;
			
			if(x1 != x2)
			{
				SDL_SetRenderDrawColor(ui_data->renderer, UI_COLOUR_R(UI_MARK_COLOUR), UI_COLOUR_G(UI_MARK_COLOUR), UI_COLOUR_B(UI_MARK_COLOUR), 0xFF);
				SDL_RenderDrawLine(ui_data->renderer, UI_BORDER_LEFT + x1 * UI_TILE_WIDTH, y, UI_BORDER_LEFT + x2 * UI_TILE_WIDTH, y);
			}
		}
	}
#endif
}

static void ui_waterfall_redraw_row_text(struct ui_struct *ui_data, int row, int subchannel, int single)
{
	const morse_decode_t *symbols = 0;
//	morse_fist_t fist = 0;
	char left[(UI_BORDER_LEFT/UI_FONT_WIDTH) + 1];
	char right[(UI_BORDER_RIGHT/UI_FONT_WIDTH) + 1];
	SDL_Rect r;
	SDL_Color paper;
	unsigned int i, active = 0;
	
	
	bzero(left, sizeof(left));
	bzero(right, sizeof(right));

	paper.r = UI_COLOUR_R(UI_BORDER_COLOUR);
	paper.g = UI_COLOUR_G(UI_BORDER_COLOUR);
	paper.b = UI_COLOUR_B(UI_BORDER_COLOUR);
	paper.a = 0xFF;
	
	r.w = UI_FONT_WIDTH;
	r.h = UI_FONT_HEIGHT;
	
	r.y = UI_BORDER_TOP + UI_TILE_HEIGHT * (row - 0);
	r.y -= UI_FONT_HEIGHT/2;
	
	if(r.y < UI_BORDER_TOP + UI_FONT_HEIGHT)
	{
		r.y = UI_BORDER_TOP + UI_FONT_HEIGHT;
	}
	else if(r.y >= UI_BORDER_TOP + UI_TILE_HEIGHT * (ui_data->waterfall_rows - 2) - UI_FONT_HEIGHT/2)
	{
		r.y = UI_BORDER_TOP + UI_TILE_HEIGHT * (ui_data->waterfall_rows - 2) - UI_FONT_HEIGHT/2;
	}

	symbols = waterfall_symbols(ui_data->waterfall, subchannel + UI_SUBCHANNEL_START);

	r.w = UI_FONT_WIDTH;

	if(!single)
	{
		for(i = 0; symbols && symbols[i].age; i++)
		{
			if(symbols[i].text && symbols[i].age > UI_FONT_WIDTH && symbols[i].age < UI_SAMPLES)
			{
				r.x = UI_BORDER_LEFT + UI_TILE_WIDTH * (UI_SAMPLES - symbols[i].age);
				ui_print(ui_data->ui_glyph_cache_decode, &r, symbols[i].text);
				active++;
			}
		}
	}

	if(((subchannel - 1 + UI_SUBCHANNEL_START) & 0x03) == 0 || single)
	{
		snprintf(right, ARRAY_SIZE(right), "%d",  UI_SAMPLE_RATE * (subchannel - 1 + UI_SUBCHANNEL_START));
	}

//	snprintf(left, ARRAY_SIZE(left), "%d", waterfall_text_lines(ui_data->waterfall, subchannel + UI_SUBCHANNEL_START));

//	fist = waterfall_fist(ui_data->waterfall, subchannel + UI_SUBCHANNEL_START);
	
//	if(fist)
//	{
//		snprintf(left, ARRAY_SIZE(left), "m=%d,%d,s=%d,%d", (int) fist->dit, (int) fist->dah, (int) fist->tid, (int) fist->letter);
//	} 

	if(*left)
	{
		r.x = UI_FONT_WIDTH;
		r.w = UI_BORDER_LEFT - UI_FONT_WIDTH * 2;
		ui_print_textbox(ui_data->ui_glyph_cache_text, r, paper, left); 
	}

	if(*right)
	{
		r.x = UI_WINDOW_WIDTH - UI_BORDER_RIGHT + UI_FONT_WIDTH;
		r.w = UI_BORDER_RIGHT - UI_FONT_WIDTH * 2;
		ui_print_textbox(ui_data->ui_glyph_cache_text, r, paper, right);
	} 
}

static void ui_waterfall_redraw_text(struct ui_struct *ui_data, int subchannel)
{
	SDL_Color paper;
	const char *string = 0;
	SDL_Rect r;


	string = waterfall_text(ui_data->waterfall, subchannel + UI_SUBCHANNEL_START);

//fprintf(stderr, "waterfall_text(ui_data->waterfall,subchannel=%d)=\"%s\"\n", (int) subchannel, string);

	r.y = UI_BORDER_TOP;
	r.x = UI_FONT_WIDTH;
	r.w = UI_WINDOW_WIDTH - 2 * UI_FONT_WIDTH;
	r.h = UI_WINDOW_HEIGHT(ui_data->waterfall_rows) - 4 * UI_FONT_HEIGHT;

	paper.r = UI_COLOUR_R(UI_TEXTBOX_COLOUR);
	paper.g = UI_COLOUR_G(UI_TEXTBOX_COLOUR);
	paper.b = UI_COLOUR_B(UI_TEXTBOX_COLOUR);
	paper.a = 0xFF;
	
	ui_print_textbox(ui_data->ui_glyph_cache_text, r, paper, string); 
}


int ui(const char *config_path, const char *config_file)
{
	struct ui_struct ui_static_data;
	SDL_Event e;
	int rows = 0;


	bzero(&ui_static_data, sizeof(ui_static_data));
	ui_data = &ui_static_data;

	if(config_load(config_path, config_file))
	{
		fprintf(stderr, "Cannot open \"%s/%s\"\n", config_path, config_file);
		return(1);
	}
	
	if(!config_get(CONFIG_AUDIO_INPUT))
	{
		fprintf(stderr, "No audio input device configured in \"%s/%s\"\n", config_path, config_file);
		return(2);
	}

	if(!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) && !TTF_Init())
	{
		rows = ui_sound_begin(config_get(CONFIG_AUDIO_INPUT), config_get(CONFIG_AUDIO_OUTPUT));
		
		if(rows)
		{
			if(!ui_waterfall_begin(rows))
			{
				SDL_PauseAudioDevice(ui_data->audioDevice, 0);

				do
				{
					while(!SDL_PollEvent(&e))
					{
						ui_waterfall_redraw(ui_data); 
						usleep(100000);
					}

					switch(e.type)
					{
					case SDL_MOUSEBUTTONDOWN:
						if(ui_data->cursor)
						{
							ui_data->cursor = 0;
							ui_waterfall_clear(ui_data);
						}
						else if(e.button.y > UI_BORDER_TOP && e.button.y < UI_BORDER_TOP + UI_TILE_HEIGHT * ui_data->waterfall_rows)
						{
							ui_data->cursor = ui_data->waterfall_rows - (e.button.y - UI_BORDER_TOP) / UI_TILE_HEIGHT;
							ui_waterfall_clear(ui_data);
						}
						break;

					default:
						// do nothing
						break;
					}
				}
				while(e.type != SDL_QUIT);

				SDL_PauseAudioDevice(ui_data->audioDevice, 1);

				ui_waterfall_end();
			}
			else
			{
				fprintf(stderr, "ui_waterfall_begin(%d) failed\n", rows);
				exit(1);
			}

			ui_sound_end();
		}
	}


	TTF_Quit();
	SDL_Quit();

	sleep(1);
	ui_data = 0;

	return(0);
}

