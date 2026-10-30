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
#include "common/debug.h"
#include "common/system.h"

namespace CBlood {
CockpitPanel::CockpitPanel(ScreenController &host, GameState &state, Resources &resources, Renderer &renderer)
	: handActive(false), buttonMask(0xffff), cursorX(160), cursorY(50), cursorYClamp(50),
	pointerButton(0), pointerButtonLatch(0), revealFrame(0), _host(host), _state(state),
	_resources(resources), _renderer(renderer), handAnimation(0), oldHandHeight(-1),
	oldHandDest(32000), _handTop(0x400) {
	memset(savedHand, 0, sizeof(savedHand));
	resetClock();
}
void CockpitPanel::initData() {
	HandMotion &motion = _state.handMotion;
	motion.gate = _resources.readU16(0x174aa);
	motion.delayX = _resources.readU16(0x174ac);
	motion.reloadX = _resources.readU16(0x174ae);
	motion.stepX = _resources.readU16(0x174b0);
	motion.delayY = _resources.readU16(0x174b4);
	motion.reloadY = _resources.readU16(0x174b6);
	motion.stepY = _resources.readU16(0x174b8);
	_resources.readResource(0x17d6a, digits, sizeof(digits));
	_resources.readResource(0x17cf0, _resources.digitGlyphs, sizeof(_resources.digitGlyphs));
	_resources.readResource(0x17d54, clockDigits, sizeof(clockDigits));
}
void CockpitPanel::resetClock() {
	shownClock = clockReply = 0;
	clockReplyReady = false;
	clockPollTicks = -1;
}
void CockpitPanel::tickClock() {
	if (clockPollTicks >= 0) --clockPollTicks;
}
void CockpitPanel::pollGameClock() {
	if (clockPollTicks >= 0) return;
	clockPollTicks = 16;
	// Consume the previous clock reply, not the result of the new request.
	uint16 elapsed = clockReply;
	bool ready = clockReplyReady;
	clockReply = _state.gameClock(_host.playTime());
	clockReplyReady = true;
	if (!ready) return;
	if (elapsed == shownClock) return;
	shownClock = elapsed;
	_state.handMotion.refresh(elapsed, _state.bloodDeadline, _state.forceHandTremor);
	Planet::formatClock(clockDigits, elapsed);
	Planet::drawDigits(_renderer.screen, _resources.digitGlyphs, clockDigits);
	debug(1, "cblood: clock display %u", elapsed);
}

void CockpitPanel::updateButtons(uint16 requested) {
	requested &= ~0x202; // No carried alien or completed visit during startup.
	uint16 previous = buttonMask;
	buttonMask = requested;
	Startup::buttons(previous, requested, [this](int button, bool covered) {
		_host.queueDraw(covered ? kCover : kReveal, button);
	});
}

void CockpitPanel::handleEvent(const Common::Event &event) {
	if (!handActive) return;
	byte buttons = pointerButton;
	int dx = 0, dy = 0;
	switch (event.type) {
	case Common::EVENT_MOUSEMOVE:
		dx = event.relMouse.x;
		dy = -event.relMouse.y;
		if (_renderer.stretchBorder()) {
			int scaled = dy * 5 + _mouseYFraction;
			dy = scaled / 6;
			_mouseYFraction = scaled % 6;
		} else {
			_mouseYFraction = 0;
		}
		break;
	case Common::EVENT_LBUTTONDOWN: buttons |= 2; break;
	case Common::EVENT_LBUTTONUP: buttons &= ~2; break;
	case Common::EVENT_RBUTTONDOWN: buttons |= 1; break;
	case Common::EVENT_RBUTTONUP: buttons &= ~1; break;
	default: return;
	}
	Startup::moveHand(cursorX, cursorY, cursorYClamp, pointerButton, pointerButtonLatch, dx, dy, buttons);
}
void CockpitPanel::restoreHand() {
	Startup::restoreHand(_renderer.screen, savedHand, oldHandDest, oldHandHeight);
}
void CockpitPanel::updateHand() {
	int y = cursorY + (pointerButton ? 6 : 0);
	if (y > cursorYClamp) cursorY = y = cursorYClamp;
	int top = 0x400;
	if (pointerButton) {
		handAnimation += 0x60;
		top -= (handAnimation & 0x300) + 0x100;
	}
	oldHandHeight = MIN(y, handRows);
	oldHandDest = (200 - y) * 160 + (cursorX & ~15) / 2;
	_handTop = top;
	const int rows = oldHandHeight <= 15 ? oldHandHeight + 1 : oldHandHeight + 2;
	for (int row = 0; row < rows; ++row)
		memcpy(savedHand + row * 24, _renderer.screen + oldHandDest + row * 160, 24);
}
void CockpitPanel::drawHand() {
	byte scratch[2664];
	Startup::mergeHand(_renderer.screen, scratch, &_resources.bloodM[0], oldHandDest,
		oldHandHeight, cursorX & 15, _handTop);
	_state.handMotion.tick(cursorX, cursorY, cursorYClamp);
}
void CockpitPanel::draw(DrawJob job, int button) {
	if (job == kCover || job == kReveal) {
		uint32 descriptor = 0x17dfe + button * 8;
		int destination = _resources.readU16(descriptor + 2);
		if (job == kCover) {
			byte saved[20 * 24];
			Startup::mergeRows(_renderer.screen, saved, &_resources.bloodM[0xbc0], destination,
				_resources.readU16(descriptor + 6) + 1, 0);
		} else {
			for (int row = 0; row < 20; ++row)
				memcpy(_renderer.screen + destination + row * 160, _renderer.cockpit + destination + row * 160, 16);
		}
	} else if (job == kBackup) {
		memcpy(_renderer.backup + 0x960, _renderer.screen + 0x960, 21600);
	} else if (job == kRestore) {
		memcpy(_renderer.screen + 0x960, _renderer.backup + 0x960, 21600);
	} else if (job == kFlags) {
		Startup::drawFlags(_renderer.screen, _resources.flags);
	} else if (job == kOorxx) {
		uint16 frame = revealFrame;
		Startup::revealOorxx(_renderer.screen, &_resources.bloodM[0x10a0], frame);
	} else if (job == kDigits) {
		Planet::drawDigits(_renderer.screen, _resources.digitGlyphs, digits);
		Planet::drawDigits(_renderer.screen, _resources.digitGlyphs, digits + 6);
	}
}

} // namespace CBlood
