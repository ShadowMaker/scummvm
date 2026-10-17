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
#ifndef CBLOOD_STARTUP_SCREEN_H
#define CBLOOD_STARTUP_SCREEN_H
#include "cblood/screen.h"
#include "common/array.h"
#include "cblood/startup.h"
namespace CBlood {
class PlanetScreen;
class StartupScreen : public Screen {
public:
	StartupScreen(ScreenController &host, GameState &state, Resources &resources,
		Renderer &renderer, CockpitPanel &panel, Sound &sound, PlanetScreen &planet);
	bool runStartup();
	void handleEvent(const Common::Event &event) override;
	void pollMusic();
	void tickFrame();
	bool introActive() const { return _introActive; }
private:
	bool runTitleLoop();
	void fadeIntro(int image);
	void fadeIntroColors(const uint16 *colors, int level, int step);
	bool chooseLanguage();
	bool initPlanets();
	bool loadFile(const char *name, Common::Array<byte> &data);
	PlanetScreen &_planet;
	Startup::Intro _intro;
	bool _introActive, _keyPressed, _markerSeen;
	int _selectedImage, _activePalette;
};
} // namespace CBlood
#endif
