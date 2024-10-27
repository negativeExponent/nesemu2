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

#include <SDL/SDL.h>
#include "system/input.h"
#include "system/system.h"
#include "misc/config.h"

#include <libretro.h>
extern retro_input_state_t input_state_cb;

/*
todo: rewrite input.  only need a few global input variables:

todo: new input system.  stores input data into the nes struct

required variables:
	mouse x,y
	mouse buttons
	keyboard state
	joystick state (merged into keyboard state, possibly)
*/

//these global variables provide information for the device input code
int joyx,joyy;			//x and y coords for paddle/mouse
u8 joytrigger;
u8 joykeys[370];		//keyboard state
int joyconfig[4][8];	//joypad button configuration

// this will map joystick axises/buttons to unused keyboard buttons
#define FIRSTJOYSTATEKEY (350) // ideally should be SDLK_LAST
u8 joystate[32];	// dpad + 8 buttons is enuff' for me but let's be sure :-)

int input_init()
{
	int i;

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
	for(i=0;i<20;i++) {
		joystate[i] = 0;
	}
	input_update_config();
	return(0);
}

void input_kill()
{
}

void input_poll()
{
	u8 keystate[320];
	int i,x,y;

	for (i = 0; i < 320; i++)
		keystate[i] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, i) ? 1 : 0;

	//need to update mousex/mousey/mousebuttons here
	joytrigger = (u8)(SDL_GetMouseState(&x,&y) & 1) << 4;
	joyx = x;
	joyy = y;

	//now update key/mouse state, the input device logic will
	//decode the key/mouse data into the correct input for the nes
	for(i=0;i<320;i++)
		joykeys[i] = keystate[i];

	//put joypad buttons in the struct
	for(i=0;i<20;i++) {
		joykeys[FIRSTJOYSTATEKEY + i] = joystate[i];
	}
}

extern int video_getxoffset();
extern int video_getyoffset();
extern int video_getscale();
	
int input_poll_mouse(int *x,int *y)
{
	u8 buttons = SDL_GetMouseState(x,y);
	int scale;

	scale = video_getscale();
	SDL_ShowCursor(1);		//  <--- kludge!
	if(config_get_bool("video.fullscreen") != 0) {
		*x -= video_getxoffset();
		*y -= video_getyoffset();
		*x /= scale;
		*y /= scale;
	}
	else {
		*x /= scale;
		*y /= scale;
	}
	return(buttons & SDL_BUTTON(1));
}

void input_update_config()
{
	int key = FIRSTJOYSTATEKEY;
	joyconfig[0][0] = key++;
	joyconfig[0][1] = key++;
	joyconfig[0][2] = key++;
	joyconfig[0][3] = key++;
	joyconfig[0][4] = key++;
	joyconfig[0][5] = key++;
	joyconfig[0][6] = key++;
	joyconfig[0][7] = key++;
	joyconfig[1][0] = key++;
	joyconfig[1][1] = key++;
	joyconfig[1][2] = key++;
	joyconfig[1][3] = key++;
	joyconfig[1][4] = key++;
	joyconfig[1][5] = key++;
	joyconfig[1][6] = key++;
	joyconfig[1][7] = key++;
}
