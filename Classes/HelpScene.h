#pragma once
#include "cocos2d.h"
#include "ui/CocosGUI.h"

using namespace cocos2d::ui;

class HelpScene : public cocos2d::Layer
{
public:
	static cocos2d::Scene* createScene();

	virtual bool init();

	// implement the "static create()" method manually
	CREATE_FUNC(HelpScene);
};
