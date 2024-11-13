/*
 * OSSv3/OSSv4 output sound code.
 * License : MIT
 * See docs/MIT_license.txt for more information.
*/

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>

#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

static s32 oss_audio_fd = -1;

int sound_init()
{
	u32 channels = CHANNELS;
#ifdef MSB_FIRST
	u32 format = AFMT_S16_BE;
#else
	u32 format = AFMT_S16_LE;
#endif
	s32 err_ret;
	s32 tmp;

	if(config_get_bool("sound.enabled") == 0)
		return(0);

	log_printf("sound_init: Start initialization\n");
	
	tmp = SAMPLE_RATE;
	
	oss_audio_fd = open("/dev/dsp", O_WRONLY 
#ifdef NONBLOCKING_AUDIO
	| O_NONBLOCK
#endif
	);
	if (oss_audio_fd < 0)
	{
		log_printf("Couldn't open /dev/dsp.\n");
		return(1);
	}
	
	err_ret = ioctl(oss_audio_fd, SNDCTL_DSP_SPEED,&tmp);
	if (err_ret == -1)
	{
		log_printf("Could not set sound frequency\n");
		return(1);
	}
	err_ret = ioctl(oss_audio_fd, SNDCTL_DSP_CHANNELS, &channels);
	if (err_ret == -1)
	{
		log_printf("Could not set channels\n");
		return(1);
	}
	err_ret = ioctl(oss_audio_fd, SNDCTL_DSP_SETFMT, &format);
	if (err_ret == -1)
	{
		log_printf("Could not set sound format\n");
		return(1);
	}
	
	return(0);
}

void sound_update(void *buffer,int length)
{
	if (!oss_audio_fd) return;
	write(oss_audio_fd, buffer, length * 2);
}

void sound_kill()
{
	if (oss_audio_fd >= 0)
	{
		close(oss_audio_fd);
		oss_audio_fd = -1;
	}
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