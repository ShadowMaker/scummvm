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

#include "base/plugins.h"
#include "common/gui_options.h"
#include "engines/advancedDetector.h"
#include "cblood/detection.h"

namespace CBlood {

static const PlainGameDescriptor cbloodGames[] = {
	{ "cblood-st", "Captain Blood" },
	{ 0, 0 }
};

const ADGameDescription gameDescriptions[] = {
	{
		"cblood-st",
		"",
		{
			{ "BLOOD_H", 0, "9842626db0f04b383bf05f6e8fd05400", 32898 },
			{ "BLOOD_I", 0, "66a71d598f1f1cfe46dc4587a13d2891", 64064 },
			{ "BLOOD_VX", 0, "db44781007c0e49e59d9a85eb5826eaa", 46905 },
			{ "ETHNICOL.JAR", 0, "922b14ba76da42aeaa949828fd8da5f0", 185187 },
			AD_LISTEND
		},
		Common::EN_ANY,
		Common::kPlatformAtariST,
		ADGF_UNSTABLE,
		GUIO2(GUIO_NOMIDI, GUIO_RENDERATARIST)
	},

	AD_TABLE_END_MARKER
};

} // End of namespace CBlood

class CBloodMetaEngineDetection : public AdvancedMetaEngineDetection<ADGameDescription> {
public:
	CBloodMetaEngineDetection() : AdvancedMetaEngineDetection(CBlood::gameDescriptions, CBlood::cbloodGames) {
	}

	const char *getName() const override {
		return "cblood";
	}

	const char *getEngineName() const override {
		return "Captain Blood";
	}

	const char *getOriginalCopyright() const override {
		return "Captain Blood (C) 1988 ERE Informatique";
	}
};

REGISTER_PLUGIN_STATIC(CBLOOD_DETECTION, PLUGIN_TYPE_ENGINE_DETECTION, CBloodMetaEngineDetection);
