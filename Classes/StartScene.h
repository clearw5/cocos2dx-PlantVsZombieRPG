#pragma once

#include "cocos2d.h"
#include "ui/CocosGUI.h"
USING_NS_CC;
using namespace cocos2d::ui;

class StartScene : public cocos2d::Layer
{
public:
	static cocos2d::Scene* createScene();

	virtual bool init();
	// implement the "static create()" method manually
	CREATE_FUNC(StartScene);

private:
	int level = 0;
};
