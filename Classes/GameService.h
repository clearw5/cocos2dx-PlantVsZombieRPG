#pragma once
#include "GameConfig.h"

class GameService {
public:
	static bool hasSavedGame() {
		return UserDefault::getInstance()->getBoolForKey("hasSavedGame");
	}

	static GameConfig getSavedConfig() {
		auto path = UserDefault::getInstance()->getStringForKey("savedPath");
		auto valueMap = FileUtils::getInstance()->getValueMapFromFile(path);
		return GameConfig::fromValueMap(valueMap);
	}

	static void saveGameConfig( GameConfig &config) {
		ValueMap m;
		config.toValueMap(m);
		FileUtils::getInstance()->writeValueMapToFile(m, "config.xml");
		UserDefault::getInstance()->setStringForKey("savedPath", "config.xml");
		UserDefault::getInstance()->setBoolForKey("hasSavedGame", true);
	}

	static bool allowCheat;
};