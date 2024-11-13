/*
 * Portaudio output sound code.
 * License : MIT
 * See docs/MIT_license.txt for more information.
*/

#include <portaudio.h>

#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

PaStream *apu_stream;

#ifdef NONBLOCKING_AUDIO
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    /* Cast data passed through stream to our structure. */
    snd.output = (s16*)outputBuffer;
    s32 i;
    (void) inputBuffer; /* Prevent unused variable warning. */
	
    return 0;
}
#endif

int sound_init()
{
	s32 err;

	if(config_get_bool("sound.enabled") == 0)
		return(0);

	log_printf("sound_init: Start initialization\n");
	
	err = Pa_Initialize();
	
	PaStreamParameters outputParameters;

	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice) 
	{
		log_printf("No sound output\n");
		return(1);
	}

	outputParameters.channelCount = CHANNELS;
	outputParameters.sampleFormat = paInt16;
	//outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	err = Pa_OpenStream( &apu_stream,
		NULL,
		&outputParameters,
		SAMPLE_RATE,
		SAMPLE_RATE / 60,
		0,
#ifdef NONBLOCKING_AUDIO
		patestCallback, NULL );
#else
		NULL, NULL );
#endif
	err = Pa_StartStream( apu_stream );
	return(0);
}

void sound_update(void *buffer,int length)
{
	#ifndef NONBLOCKING_AUDIO
	Pa_WriteStream( apu_stream, buffer, length);
	#endif
}

void sound_kill()
{
	s32 err;
	err = Pa_CloseStream( apu_stream );
	err = Pa_Terminate();
}

void sound_pause()
{
}

void sound_play()
{
}

void sound_setfps(int fps)
{
}