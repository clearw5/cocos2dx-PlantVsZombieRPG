#include "EnemyInfo.h"



static map<string, EnemyBehavior> behaviors;
EnemeyInfoFactory *EnemeyInfoFactory::defaultInstance = nullptr;

EnemyInfo::EnemyInfo(EnemyBehavior _beheavior, int _healthPoint, int _attack, float _attackDelay, float _velocity) :
	healthPoint(_healthPoint),
	attack(_attack),
	attackDelay(_attackDelay),
	velocity(_velocity),
	primaryVelocity(_velocity),
	beheavior(_beheavior),
	totalHealthPoint(_healthPoint)
{
	totalHealthPoint = healthPoint;
}


EnemyBehavior EnemyInfo::beheaviorOfString(const string &b) {
	if (behaviors.empty()) {
		behaviors["random"] = EnemyBehavior::RANDOM;
		behaviors["jump"] = EnemyBehavior::JUMP;
		behaviors["trace"] = EnemyBehavior::TRACE;
		behaviors["repeat"] = EnemyBehavior::REPEAT;
		behaviors["ai"] = EnemyBehavior::AI;
	}
	return behaviors[b];
}

