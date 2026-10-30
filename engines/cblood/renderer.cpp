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
#include "graphics/scaler/aspect.h"
#include "common/textconsole.h"
namespace CBlood {
Renderer::Renderer(OSystem &system) : showBorder(true), shownImage(0), splitPalette(false), rasterActive(false), _system(&system) {
	memset(rasterRows, 0, sizeof(rasterRows));
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
	for (int y = 0; y < 200; ++y)
		_borderRows[y] = splitPalette && y >= 18 && y < 144 ? 16 : 0;
	if (rasterActive)
		presentRaster(pixels);
	else if (splitPalette)
		for (int i = 18 * 320; i < 144 * 320; ++i) pixels[i] |= 16;
	if (showBorder) {
		byte *output = _borderPixels;
		for (int y = 0; y < kOutputHeight; ++y) {
			int sourceY = y - kBorderTop;
			int paletteY = sourceY < 0 ? 0 : sourceY > 199 ? 199 : sourceY;
			byte *row = output + y * kOutputWidth;
			memset(row, _borderRows[paletteY], kOutputWidth);
			if (sourceY >= 0 && sourceY < 200)
				memcpy(row + kBorderX, pixels + sourceY * 320, 320);
		}
		presentBorder();
	} else {
		_system->copyRectToScreen(pixels, 320, 0, 0, 320, 200);
	}
	_system->updateScreen();
}

void Renderer::publishPalette(const byte *rgb, uint start, uint count) {
	memcpy(_rgbPalette + start * 3, rgb, count * 3);
	if (_system->getScreenFormat().bytesPerPixel == 1)
		_system->getPaletteManager()->setPalette(rgb, start, count);
}

void Renderer::presentBorder() {
	const Graphics::PixelFormat format = _system->getScreenFormat();
	byte *output = _borderPixels;
	int pitch = kOutputWidth;
	if (format.bytesPerPixel == 2) {
		uint16 colors[256];
		for (uint i = 0; i < 256; ++i)
			colors[i] = format.RGBToColor(_rgbPalette[i * 3], _rgbPalette[i * 3 + 1], _rgbPalette[i * 3 + 2]);
		for (uint i = 0; i < kOutputWidth * kOutputHeight; ++i)
			_rgbPixels[i] = colors[_borderPixels[i]];
		output = reinterpret_cast<byte *>(_rgbPixels);
		pitch *= 2;
	}
#ifdef USE_ASPECT
	if (stretchBorder())
		stretch200To240(output, pitch, kOutputWidth, kOutputHeight, 0, 0, 0,
			_system->getFeatureState(OSystem::kFeatureFilteringMode), format);
#endif
	_system->copyRectToScreen(output, pitch, 0, 0, kOutputWidth, outputHeight());
}

void Renderer::clearRasterPalette() {
	rasterActive = false;
	setPalette(currentPalette);
}

void Renderer::presentRaster(byte *pixels) {
	static const byte levels[8] = {0, 0x22, 0x44, 0x66, 0x88, 0xaa, 0xcc, 0xee};
	int16 indices[4096];
	memset(indices, -1, sizeof(indices));
	byte rgb[256 * 3];
	uint count = 0;
	for (int row = 0; row < 200; ++row) {
		byte mapping[16];
		for (int entry = 0; entry < 16; ++entry) {
			uint16 color = rasterRows[row][entry] & 0x777;
			if (indices[color] < 0) {
				if (count == 256) error("CBlood: raster palette exceeds 256 colors");
				indices[color] = count;
				rgb[count * 3] = levels[(color >> 8) & 7];
				rgb[count * 3 + 1] = levels[(color >> 4) & 7];
				rgb[count * 3 + 2] = levels[color & 7];
				++count;
			}
			mapping[entry] = byte(indices[color]);
		}
		_borderRows[row] = mapping[0];
		for (int x = 0; x < 320; ++x) pixels[row * 320 + x] = mapping[pixels[row * 320 + x]];
	}
	publishPalette(rgb, 0, count);
}

void Renderer::setPalette(const uint16 *colors) {
	static const byte levels[8] = {0, 0x22, 0x44, 0x66, 0x88, 0xaa, 0xcc, 0xee};
	byte rgb[48];
	for (int i = 0; i < 16; ++i) {
		currentPalette[i] = colors[i];
		rgb[i * 3] = levels[(colors[i] >> 8) & 7];
		rgb[i * 3 + 1] = levels[(colors[i] >> 4) & 7];
		rgb[i * 3 + 2] = levels[colors[i] & 7];
	}
	publishPalette(rgb, 0, 16);
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
	static const byte levels[8] = {0, 0x22, 0x44, 0x66, 0x88, 0xaa, 0xcc, 0xee};
	byte rgb[48];
	for (int i = 0; i < 16; ++i) {
		uint16 color = READ_BE_UINT16(colors + 32 + i * 2);
		rgb[i * 3] = levels[(color >> 8) & 7];
		rgb[i * 3 + 1] = levels[(color >> 4) & 7];
		rgb[i * 3 + 2] = levels[color & 7];
	}
	publishPalette(rgb, 16, 16);
}

} // namespace CBlood
