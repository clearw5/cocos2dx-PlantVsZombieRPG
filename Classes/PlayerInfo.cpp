#include "PlayerInfo.h"

static map<string, PlantInfo> plants;

PlantInfo::PlantInfo(int totalHp, int _frozenDuration, int _requiredSunlight, int _velocity, int _jumpingVelocity, long _attackDelay, float _repeatedAttackDelay) :
	totalHealthPoint(totalHp),
	frozenDuration(_frozenDuration * 1000L),
	requiredSunlight(_requiredSunlight),
	velocity(_velocity),
	jumpingVelocity(_jumpingVelocity),
	attackDelay(_attackDelay),
	repeatedAttackDelay(_repeatedAttackDelay)
	{
}

PlantInfo PlantInfo::create(const string &name) {
	if (plants.empty()) {
		plants["Sunflower"] = PlantInfo(5, 5, 50, 50);
		plants["PeaShooter"] = PlantInfo(10, 10, 100, 100, 300, 650, 0.7f);
		plants["WallNut"] = PlantInfo(40, 20, 100, 50, 200);
		plants["Repeater"] = PlantInfo(10, 15, 200, 120, 320);
		plants["SnowPea"] = PlantInfo(10, 20, 200, 100, 300);
		plants["Jalapeno"] = PlantInfo(5, 30, 300, 100, 300);
	}
	return plants[name];
}

 PlantInfo PlantInfo::createWithFullHp(const string &name) {
	 auto plantInfo = create(name);
	 plantInfo.healthPoint = plantInfo.totalHealthPoint;
	 return plantInfo;
}