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
#include "cblood/screen_controller.h"
#include "cblood/destruction_timing.h"
#include "common/debug.h"

namespace CBlood {

void PlanetScreen::close() {
	_planetActive = _destroyQueued = _emptyView = _paletteActive = false;
	if (_renderer.rasterActive) _renderer.clearRasterPalette();
}

void PlanetScreen::handleInput() {
	if (_destroyQueued) {
		_destroyQueued = false;
		_host.waitForDraw();
		if (!_host.shouldQuit()) runDestruction();
	} else if (_emptyView && (_panel.pointerButtonLatch & 2)) {
		finishPlanetView();
	}
}

bool PlanetScreen::paletteFrame() {
	if (!_paletteActive) return false;
	advancePalette();
	for (int y = 0; y < 200; ++y)
		memcpy(_renderer.rasterRows[y], _renderer.currentPalette, sizeof(_renderer.currentPalette));
	_raster.bands(_rasterDelays, [this](int row, int offset) {
		for (int y = row; y < 200; ++y)
			for (int color = 0; color < 16; ++color)
				_renderer.rasterRows[y][color] = offset < 0 ? _renderer.currentPalette[color] :
					READ_BE_UINT16(_rasterColors + offset + color * 2);
	});
	_renderer.rasterActive = true;
	return true;
}

void PlanetScreen::advancePalette() {
	if (!_paletteActive) return;
	const uint32 frames = uint32(uint64(_host.millis() - _paletteEpoch) * 8021247 / 160256000);
	while (_paletteFrames < frames) {
		_raster.tick();
		++_paletteFrames;
	}
}

void PlanetScreen::waitDestruct(uint32 start, uint32 cycles) {
	const uint32 milliseconds = uint32((uint64(cycles) * 1000 + 8021246) / 8021247);
	while (!_host.shouldQuit() && _host.millis() - start < milliseconds) {
		_host.pumpFrames();
		_host.delay(1);
	}
}

void PlanetScreen::makeDestructImage(byte *image, byte *field, byte *pattern, uint16 midpoint, uint phase) {
	uint32 start = _host.millis();
	DestructImage generator(field, pattern, _state.localSeed >> 16,
		_resources.readU16(0x1ba8c), midpoint);
	generator.begin();
	for (uint column = 0; column < 20 && !_host.shouldQuit(); ++column) {
		generator.drawColumn(image);
		_host.pumpFrames();
	}
	_state.localSeed = (_state.localSeed & 0xffff) | (uint32(generator.seed()) << 16);
	waitDestruct(start, kDestructBuild[phase]);
	debug(1, "cblood: destruction image %u built in %u ms", phase + 1, _host.millis() - start);
}

uint16 PlanetScreen::waitPoints(uint phase) {
	advancePalette();
	_raster.countdown = phase < 2 ? 100 : phase == 2 ? 160 : 60;
	WRITE_BE_UINT16(_depthState + 22, 0xff00);
	const DestructTiming &timing = kDestructWaits[phase];
	uint32 start = _host.millis(), elapsed = 0;
	uint pass = 0;
	uint16 last = 0;
	do {
		last = Planet::projectPoints(_renderer.screen, _depthPoints, _depthState, _state.localSeed, 4);
		elapsed += timing.cycles[pass % timing.count];
		++pass;
		waitDestruct(start, elapsed);
		advancePalette();
	} while (!_host.shouldQuit() && int16(_raster.countdown) >= 0);
	WRITE_BE_UINT16(_depthState + 22, 0);
	debug(1, "cblood: destruction wait %u ended in %u ms, %u projections", phase + 1,
		_host.millis() - start, pass);
	return last;
}

void PlanetScreen::runDestruction() {
	const uint32 setupStart = _host.millis();
	_panel.revealFrame = 0;
	_state.oorxxVisitScan = 0xffff;
	_panel.cursorYClamp = 50;
	byte script[182];
	_resources.readResource(0x1a65a, script, sizeof(script));
	_resources.readResource(0x1a2ee, _rasterDelays, sizeof(_rasterDelays));
	PaletteSequence palette;
	if (!palette.build(script, sizeof(script), _rasterColors, sizeof(_rasterColors)))
		error("CBlood: invalid destruction palette script");
	WRITE_BE_UINT16(_depthState + 26, 96);
	for (uint i = 0; i < 97; ++i) {
		byte *point = _depthPoints + i * 12;
		WRITE_BE_UINT32(point, 0);
		WRITE_BE_UINT16(point + 4, Planet::shiftPoint(int16(Planet::nextRandom(_state.localSeed)), 63));
		WRITE_BE_UINT16(point + 6, Planet::shiftPoint(int16(Planet::nextRandom(_state.localSeed)), 63));
		WRITE_BE_UINT16(point + 8, Planet::nextRandom(_state.localSeed) & 0xfff);
	}
	waitDestruct(setupStart, kDestructSetup);
	if (_host.shouldQuit()) return;
	_sound.startSpeech(0x1cff2);
	_planetState[24] |= 0x80;
	if (!_state.destroyPlanet()) error("CBlood: invalid planet state for destruction");
	debug(1, "cblood: destruction started, Destroy Planet speech playing");
	WRITE_BE_UINT16(_depthState + 22, 0xff80);
	WRITE_BE_UINT32(_depthState + 18, 0x7ff00002);
	const uint32 start = _host.millis();
	uint16 last = 0;
	bool skipped = false;
	for (uint pass = 0; pass < 512 && !_host.shouldQuit(); ++pass) {
		if (_panel.pointerButtonLatch & 1) {
			_panel.pointerButtonLatch &= ~1;
			skipped = true;
			break;
		}
		uint32 step = READ_BE_UINT32(_depthState + 18);
		last = Planet::projectPoints(_renderer.screen, _depthPoints, _depthState, _state.localSeed,
			Planet::swapWords(step & 0x70007));
		WRITE_BE_UINT32(_depthState + 18, step + 0x800);
		waitDestruct(start, kDestructOpening[pass]);
	}
	debug(1, "cblood: destruction opening %s in %u ms", skipped ? "skipped" : "finished", _host.millis() - start);
	if (!skipped && !_host.shouldQuit()) {
		byte field[DestructImage::kFieldBytes] = {};
		byte pattern[DestructImage::kPatternBytes];
		byte image[DestructImage::kImageBytes];
		_resources.readResource(0x1b9a2, pattern, sizeof(pattern));
		_raster = {_resources.readU16(0x1a2dc), _resources.readU16(0x1a2de), 2, 0};
		_paletteEpoch = _host.millis();
		_paletteFrames = 0;
		_paletteActive = true;
		for (uint phase = 0; phase < 2 && !_host.shouldQuit(); ++phase) {
			advancePalette();
			_raster.offset = 0;
			makeDestructImage(image, field, pattern, last, phase);
			last = waitPoints(phase);
			if (_host.shouldQuit()) break;
			uint32 copyStart = _host.millis();
			waitDestruct(copyStart, kDestructCopy[phase]);
			memcpy(_renderer.screen + 20 * 160, image, 123 * 160);
			debug(1, "cblood: destruction image %u published", phase + 1);
		}
		if (!_host.shouldQuit()) {
			advancePalette();
			_raster.offset = 0x1e0;
			waitPoints(2);
			advancePalette();
			_raster.step = 0xfffe;
			_raster.offset = 0x200;
			if (!_host.shouldQuit()) waitPoints(3);
		}
	}
	if (!_host.shouldQuit()) {
		WRITE_BE_UINT16(_depthState + 26, Planet::initPoints(_depthPoints, _state.localSeed));
		memset(_renderer.screen + 18 * 160, 0, 128 * 160);
		Planet::projectPoints(_renderer.screen, _depthPoints, _depthState, _state.localSeed);
	}
	_paletteActive = false;
	_renderer.clearRasterPalette();
	_renderer.splitPalette = false;
	_panel.pointerButton = _panel.pointerButtonLatch = 0;
	_panel.updateButtons(0);
	_panel.cursorYClamp = 104;
	_emptyView = true;
	debug(1, "cblood: destruction complete, awaiting click");
}

void PlanetScreen::finishPlanetView() {
	if (_emptyView) _panel.revealFrame = 0;
	_panel.pointerButtonLatch = 0;
	if (!_panel.revealFrame) {
		const uint32 descriptor = 0x17ec0;
		uint source = _resources.readU32(descriptor) - 0x58000;
		uint destination = _resources.readU32(descriptor + 4) - 0x68000 + 0x960;
		uint bytes = (_resources.readU16(descriptor + 8) + 1) * 8;
		uint rows = _resources.readU16(descriptor + 10) + 1;
		for (uint row = 0; row < rows; ++row) {
			if (source + bytes > _resources.bloodM.size() || destination + bytes > sizeof(_renderer.backup))
				error("CBlood: invalid cockpit return patch");
			memcpy(_renderer.backup + destination, &_resources.bloodM[source], bytes);
			source += bytes + _resources.readS16(descriptor + 12);
			destination += bytes + _resources.readS16(descriptor + 14);
		}
	}
	_emptyView = false;
	_host.requestCockpit();
}

} // namespace CBlood
