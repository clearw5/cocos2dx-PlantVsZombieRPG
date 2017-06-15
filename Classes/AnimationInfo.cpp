#include "AnimationInfo.h"

void AnimationInfo::setAnim(const string & name, unsigned int frameCount, const Size &frameSize, float delay){
	frameCounts[name] = frameCount;
	frameSizes[name] = frameSize;
	delays[name] = delay;
}
size_t  AnimationInfo::getAnimFrameCount(const string & name){
	return frameCounts[name];
}
const Size &  AnimationInfo::getAnimFrameSize(const string & name){
	return frameSizes[name];
}

float  AnimationInfo::getAnimDelay(const string & name) {
	return delays[name];
}