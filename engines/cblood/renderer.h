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
#ifndef CBLOOD_RENDERER_H
#define CBLOOD_RENDERER_H
#include "common/scummsys.h"
class OSystem;
namespace CBlood {
class Renderer {
public:
	enum {
		kBorderX = 48, kBorderTop = 30,
		kOutputWidth = 416, kOutputHeight = 260, kAspectHeight = 312
	};
	bool showBorder;
	bool correctAspect = false;
	bool stretchBorder() const {
#ifdef USE_ASPECT
		return showBorder && correctAspect;
#else
		return false;
#endif
	}
	int outputHeight() const {
		return showBorder ? (stretchBorder() ? kAspectHeight : kOutputHeight) : 200;
	}
	explicit Renderer(OSystem &system);
	void present();
	void setPalette(const uint16 *colors);
	void setPlanetPalette(const byte *colors);
	void clearRasterPalette();
	void presentRaster(byte *pixels);
	void decodeScreen(const byte *src, byte *dst) const;
	byte screen[32768], creature[32000], cockpit[32000], backup[32000];
	uint16 palettes[4][16], currentPalette[16];
	int shownImage;
	bool splitPalette;
	bool rasterActive;
	uint16 rasterRows[200][16];
private:
	OSystem *_system;
	byte _borderRows[200];
	byte _borderPixels[kOutputWidth * kAspectHeight];
};
} // namespace CBlood
#endif
