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
#include "cblood/renderer.h"
#include "common/endian.h"
#include "common/system.h"
#include "graphics/paletteman.h"
namespace CBlood {
Renderer::Renderer(OSystem &system) : shownImage(0), splitPalette(false), _system(&system) {
	memset(screen, 0, sizeof(screen));
	memset(creature, 0, sizeof(creature));
	memset(cockpit, 0, sizeof(cockpit));
	memset(backup, 0, sizeof(backup));
	memset(palettes, 0, sizeof(palettes));
	memset(currentPalette, 0, sizeof(currentPalette));
}
void Renderer::present() {
	byte pixels[64000];
	decodeScreen(shownImage == 1 ? creature : screen, pixels);
	if (splitPalette)
		for (int i = 18 * 320; i < 144 * 320; ++i) pixels[i] |= 16;
	_system->copyRectToScreen(pixels, 320, 0, 0, 320, 200);
	_system->updateScreen();
}

void Renderer::setPalette(const uint16 *colors) {
	static const byte levels[8] = {0, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff};
	byte rgb[48];
	for (int i = 0; i < 16; ++i) {
		currentPalette[i] = colors[i];
		rgb[i * 3] = levels[(colors[i] >> 8) & 7];
		rgb[i * 3 + 1] = levels[(colors[i] >> 4) & 7];
		rgb[i * 3 + 2] = levels[colors[i] & 7];
	}
	_system->getPaletteManager()->setPalette(rgb, 0, 16);
}

void Renderer::decodeScreen(const byte *src, byte *dst) const {
	for (int group = 0; group < 4000; ++group) {
		for (int bit = 15; bit >= 0; --bit) {
			byte color = 0;
			for (int plane = 0; plane < 4; ++plane)
				color |= ((READ_BE_UINT16(src + group * 8 + plane * 2) >> bit) & 1) << plane;
			*dst++ = color;
		}
	}
}

void Renderer::setPlanetPalette(const byte *colors) {
	static const byte levels[8] = {0, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff};
	byte rgb[48];
	for (int i = 0; i < 16; ++i) {
		uint16 color = READ_BE_UINT16(colors + 32 + i * 2);
		rgb[i * 3] = levels[(color >> 8) & 7];
		rgb[i * 3 + 1] = levels[(color >> 4) & 7];
		rgb[i * 3 + 2] = levels[color & 7];
	}
	_system->getPaletteManager()->setPalette(rgb, 16, 16);
}

} // namespace CBlood
