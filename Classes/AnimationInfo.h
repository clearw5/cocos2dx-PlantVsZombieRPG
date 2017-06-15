#pragma once
#include "cocos2d.h"
using namespace std;
USING_NS_CC;

class AnimationInfo {
public:
	void setAnim(const string & name, unsigned int frameCount, const Size &frameSize, float delay = 0.1f);
	size_t getAnimFrameCount(const string & name);
	const Size & getAnimFrameSize(const string & name);
	float  getAnimDelay(const string & name);
	
private:
	map<string, size_t> frameCounts;
	map<string, Size> frameSizes;
	map<string, float> delays;
};