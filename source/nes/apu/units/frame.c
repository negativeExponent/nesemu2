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

#define FRAME_REG			nes->apu.frame.reg
#define FRAME_CYCLES		nes->apu.frame.cycles
#define FRAME_QUARTER	nes->apu.frame.quarter
#define FRAME_HALF		nes->apu.frame.half
#define FRAME_IRQ			nes->apu.frame.irq
#define FRAME_ZERO		nes->apu.frame.zero

static s32 FrameCyclesNTSC[5] = { 7456,14912,22370,29828,37280 };
static s32 FrameCyclesPAL[5] = { 8312,16626,24938,33252,41560 };

static s32 *FrameCycles;

static INLINE void apu_frame_reset(int hard)
{
	if(hard) {
		FRAME_REG = 0;
	}
	FRAME_CYCLES = 0;
	FRAME_QUARTER = 0;
	FRAME_HALF = 0;
	FRAME_IRQ = 0;
	FRAME_ZERO = 0;
}

static INLINE void apu_frame_write(u32 addr,u8 data)
{
	FRAME_REG = data & 0xC0;
	if(nes->cpu.cycles & 1)
		FRAME_ZERO = 3;
	else
		FRAME_ZERO = 2;
	if(data & 0x40) {
		cpu_clear_irq(IRQ_FRAME);
	}
//	log_printf("apu_frame_write:  lengthcounter = %02X (cycle %d, line %d, frame %d)\n",data,LINECYCLES,SCANLINE,FRAMES);
}

static INLINE void apu_frame_step()
{
	if(FRAME_CYCLES == FrameCycles[0]) {
		FRAME_QUARTER = 2;
	}
	else if(FRAME_CYCLES == FrameCycles[1]) {
		FRAME_QUARTER = 2;
		FRAME_HALF = 2;
	}
	else if(FRAME_CYCLES == FrameCycles[2]) {
		FRAME_QUARTER = 2;
	}
	else if(FRAME_CYCLES == FrameCycles[3]) {
		if ((FRAME_REG & 0x80) == 0) {
			FRAME_QUARTER = 2;
			FRAME_HALF = 2;
			FRAME_IRQ = 3;
			FRAME_CYCLES = -2;
		}
	}
	else if (FRAME_CYCLES == FrameCycles[4]) {
		FRAME_QUARTER = 2;
		FRAME_HALF = 2;
		FRAME_CYCLES = -2;
	}
	FRAME_CYCLES++;
	if(FRAME_QUARTER && --FRAME_QUARTER == 0) {
		apu_square0_quarter();
		apu_square1_quarter();
		apu_triangle_quarter();
		apu_noise_quarter();
	}
	if(FRAME_HALF && --FRAME_HALF == 0) {
		apu_square0_half();
		apu_square1_half();
		apu_triangle_half();
		apu_noise_half();
	}
	if(FRAME_IRQ) {
		if(FRAME_REG == 0) {
//			if(nes->cpu.flags.i == 0)
//				log_printf("apu_frame_step:  frame irq!  cycle %d, line %d, frame %d\n",LINECYCLES,SCANLINE,FRAMES);
			cpu_set_irq(IRQ_FRAME);
		}
		FRAME_IRQ--;
	}
	if(FRAME_ZERO && --FRAME_ZERO == 0) {
		if(FRAME_REG & 0x80) {
			FRAME_QUARTER = 2;
			FRAME_HALF = 2;
		}
		FRAME_CYCLES = 0;
	}
}
