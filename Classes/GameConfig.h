#pragma once
#include "cocos2d.h"
#include "EnemyInfo.h"
#include "PlayerInfo.h"
using namespace std;
USING_NS_CC;

typedef  map<EventKeyboard::KeyCode, EventKeyboard::KeyCode> KeyCodeMapper;
typedef EventKeyboard::KeyCode KeyCode;

class GameConfig {
public:
	GameConfig(const string mapPath = "map0.tmx", EnemeyInfoFactory* _enemyInfoFactory = EnemeyInfoFactory::getDefault()) :
		gameMapPath(mapPath),
		enemyInfoFactory(_enemyInfoFactory)
	{

		KeyCodeMapper player2keyCodeMapper = {
			{KeyCode::KEY_LEFT_ARROW, KeyCode::KEY_A },
			{KeyCode::KEY_RIGHT_ARROW, KeyCode::KEY_D },
			{KeyCode::KEY_UP_ARROW, KeyCode::KEY_W },
			{KeyCode::KEY_DOWN_ARROW, KeyCode::KEY_S},
			{KeyCode::KEY_1, KeyCode::KEY_J },
			{KeyCode::KEY_2, KeyCode::KEY_K },
			{KeyCode::KEY_3, KeyCode::KEY_L },
			{ KeyCode::KEY_4, KeyCode::KEY_U },
			{ KeyCode::KEY_5, KeyCode::KEY_I },
			{ KeyCode::KEY_6, KeyCode::KEY_O }
		};
		keyCodeMappers[1] = player2keyCodeMapper;
	}
	static GameConfig defaultConfig(int playerCount = 2, const string &defaultPlantName = "Sunflower") {
		GameConfig config;
		for (int i = 0; i < playerCount; i++) {
			PlayerInfo playerInfo;
			playerInfo.playerIndex = i;
			playerInfo.addPlant(defaultPlantName, PlantInfo::createWithFullHp(defaultPlantName));
			config.addPlayer(playerInfo);
		}
		return config;
	}


	static GameConfig fromValueMap(const ValueMap &m) {
		int level = m.at("level").asInt();
		GameConfig config;
		config.playerInfos.clear();
		auto playerInfos = m.find("playerInfos");
		if (playerInfos != m.end()) {
			for (auto &playInfo : playerInfos->second.asValueVector()) {
				PlayerInfo p = PlayerInfo::fromValueMap(playInfo.asValueMap());
				config.addPlayer(p);
			}
		}
		config.setGameLevel(level);
		return config;
	}

	void toValueMap(ValueMap &m) {
		m["level"] = gameLevel;
		ValueVector playerInfos;
		for (auto &playerInfo : this->playerInfos) {
			ValueMap playerInfoMap;
			playerInfo.toValueMap(playerInfoMap);
			playerInfos.push_back(Value(playerInfoMap));
		}
		m["playerInfos"] = playerInfos;
	}

	string getGameMapPath() {
		return gameMapPath;
	}

	PlayerInfo getPlayerInfo(int i) {
		return playerInfos[i];
	}

	size_t getPlayerCount() {
		return playerInfos.size();
	}
	const EnemeyInfoFactory &getEnemeyInfoFactory() {
		return *enemyInfoFactory;
	}
	void addPlayer(PlayerInfo info) {
		info.playerIndex = playerInfos.size();
		playerInfos.push_back(info);
	}
	void clearPlayers() {
		playerInfos.clear();
	}

	const KeyCodeMapper &getKeyCodeMapper(int player) {
		return keyCodeMappers[player];
	}

	int getGameLevel() {
		return gameLevel;
	}

	void setGameLevel(int level) {
		gameLevel = level;
		gameMapPath = "map" + to_string(level) + ".tmx";
 	}

	void clearTransitveValues() {
		for (size_t i = 0; i < playerInfos.size(); i++) {
			playerInfos[i].clearTransitveValues();
		}
	}

private:
	int gameLevel = 0;
	vector<PlayerInfo> playerInfos;
	EnemeyInfoFactory *enemyInfoFactory;
	string gameMapPath;
	map<int, KeyCodeMapper> keyCodeMappers;
};