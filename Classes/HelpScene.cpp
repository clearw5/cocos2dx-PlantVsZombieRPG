#include "HelpScene.h"
#include "StartScene.h"

USING_NS_CC;


Scene* HelpScene::createScene()
{
	// 'scene' is an autorelease object
	auto scene = Scene::create();

	// 'layer' is an autorelease object
	auto layer = HelpScene::create();

	// add layer as a child to scene
	scene->addChild(layer);

	// return the scene
	return scene;
}

// on "init" you need to initialize your instance
bool HelpScene::init()
{
	//////////////////////////////
	// 1. super init first
	if (!Layer::init())
	{
		return false;
	}

	Size visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	auto bg = Sprite::create("startbg.jpg");
	bg->setScale(visibleSize.width / bg->getContentSize().width, visibleSize.height / bg->getContentSize().height);
	bg->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y));
	this->addChild(bg, 0);

	auto back_button = Button::create();
	back_button->setTitleText("BACK");
	back_button->setTitleFontName("fonts/ChronicGothic.ttf");
	back_button->setTitleFontSize(36);
	back_button->setPosition(Size(visibleSize.width - back_button->getContentSize().width, origin.y + back_button->getContentSize().height));
	this->addChild(back_button, 1);
	back_button->addClickEventListener([](Ref* ref) {
		Director::sharedDirector()->replaceScene(StartScene::createScene());
	});



	auto letterBg = Sprite::create("letter_bg.png");
	letterBg->setScale(1.5);
	letterBg->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y - 50));
	this->addChild(letterBg, 0);

	auto help = Label::createWithTTF(TTFConfig("fonts/ChronicGothic.ttf", 30), "Player 1\nwasd           Control\nJ              Attack\nK              Switch Plant\nL              Grow Plant\nPlayer 2\n1              Attack\n2              Switch Plant\n3              Grow Plant\n\nEnter           Pause");
	help->setTextColor(Color4B(1, 0, 0, 255));
	help->setPosition(Vec2(visibleSize.width / 2 - 45, visibleSize.height / 2 - 40));
	addChild(help);

	return true;

}

