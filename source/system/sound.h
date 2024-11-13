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

#ifndef __sound_h__
#define __sound_h__

int sound_init();
void sound_kill();
void sound_pause();
void sound_play();
void sound_update(void *buffer,int length);
void sound_setfps(int fps);

#define SAMPLE_RATE			44100
#define FRAMES_PER_SECOND	60
#define CHANNELS		    1

#define SAMPLE_SIZE         sizeof(s16)
#define SAMPLES_PER_FRAME	(SAMPLE_RATE / FRAMES_PER_SECOND)
#define BYTES_PER_FRAME     (SAMPLES_PER_FRAME * SAMPLE_SIZE)

#define BUFFER_SIZE         SAMPLES_PER_FRAME
#define MAX_AUDIO_LATENCY	4

#endif
