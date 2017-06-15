#include "startScene.h"
#include "ZVP.h"
#include "HelpScene.h"
#include "SimpleAudioEngine.h"
USING_NS_CC;

bool GameService::allowCheat = false;

Scene* StartScene::createScene()
{
	auto scene = Scene::create();

	auto layer = StartScene::create();

	// add layer as a child to scene
	scene->addChild(layer);

	// return the scene
	return scene;
}

// on "init" you need to initialize your instance
bool StartScene::init()
{
	//////////////////////////////
	// 1. super init first
	if (!Layer::init())
	{
		return false;
	}
	//UserDefault::getInstance()->setBoolForKey("hasSavedGame", false);

	Size visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	auto bg = Sprite::create("bg.jpg");
	bg->setScale(visibleSize.width / bg->getContentSize().width, visibleSize.height / bg->getContentSize().height);
	bg->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y));
	this->addChild(bg, 0);


	auto builder = ButtonMenuBuilder()
		.font("fonts/ChronicGothic.ttf")
		.fontSize(46)
		.margin(16);
	if (GameService::hasSavedGame()) {
		builder.addItem("CONTINUE", [=](Ref* ref) {
			Director::sharedDirector()->replaceScene(ZVP::createScene(GameService::getSavedConfig()));
		});
	}
	auto menu = builder
		.addItem("1 PLAYER", [=](Ref* ref) {
			auto gameConfig = GameConfig::defaultConfig(1);
			gameConfig.setGameLevel(level);
			Director::sharedDirector()->replaceScene(ZVP::createScene(gameConfig));
		})
		.addItem("2 PLAYERS", [=](Ref* ref) {
			auto gameConfig = GameConfig::defaultConfig(2);
			gameConfig.setGameLevel(level);
			Director::sharedDirector()->replaceScene(ZVP::createScene(gameConfig));
		})
		.addItem("HELP", [=](Ref* ref) {
			Director::sharedDirector()->replaceScene(HelpScene::createScene());
		})
		.build();

	menu->setPosition(visibleSize / 2);
	addChild(menu);

	auto audio = CocosDenshion::SimpleAudioEngine::getInstance();
	audio->preloadBackgroundMusic("music/start_bgm.mp3");
	audio->playBackgroundMusic("music/start_bgm.mp3", true);

	auto keyboardListener = EventListenerKeyboard::create();
	keyboardListener->onKeyPressed = [=](EventKeyboard::KeyCode code, Event * event) {
		if (code == KeyCode::KEY_ENTER) {
			level++;
		}
		if(code == KeyCode::KEY_SPACE) {
			GameService::allowCheat = true;
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

	return true;
}
