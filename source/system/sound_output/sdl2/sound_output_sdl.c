#include <SDL2/SDL.h>
#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

static s32 BUFFSIZE;
static u8 *buffer;
static s32 buf_read_pos = 0;
static u32 buf_write_pos = 0;
static s32 buffered_bytes = 0;

static void sdl_write_buffer(u8* data, s32 len)
{
	int i;
	int bytes_to_copy = len * sizeof(s16);
	
	for(i = 0; i < bytes_to_copy; i += 2) {
		if(buffered_bytes == BUFFSIZE) return; // just drop samples
		*(s16*)((char*)buffer + buf_write_pos) = *(s16*)((char*)data + i);
		buf_write_pos = (buf_write_pos + 2) % BUFFSIZE;
		buffered_bytes += 2;
	}
}

static s32 sdl_read_buffer(u8* data, s32 len)
{
	if (buffered_bytes >= len) {
		if(buf_read_pos + len <= BUFFSIZE ) {
			memcpy(data, buffer + buf_read_pos, len);
		} else {
			s32 tail = BUFFSIZE - buf_read_pos;
			memcpy(data, buffer + buf_read_pos, tail);
			memcpy(data + tail, buffer, len - tail);
		}
		buf_read_pos = (buf_read_pos + len) % BUFFSIZE;
		buffered_bytes -= len;
	}

	return len;
}

void sdl_callback(void *unused, u8 *stream, s32 len)
{
	sdl_read_buffer((u8 *)stream, len);
}

static int sdl_create_buffers(void)
{
	log_printf("sdl_create_buffers:  creating stream buffer of %u bytes\n", BUFFSIZE);
	buffer = (u8 *)mem_alloc(BUFFSIZE);
	if (buffer == NULL)
		return(1);
	/* Add some silence to the buffer */
	buffered_bytes = 0;
	buf_read_pos = 0;
	buf_write_pos = 0;
	return(0);
}

static void sdl_destroy_buffers(void)
{
	if (buffer != NULL)
	{
		mem_free(buffer);
		buffer = NULL;
	}
}

int sound_init()
{
	int audio_latency;
	SDL_AudioSpec aspec, obtained;
	char *audio_driver;

	if(config_get_bool("sound.enabled") == 0)
		return(0);

	log_printf("sound_init: Start initialization\n");

	aspec.format   = AUDIO_S16SYS;
	aspec.freq     = SAMPLE_RATE;
	aspec.channels = CHANNELS;
	aspec.samples  = 512;
	aspec.callback = sdl_callback;
	aspec.userdata = NULL;

	/* initialize the SDL Audio system */
	if (SDL_InitSubSystem (SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE))
		goto cant_start_audio;

	/* Open the audio device and start playing sound! */
	if(SDL_OpenAudio(&aspec, &obtained) < 0) 
		goto cant_start_audio;

	audio_driver = SDL_GetCurrentAudioDriver();
	log_printf("sound_init: Driver is %s\n",audio_driver);
	log_printf("sound_init: frequency: %d, channels: %d, samples: %d\n",obtained.freq,obtained.channels,obtained.samples);

	SDL_PauseAudio(0);

	audio_latency = 1;

	BUFFSIZE = SAMPLE_RATE * sizeof(s16) * audio_latency / MAX_AUDIO_LATENCY;
	BUFFSIZE = (BUFFSIZE / 1024) * 1024;
	if(BUFSIZ < 1024) BUFFSIZE = 1024;

	if(sdl_create_buffers())
		goto cant_create_buffers;

	log_printf("sound_init: End initialization with latency %d.\n", audio_latency);

	return(0);
	
	// error handling
cant_create_buffers:
cant_start_audio:
	log_printf("sound_init: Initialization failed. SDL error: %s\n", SDL_GetError());
	return(1);
}

void sound_pause()
{
	SDL_PauseAudio(1);
}

void sound_play()
{
	SDL_PauseAudio(0);
}

void sound_update(void *buffer,int length)
{
	SDL_LockAudio();
	sdl_write_buffer((u8*)buffer, length);
	SDL_UnlockAudio();
}

void sound_kill()
{
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	sdl_destroy_buffers();
}

void sound_setfps(int fps)
{
}