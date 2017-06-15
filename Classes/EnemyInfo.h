#pragma once
#include "cocos2d.h"

using namespace std;
USING_NS_CC;

enum EnemyBehavior {
	RANDOM,
	TRACE,
	REPEAT,
	JUMP,
	AI
};

class EnemyInfo {
public:
	// 生命值
	int healthPoint = 20;
	int totalHealthPoint = 20;
	// 攻击力
	int attack = 1;
	// 两次攻击最小间隔。寒冰射手能使这个间隔变大。
	float attackDelay = 1.5f;
	// 速度
	float velocity = 10;
	// 初始速度。用于寒冰射手减少和恢复僵尸的速度。
	float primaryVelocity = 10;
	// 冻住时的时间戳。用于计算被寒冰射手射中的僵尸是否解冻。
	long frozenTimeStamp;
	// 冻住时长。为0则没有被冻住。
	long frozenDuration = 0;
	EnemyBehavior beheavior = REPEAT;

	int tracingPlayer = -1;

	EnemyInfo(EnemyBehavior beheavior = REPEAT, int _healthPoint = 10, int _attack = 1, float _attackDelay = 1.5f, float _velocity = 15);

	static EnemyBehavior beheaviorOfString(const string &b);

};

class EnemeyInfoFactory {
public:
	
	static EnemeyInfoFactory *getDefault() {
		if (defaultInstance == nullptr) {
			defaultInstance = new EnemeyInfoFactory();
			defaultInstance->zombies["Zombie"] = EnemyInfo(EnemyBehavior::REPEAT);
			defaultInstance->zombies["Ballon"] = EnemyInfo(EnemyBehavior::TRACE);
			defaultInstance->zombies["Gargantuar"] = EnemyInfo(EnemyBehavior::REPEAT, 100, 3);
			defaultInstance->zombies["Snowman"] = EnemyInfo(EnemyBehavior::REPEAT, 50, 3);
			defaultInstance->zombies["Spaceman"] = EnemyInfo(EnemyBehavior::REPEAT, 40, 1, 1, 120);
			defaultInstance->zombies["CurlyHair"] = EnemyInfo(EnemyBehavior::REPEAT, 10, 3);
			defaultInstance->zombies["Imp"] = EnemyInfo(EnemyBehavior::REPEAT, 5, 1, 1, 80);
			defaultInstance->zombies["Diver"] = EnemyInfo(EnemyBehavior::REPEAT, 40, 3);
			defaultInstance->zombies["Tie"] = EnemyInfo(EnemyBehavior::TRACE, 10, 1, 1.5f, 50);
			defaultInstance->zombies["Conehead"] = EnemyInfo(EnemyBehavior::REPEAT, 30);
		}
		return defaultInstance;
	}

	EnemyInfo *createEnemyInfo(const string &name) const {
		return new EnemyInfo(zombies.at(name));
	}

	void putEnemyInfo(const string &name, const EnemyInfo &info) {
		zombies[name] = info;
	}

private:
	static EnemeyInfoFactory *defaultInstance;
	map<string, EnemyInfo> zombies;
};