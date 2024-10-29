/***************************************************************************
 *   Copyright (C) 2006-2009 by Dead_Body   *
 *   jamesholodnak@gmail.com   *
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

#include <string.h>

#ifdef WIN32
	#include <windows.h>
	#include <direct.h>
	#include <io.h>
#else
	#include <unistd.h>
#endif
#include "emu/events.h"
#include "system/main.h"
#include "system/video.h"
#include "system/input.h"
#include "system/sound.h"
#include "system/sdl/console/console.h"
#include "nes/nes.h"
#include "nes/state/state.h"
#include "misc/paths.h"
#include "misc/log.h"
#include "misc/config.h"

#ifndef _MAX_PATH
	#define _MAX_PATH 4096
#endif

#include <libretro.h>
extern retro_input_state_t input_state_cb;
extern bool libretro_supports_bitmasks;

int system_init()
{
	return(0);
}

void system_kill()
{
}

#define bit(n)	(1 << (n))
#define checkkey(key,n,evt)								\
	if(joykeys[key] && (keydown & bit(n)) == 0) {	\
		keydown |= bit(n);									\
		emu_event(evt,0);										\
	}																\
	else if(joykeys[key] == 0) {							\
		keydown &= ~bit(n);									\
	}

#define BUTTON_A        0
#define BUTTON_B        1
#define BUTTON_SELECT   2
#define BUTTON_START    3
#define BUTTON_UP       4
#define BUTTON_DOWN     5
#define BUTTON_LEFT     6
#define BUTTON_RIGHT    7

void system_checkevents()
{
	static int keydown = 0;
	static int inDiskSwitch = 0;
	u16 joypad_bits[2];
	int i,j;

	if (libretro_supports_bitmasks)
	{
		for (j = 0; j < 2; j++)
			joypad_bits[j] = input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
	}
	else
	{
		joypad_bits[0] = 0;
		joypad_bits[1] = 0;
		for (j = 0; j < 2; j++)
			for (i = 0; i <= RETRO_DEVICE_ID_JOYPAD_R3; i++)
				joypad_bits[j] |= input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
	}

	joystate[BUTTON_A]      = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_A) ? 1 : 0;
	joystate[BUTTON_B]      = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_B) ? 1 : 0;
	joystate[BUTTON_SELECT] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT) ? 1 : 0;
	joystate[BUTTON_START]  = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_START) ? 1 : 0;
	joystate[BUTTON_UP]     = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP) ? 1 : 0;
	joystate[BUTTON_DOWN]   = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN) ? 1 : 0;
	joystate[BUTTON_LEFT]   = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT) ? 1 : 0;
	joystate[BUTTON_RIGHT]  = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT) ? 1 : 0;

	joystate[8 | BUTTON_A]      = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_A) ? 1 : 0;
	joystate[8 | BUTTON_B]      = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_B) ? 1 : 0;
	joystate[8 | BUTTON_SELECT] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT) ? 1 : 0;
	joystate[8 | BUTTON_START]  = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_START) ? 1 : 0;
	joystate[8 | BUTTON_UP]     = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP) ? 1 : 0;
	joystate[8 | BUTTON_DOWN]   = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN) ? 1 : 0;
	joystate[8 | BUTTON_LEFT]   = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT) ? 1 : 0;
	joystate[8 | BUTTON_RIGHT]  = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT) ? 1 : 0;

	if (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L))
	{
		if (!inDiskSwitch)
		{
			emu_event(E_FLIPDISK, 0);
			inDiskSwitch = 1;
		}
	}
	else
		inDiskSwitch = 0;

	//update the console
	console_update();

	//check for system key presses
	checkkey(RETROK_ESCAPE,	0,		E_QUIT);
	checkkey(RETROK_F1,			1,		E_TOGGLERUNNING);
	/*checkkey(SDLK_F4,			2,		E_TOGGLEFULLSCREEN);*/
	/*checkkey(SDLK_F5,			3,		E_SAVESTATE);*/
	/*checkkey(SDLK_F8,			4,		E_LOADSTATE);*/
	/*checkkey(RETROK_F9,			5,		E_FLIPDISK);*/

//	checkkey(SDLK_p,			1,		E_SOFTRESET);
//	checkkey(SDLK_o,			2,		E_HARDRESET);
//	checkkey(SDLK_F3,			4,		E_PLAYMOVIE);
//	checkkey(SDLK_F6,			7,		E_RECORDMOVIE);
//	checkkey(SDLK_F7,			8,		E_STOPMOVIE);
//	checkkey(SDLK_F10,		11,	E_DUMPDISK);
}

char *system_getcwd()
{
	static char buf[_MAX_PATH];

	if(getcwd(buf,_MAX_PATH) == NULL)
		memset(buf,0,_MAX_PATH);
	return(buf);
}

#ifdef WIN32
u64 system_gettick()
{
	LARGE_INTEGER li;

	QueryPerformanceCounter(&li);
	return(li.QuadPart);
}

u64 system_getfrequency()
{
	LARGE_INTEGER li;

	if(QueryPerformanceFrequency(&li) == 0)
		return(1);
	return(li.QuadPart);
}
#else //#elif defined(SDL)
u64 system_gettick()
{
	/* TODO: implement ticks */
	return(0);
}

u64 system_getfrequency()
{
	/* return(1000); */
	return(0);
}
#endif
