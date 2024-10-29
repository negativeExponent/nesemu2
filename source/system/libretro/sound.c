/***************************************************************************
 *   Copyright (C) 2013 by James Holodnak                                  *
 *   jamesholodnak@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "types.h"
#include "misc/log.h"
#include "misc/config.h"
#include "misc/memutil.h"
#include "system/sound.h"

#include <libretro.h>
extern retro_audio_sample_batch_t audio_batch_cb;

static s16 *sound_buf;

int sound_init()
{
	if (sound_buf) return 0;
	/* samples are 735 length s16* */
	sound_buf = (s16*)mem_alloc(1024 * 2 * sizeof(s16));
	return(0);
}

void sound_kill()
{
	mem_free(sound_buf);
	sound_buf = 0;
}

void sound_play()
{
}

void sound_pause()
{
}

void sound_update(void *buf,int size)
{
	s16 *sample = (s16*)buf;
	int len = 0;

	while (size) {
		int pos = len >> 1;
		sound_buf[len++] = sample[pos];
		sound_buf[len++] = sample[pos];
		size--;
	}

	audio_batch_cb((const s16*)sound_buf, len >> 1);
}

void sound_setfps(int fps)
{
}
