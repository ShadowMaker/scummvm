/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CBLOOD_HAND_MOTION_H
#define CBLOOD_HAND_MOTION_H

#include "common/scummsys.h"

namespace CBlood {

struct HandMotion {
	uint16 gate = 0;
	uint16 delayX = 0, reloadX = 0, stepX = 0;
	uint16 delayY = 0, reloadY = 0, stepY = 0;

	void refresh(uint16 elapsed, uint16 deadline, uint16 forced) {
		gate = 0;
		if (!forced) {
			if (elapsed <= deadline) return;
			gate = uint16(elapsed - deadline);
			if (gate <= 1500 && (gate & 8)) gate = 0;
			if (gate <= 2000) return;
		}
		stepX = 3;
		stepY = 2;
		gate = 1;
	}

	static void tickAxis(int &position, int maximum, uint16 &delay, uint16 reload, uint16 &step) {
		delay = uint16(delay - 1);
		if (delay & 0x8000) {
			delay = reload;
			step = uint16(0 - step);
		}
		uint16 candidate = uint16(position + step);
		if (!(candidate & 0x8000) && candidate <= maximum) position = candidate;
	}

	void tick(int &x, int &y, int maxY) {
		if (!gate) return;
		tickAxis(x, 288, delayX, reloadX, stepX);
		tickAxis(y, maxY, delayY, reloadY, stepY);
	}
};

} // namespace CBlood
#endif
