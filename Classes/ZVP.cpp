#include "ZVP.h"
#include "GameService.h"
#include "SimpleAudioEngine.h"
#include "ButtonMenu.h"
#include "StartScene.h"

const bool DEBUG = false;
bool isAudioEnabled = true;

const int tagAnimationPlayerWalking = 11;
const int tagAnimationPlayerAttack = 12;
const int tagAnimationPlayerAttacked = 13;
const int tagActionPlayerAttacking = 14;

const int tagActionMoveBackAndForth = 11;
const int tagActionEnemyFrozen = 12;
const int tagActionZombieAttacking = 13;
const int tagActionEnemyDisplayingHp = 14;

const int tagPlayer = 0x100;
const int tagLand = 0x1000;
const int tagSunlight = 0x2000;
const int tagEnemy = 0x4000;
const int tagPlantCard = 0x8000;
const int tagFixedBlock = 0x10000;
const int tagPea = 0x20000;
const int tagFire = 0x40000;
const int tagRewardBlock = 0x80000;
const int tagStar = 0x100000;

const auto GRAVITY = Vec2(0, -300.0f);
const long PALYER_ATTACK_DELAY = 500;
const float PALYER_REPEATED_ATTACK_DELAY = 0.6f;


const auto COLOR_SPRITE_FROZEN = Color3B(100, 100, 150);
const auto COLOR_CARD_FROZEN = Color3B::BLACK;
const auto COLOR_SUNLIGHT_LABEL = Color3B(0xff, 0xeb, 0x3b);
const auto COLOR_ATTACKED = Color3B(255, 100, 100);
const auto COLOR_PLANT_SUNLIGHT_LABEL = Color3B(0xFF, 0x57, 0x22);

const TTFConfig TTF_SUNLIGHT_LABEL("fonts/Marker Felt.ttf", 30);
const TTFConfig TTF_PLANT_HP_LABEL("fonts/Marker Felt.ttf", 20);
const TTFConfig TTF_PLANT_SUNLIGHT_LABEL("fonts/Marker Felt.ttf", 16);
const TTFConfig TTF_LEVEL("fonts/Marker Felt.ttf", 64);


cocos2d::Scene * ZVP::createScene(GameConfig config)
{
	srand((unsigned)time(NULL));
	auto scene = Scene::createWithPhysics();

	if (DEBUG) {
		scene->getPhysicsWorld()->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);
		Director::getInstance()->setDisplayStats(true);
	}
	scene->getPhysicsWorld()->setGravity(GRAVITY);

	auto gameLayer = ZVP::create(config);
	gameLayer->setPhysicsWorld(scene->getPhysicsWorld());
	scene->addChild(gameLayer, 1);

	auto bgLayer = createBackgroundLayer();
	scene->addChild(bgLayer, 0);

	scene->addChild(gameLayer->infoLayer, 2);

	return scene;
}

void ZVP::setPhysicsWorld(PhysicsWorld * world)
{
	m_world = world;
}

Layer* ZVP::createBackgroundLayer() {
	Size visibleSize = Director::getInstance()->getVisibleSize();
	auto bgLayer = Layer::create();
	auto bgSprite = Sprite::create("bg.png");
	bgSprite->setPosition(visibleSize / 2);
	auto visibleSizeWidth = Director::getInstance()->getVisibleSize().width;
	bgSprite->setScale(visibleSizeWidth / bgSprite->getContentSize().width, visibleSize.height / bgSprite->getContentSize().height);
	bgLayer->addChild(bgSprite);
	return bgLayer;
}


bool ZVP::init()
{
	if (!Layer::init()) {
		return false;
	}

	CCLOG(FileUtils::getInstance()->getDefaultResourceRootPath().c_str());
	//FileUtils::getInstance()->setDefaultResourceRootPath("E:\\YiBin\\cocos\\OurRPG\\Resources\\");

	visibleSize = Director::getInstance()->getVisibleSize();

	loadIdleAnimationInfo();

	loadMap();

	addPlayer();
	
	setViewPoint(players[0]->getPosition());
	renderMapTiles(0, m_map->getMapSize().width);
	renderVisibleMapObjects();

	initGameInfoLayer();

	addListeners();
	preloadMusic();

	
	schedule(schedule_selector(ZVP::update), 0.01f, kRepeatForever, 0.1f);
	schedule(schedule_selector(ZVP::updateEnemys), 0.05f, kRepeatForever, 0.1f);
	schedule(schedule_selector(ZVP::updateGameInfo), 0.05f, kRepeatForever, 0.1f);

	auto level = Label::createWithTTF(TTF_LEVEL, "Level " + to_string(config.getGameLevel()));
	level->setPosition(getPosition() + visibleSize / 2);
	addChild(level, 5);
	level -> runAction(Sequence::create(FadeIn::create(2.0f), FadeOut::create(1.0f), CallFunc::create([=] {
		level->removeFromParent();
	}), nullptr));
	

	return true;
}

void ZVP::loadMap() {
	m_map = TMXTiledMap::create(config.getGameMapPath());
	
	mapPixelWidth = m_map->getMapSize().width * 37.5;
	mapPixelHeight = m_map->getMapSize().height * 37.5;
	mapVisibleRegionWidth = (int)ceil(Director::getInstance()->getVisibleSize().width / 37.5);

	//注册各个方块层对应的"渲染器"
	if (TMXLayer* soil = m_map->getLayer("soil")) {
		tileRenderers[soil] = [this](Sprite *s) ->void {
			renderLandTile(s);
			s->setTag(tagFixedBlock | tagLand);
		};
	}
	if (grasslandLayer = m_map->getLayer("grassland")) {
		tileRenderers[grasslandLayer] = [this](Sprite *s) ->void { renderLandTile(s); };
	}
	if (TMXLayer* sunlights = m_map->getLayer("sunlights")) {
		tileRenderers[sunlights] = [this](Sprite *s) ->void { renderSunlightTile(s); };
	}

	//注册各个对象层对应的"渲染器"
	if (TMXObjectGroup *zombies = m_map->getObjectGroup("zombies")) {
		objectRenders[zombies] = [this](const ValueMap &obj) ->void { renderZombieObject(obj); };
	}
	if (TMXObjectGroup *rewards = m_map->getObjectGroup("rewards")) {
		objectRenders[rewards] = [this](const ValueMap &obj) ->void { renderRewardBlock(obj); };
	}
	if (TMXObjectGroup *moving_blocks = m_map->getObjectGroup("moving_blocks")) {
		objectRenders[moving_blocks] = [this](const ValueMap &obj) ->void { renderMovingBlock(obj); };
	}

	addChild(m_map, 1);
}


void ZVP::renderLandTile(Sprite* landTile) {
	if (landTile == nullptr) {
		return;
	}
	landTile->setTag(tagLand);
	PhysicsBody* body = PhysicsBody::createBox(Size(37.5f, 37.5f), PhysicsMaterial(1.0f, 0.0f, 0.0f));
	body->setDynamic(false);
	body->setGravityEnable(false);
	body->setContactTestBitmask(0xFFFFFFFF);
	landTile->setPhysicsBody(body);
}

void ZVP::renderMovingBlock(const ValueMap &obj) {
	int length = obj.at("length").asInt();
	float moveX = obj.at("moveX").asFloat();
	float moveY = obj.at("moveY").asFloat();
	float duration = obj.at("duration").asFloat();
	

	Sprite* blockGroup = Sprite::create();
	blockGroup->setContentSize(Size(length * 37.5f, 37.5f));
	auto tilePos = getTileCoordFromPosition(Vec2(obj.at("x").asFloat(), obj.at("y").asFloat()));
	blockGroup->setPosition((tilePos.x + 0.5f) * 37.5f +  blockGroup->getContentSize().width / 2, (m_map->getMapSize().height - tilePos.y - 0.5f) * 37.5f);
	blockGroup->setTag(tagLand | tagFixedBlock);

	for (int i = 0; i < length; i++) {
		Sprite* grassLandBlock = createSprite("grassland", 37.5f, 37.5f);
		grassLandBlock->setPosition(37.5f * i, 0);
		blockGroup->addChild(grassLandBlock);	
	}
	
	PhysicsBody* body = PhysicsBody::createBox(Size(length * 37.5f, 37.5f), PhysicsMaterial(1.0f, 0.0f, 0.0f), Vec2(-37.5f / 2, -37.5f/ 2));
	body->setDynamic(false);
	body->setGravityEnable(false);
	body->setContactTestBitmask(0xFFFFFFFF);
	body->setVelocity(Vec2(moveX, moveY) / duration);
	blockGroup->setPhysicsBody(body);
	
	addChild(blockGroup, 1);

	blockGroup->runAction(RepeatForever::create(Sequence::create(DelayTime::create(duration), CallFunc::create([=] {
		body->setVelocity(-body->getVelocity());
	}), nullptr)));
}



void ZVP::renderRewardBlock(const ValueMap &obj) {
	auto name = obj.at("name").asString();
	if (name == "Star") {
		auto star = createSprite("star");
		star->setPosition(obj.at("x").asFloat(), obj.at("y").asFloat());
		//让星星和阳光行为一致
		renderSunlightTile(star);
		star->setTag(tagStar);
		addChild(star, 2);
		return;
	}
	Sprite* questionBlock = createQuestionBlockAt(obj.at("x").asFloat(), obj.at("y").asFloat());
	questionBlock->setTag(tagLand | tagRewardBlock);
	questionBlock->setName(name);
	questionBlock->setUserData(new ValueMap(obj));
	addChild(questionBlock, 2);
}

void ZVP::renderSunlightTile(Sprite* tile) {
	auto moveUp = MoveBy::create(0.5f, Vec2(0, tile->getContentSize().height / 3));
	tile->runAction(RepeatForever::create(Sequence::create(moveUp, moveUp->reverse(), nullptr)));
	PhysicsBody* body = PhysicsBody::createBox(m_map->getTileSize(), PhysicsMaterial(1.0f, 0.0f, 0.0f));
	body->setDynamic(false);
	//阳光不能和任何物体发生碰撞。但是要有碰撞检测，以便玩家拾获。
	body->setContactTestBitmask(0xFFFFFFFF);
	body->setCollisionBitmask(0);
	tile->setPhysicsBody(body);
	tile->setTag(tagSunlight);
}


void  ZVP::renderZombieObject(const ValueMap &obj) {
	string name = obj.at("name").asString();
	auto zombie = createZombie(name);
	zombie->setPosition(obj.at("x").asFloat(), obj.at("y").asFloat());

	EnemyInfo *info = (EnemyInfo*)zombie->getUserData();
	auto behavior = obj.find("behavior");
	if (behavior != obj.end()) {
		info->beheavior = EnemyInfo::beheaviorOfString(behavior->second.asString());
	}


	auto hp = obj.find("hp");
	if (hp != obj.end()) {
		info->healthPoint =info->totalHealthPoint = hp->second.asInt();
	}
	Tools::assignIfContainsKey(obj, "attack", info->attack);
	Tools::assignIfContainsKey(obj, "velocity", info->velocity);
	Tools::assignIfContainsKey(obj, "velocity", info->primaryVelocity);
	Tools::assignIfContainsKey(obj, "attackDelay", info->attackDelay);

	addChild(zombie, 2);
	enemys.insert(zombie);

}


void ZVP::preloadMusic() {
	if (!isAudioEnabled) {
		return;
	}
	audio.preload("music/boxfall.mp3");
	audio.preload("music/breakBlock.wav");
	audio.preload("music/gameover.wav");
	audio.preload("music/Plant.mp3");
	audio.preload("music/GrowPlant.mp3");
	audio.preload("music/enemyAttacked.mp3");
	audio.preload("music/enemyDie.mp3");
	audio.preload("music/fire.mp3");
	audio.preload("music/getSomething.wav");
	audio.preload("music/gameover.wav");
	audio.preload("music/SnowPeaAttack.mp3");
	audio.preload("music/jump.mp3");
	audio.preload("music/plantAttacked.mp3");
	audio.preload("music/RepeaterAttack.mp3");
	audio.preload("music/PeaShooterAttack.mp3");
	audio.preload("music/upgrade.wav");
	audio.preload("music/frozen.mp3");
	audio.preload("music/Win.mp3");

	CocosDenshion::SimpleAudioEngine::getInstance()->preloadBackgroundMusic("music/bgm.mp3");
	CocosDenshion::SimpleAudioEngine::getInstance()->playBackgroundMusic("music/bgm.mp3", true);
}


void ZVP::addPlayer() {
	for (size_t i = 0; i < config.getPlayerCount(); i++) {
		PlayerInfo *info = new PlayerInfo(config.getPlayerInfo(i));
		string plantName = info->currentPlantName();
		loadIdle(plantName);
		auto player = Sprite::createWithSpriteFrame(SpriteFrameCache::getInstance()->getSpriteFrameByName(plantName));
		player->setTag(tagPlayer);
		player->setPosition(info->initialPosX, info->initialPosY);
		player->setUserData(info);
		setPlayerPlant(player, plantName);
		this->addChild(player, 2);
		players.push_back(player);
	}
}

void ZVP::setPlayerPlant(Sprite *player, const string &name) {
	player->stopActionByTag(tagAnimationPlayerWalking);
	player->stopActionByTag(tagAnimationPlayerAttack);
	((PlayerInfo*)player->getUserData())->isJumping = false;

	loadIdle(name);
	player->setSpriteFrame(SpriteFrameCache::getInstance()->getSpriteFrameByName(name));
	player->setName(name);

	auto body = PhysicsBody::createBox(player->getContentSize(), PhysicsMaterial(100.0f, 0.0f, 1.0f));
	body->setRotationEnable(false);
	body->setContactTestBitmask(0xFFFFFFFF);
	player->setPhysicsBody(body);
}

void  ZVP::loadIdleAnimationInfo() {
	//豌豆射手
	AnimationInfo peaShooterAnimInfo;
	peaShooterAnimInfo.setAnim("Walking", 6, Size(35, 38));
	peaShooterAnimInfo.setAnim("Attack", 6, Size(36, 38));
	idleAnimInfos["PeaShooter"] = peaShooterAnimInfo;

	//向日葵
	AnimationInfo sunFlowerAnimInfo;
	sunFlowerAnimInfo.setAnim("Walking", 10, Size(40, 40));
	idleAnimInfos["Sunflower"] = sunFlowerAnimInfo;

	//坚果
	AnimationInfo wallNutAnimInfo;
	wallNutAnimInfo.setAnim("Walking", 12, Size(35, 40));
	idleAnimInfos["WallNut"] = wallNutAnimInfo;

	//双发射手
	AnimationInfo repeaterAnimInfo;
	repeaterAnimInfo.setAnim("Walking", 12, Size(42, 42), 0.07f);
	idleAnimInfos["Repeater"] = repeaterAnimInfo;

	//寒冰射手
	AnimationInfo snowPeaAnimInfo;
	snowPeaAnimInfo.setAnim("Walking", 7, Size(40, 40));
	idleAnimInfos["SnowPea"] = snowPeaAnimInfo;

	//火爆辣椒
	AnimationInfo jalapenoAnim;
	jalapenoAnim.setAnim("Walking", 7, Size(30, 46));
	idleAnimInfos["Jalapeno"] = jalapenoAnim;

	//普通僵尸
	AnimationInfo zombie;
	zombie.setAnim("Walking", 11, Size(38, 50), 0.2f);
	idleAnimInfos["Zombie"] = zombie;

	// 气球僵尸
	AnimationInfo ballon;
	ballon.setAnim("Walking", 11, Size(43, 70));
	idleAnimInfos["Ballon"] = ballon;

	//巨人僵尸
	AnimationInfo gargantuar;
	gargantuar.setAnim("Walking", 8, Size(120, 120));
	idleAnimInfos["Gargantuar"] = gargantuar;

	//卷发僵尸
	AnimationInfo curlyHair;
	curlyHair.setAnim("Walking", 12, Size(43, 64));
	idleAnimInfos["CurlyHair"] = curlyHair;

	//宇航服僵尸
	AnimationInfo spaceman;
	spaceman.setAnim("Walking", 9, Size(71, 80));
	idleAnimInfos["Spaceman"] = spaceman;

	//小鬼僵尸
	AnimationInfo imp;
	imp.setAnim("Walking", 10, Size(21, 30));
	idleAnimInfos["Imp"] = imp;

	//潜水员僵尸
	AnimationInfo diver;
	diver.setAnim("Walking", 11, Size(39, 70));
	idleAnimInfos["Diver"] = diver;

	//雪人僵尸
	AnimationInfo snowman;
	snowman.setAnim("Walking", 11, Size(62, 100));
	idleAnimInfos["Snowman"] = snowman;

	//路障僵尸
	AnimationInfo conehead;
	conehead.setAnim("Walking", 11, Size(39, 64));
	idleAnimInfos["Conehead"] = conehead;
}

void ZVP::loadIdle(const string &name) {
	//如果角色已经被加载过，直接返回
	if (SpriteFrameCache::getInstance()->getSpriteFrameByName(name)) {
		return;
	}
	auto animInfo = idleAnimInfos[name];
	//把Walking动画的第一帧作为精灵的SpriteFrame。
	auto frameCount = animInfo.getAnimFrameCount("Walking");
	auto frameSize = animInfo.getAnimFrameSize("Walking");
	auto image = Director::getInstance()->getTextureCache()->addImage(name + "Walking.png");
	auto idleFrame = SpriteFrame::createWithTexture(image, CC_RECT_PIXELS_TO_POINTS(Rect(0, 0, frameSize.width, frameSize.height)));
	SpriteFrameCache::getInstance()->addSpriteFrame(idleFrame, name);

	//加载各个状态的动画。行走、攻击动画等。
	string status[2] = { string("Walking"), string("Attack") };
	for (int i = 0; i < 2; i++) {
		frameCount = animInfo.getAnimFrameCount(status[i]);
		frameSize = animInfo.getAnimFrameSize(status[i]);
		auto delay = animInfo.getAnimDelay(status[i]);
		if (abs(delay) < 1e-3f) {
			delay = 0.1f;
		}
		loadAnimation(name + status[i], frameCount, frameSize, delay);
	}
}

void ZVP::loadAnimation(const string &name, size_t frameCount, const Size &frameSize, float interval) {
	if (AnimationCache::getInstance()->getAnimation(name) != nullptr) {
		return;
	}
	Vector<SpriteFrame*> frames;
	auto image = Director::getInstance()->getTextureCache()->addImage(name + ".png");
	for (size_t j = 0; j < frameCount; j++) {
		frames.pushBack((SpriteFrame::createWithTexture(image, CC_RECT_PIXELS_TO_POINTS(Rect(frameSize.width * j, 0, frameSize.width, frameSize.height)))));
	}
	AnimationCache::getInstance()->addAnimation(Animation::createWithSpriteFrames(frames, interval), name);
}

Sprite *ZVP::createSprite(const string &name, float w, float h) {
	auto frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(name);
	if (frame == nullptr) {
		auto image = Director::getInstance()->getTextureCache()->addImage(name + ".png");
		// FIXME  image->getPixelsWide()、 image->getPixelsHigh() 以及image->getContentSize()、getContentSizeInPixels获取到的宽高总是比图片的真实宽高小一点。
		//				这里乘了1.15，只是不好的解决方法。
		if (w < 0) {
			w = image->getPixelsWide() * 1.15f;
		}
		if (h < 0) {
			h = image->getPixelsHigh() * 1.15f;
		}
		frame = SpriteFrame::createWithTexture(image, Rect(0, 0, w, h));
		SpriteFrameCache::getInstance()->addSpriteFrame(frame, name);
	}
	return Sprite::createWithSpriteFrame(frame);
}

Sprite *ZVP::createPlantCard(const string &name) {
	auto card = createSprite("Card");
	loadIdle(name);
	auto plant = createSprite(name);
	plant->setPosition(card->getContentSize() / 2);
	card->addChild(plant);
	return card;
}

void ZVP::createPlantCardAbove(const string &name, const Vec2 &pos) {
	auto card = createPlantCard(name);
	card->setTag(tagPlantCard);
	card->setName(name);
	card->setRotation(15);
	card->setScale(0.1f);
	card->setPosition(pos.x, pos.y + 40);

	auto body = PhysicsBody::createBox(card->getContentSize(), PHYSICSSHAPE_MATERIAL_DEFAULT);
	body->setContactTestBitmask(0xFFFFFFFF);
	body->setGravityEnable(false);
	card->setPhysicsBody(body);


	auto moveUp = MoveBy::create(2.0f, Vec2(32, 16));
	card->runAction(RepeatForever::create(Sequence::create(moveUp, moveUp->reverse(), nullptr)));
	card->runAction(ScaleTo::create(1.0f, 0.5f));
	if (isAudioEnabled) {
		card->runAction(CallFunc::create([=] {
			audio.play2d("music/boxfall.mp3", false);
		}));
	}
	

	addChild(card, 2);
}

Sprite *ZVP::createQuestionBlockAt(float x, float y) {
	Sprite* questionBlock = createSprite("question_mark", 37.5f, 37.5f);

	PhysicsBody* body = PhysicsBody::createBox(questionBlock->getContentSize(), PhysicsMaterial(1.0f, 0.0f, 0.0f));
	body->setDynamic(false);
	body->setContactTestBitmask(0xFFFFFFFF);
	questionBlock->setPhysicsBody(body);

	auto tilePos = getTileCoordFromPosition(Vec2(x, y));
	questionBlock->setPosition((tilePos.x + 0.5f) * 37.5f, (m_map->getMapSize().height - tilePos.y - 0.5f) * 37.5f);
	return questionBlock;
}

Sprite *ZVP::createGrasslandBlock() {
	Sprite* grasslandBlock = createSprite("grassland", 37.5f, 37.5f);
	
	renderLandTile(grasslandBlock);

	return grasslandBlock;
}

Sprite *ZVP::createZombie(const string &name) {
	loadIdle(name);
	auto zombie = Sprite::createWithSpriteFrame(SpriteFrameCache::getInstance()->getSpriteFrameByName(name));
	zombie->setTag(tagEnemy);
	zombie->setUserData(config.getEnemeyInfoFactory().createEnemyInfo(name));

	PhysicsBody* body = PhysicsBody::createBox(zombie->getContentSize(), PhysicsMaterial(4.0f, 0.0f, 0.0f));
	// 两个僵尸不能碰撞
	body->setCategoryBitmask(0xF0F0F0F0);
	body->setCollisionBitmask(0x0F0F0F0F);
	body->setContactTestBitmask(0xFFFFFFFF);
	body->setRotationEnable(false);
	zombie->setPhysicsBody(body);

	//使用hp条设置progressBar
	cocos2d::ProgressTimer* hp = ProgressTimer::create(createSprite("hp"));
	hp->setName("hp");
	hp->setScale(zombie->getContentSize().width / hp->getContentSize().width, 0.2f);
	hp->setAnchorPoint(Point(0, 0.5));
	hp->setType(ProgressTimerType::BAR);
	hp->setBarChangeRate(Point(1, 0));
	hp->setMidpoint(Point(0, 1));
	hp->setPosition(0, zombie->getContentSize().height + 10);
	hp->setVisible(false);

	zombie->addChild(hp);

	zombie->runAction(RepeatForever::create(Animate::create(AnimationCache::getInstance()->getAnimation(name + "Walking"))));
	return zombie;
}

void ZVP::addPlant(Sprite *player, const string &plantName, PlantInfo plantInfo, Sprite* card) {
	auto playerInfo = (PlayerInfo*)player->getUserData();
	if (playerInfo->hasPlant(plantName)) {
		return;
	}

	auto w = 62;
	auto h = 90;
	auto pos = Vec2( visibleSize.width / players.size() *  playerInfo->playerIndex +  playerInfo->plantCount() * (w + 4) + w / 2 + 4, visibleSize.height - h / 2 - 4);

	if (card == nullptr) {
		card = createPlantCard(plantName);
		card->setPosition(pos);
		infoLayer->addChild(card);
		playerInfo->addPlant(plantName, plantInfo);
		appendPlantCardInfo(card, &playerInfo->plantInfo[plantName]);
		playerInfo->infoViews.plantCards.push_back(card);
	}
	else {
		removeChild(card, false);
		infoLayer->addChild(card);
		card->setPosition(card->getPosition() + getPosition());
		card->getPhysicsBody()->setContactTestBitmask(0);
		card->getPhysicsBody()->setDynamic(false);
		card->stopAllActions();
		card->runAction(ScaleTo::create(0.7f, 1.0f));
		card->runAction(RotateTo::create(0.7f, 0));
		card->runAction(Sequence::create(MoveTo::create(0.7f, pos), CallFunc::create([=] {
			appendPlantCardInfo(card, &playerInfo->plantInfo[plantName]);
			playerInfo->infoViews.plantCards.push_back(card);
		}), nullptr));
		playerInfo->addPlant(plantName, plantInfo);
	}

}

void ZVP::appendPlantCardInfo(Sprite* card,  PlantInfo *plantInfo) {
	auto sunlightLabel = Label::createWithTTF(TTF_PLANT_SUNLIGHT_LABEL, to_string(plantInfo->requiredSunlight));
	sunlightLabel->setPosition(card ->getContentSize().width / 2 ,  13);
	sunlightLabel->setColor(COLOR_PLANT_SUNLIGHT_LABEL);
	card->addChild(sunlightLabel);

	auto hpLabel = Label::createWithTTF(TTF_PLANT_HP_LABEL, "");
	hpLabel->setName("hp");
	card->addChild(hpLabel);

	auto shadow = Sprite::create();
	shadow->setColor(Color3B::BLACK);
	shadow->setOpacity(180);
	shadow->setTextureRect(card->getTextureRect());
	shadow->setContentSize(Size(card->getContentSize().width - 8, 0));
	shadow->setAnchorPoint(Point(0, 0));
	shadow->setPosition(3.9f, 0);
	shadow->setName("shadow");
	card->addChild(shadow);
}

void ZVP::makeSpriteMoveBackAndForth(Sprite *sprite, float distance, float duration) {
	auto move = MoveBy::create(duration, Vec2(distance, 0));
	auto action = RepeatForever::create(Sequence::create(move, CallFunc::create([=] {
		sprite->setFlippedX(!sprite->isFlippedX());
	}), move->reverse(), CallFunc::create([=] {
		sprite->setFlippedX(!sprite->isFlippedX());
	}), nullptr));
	action->setTag(tagActionMoveBackAndForth);
	sprite->runAction(action);
}


void ZVP::addListeners() {
	auto keyboardListener = EventListenerKeyboard::create();
	keyboardListener->onKeyPressed = CC_CALLBACK_2(ZVP::onKeyPressed, this);
	keyboardListener->onKeyReleased = CC_CALLBACK_2(ZVP::onKeyReleased, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

	auto contactListener = EventListenerPhysicsContact::create();
	contactListener->onContactBegin = CC_CALLBACK_1(ZVP::onConcactBegin, this);
	contactListener->onContactSeparate = CC_CALLBACK_1(ZVP::onContactSeparate, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(contactListener, this);
}

void ZVP::onKeyPressed(EventKeyboard::KeyCode code, Event * event) {
	auto player = 0;
	for (size_t i = 1; i < players.size(); i++) {
		auto keyCodeMapper = config.getKeyCodeMapper(i);
		auto keyCode = keyCodeMapper.find(code);
		if (keyCode != keyCodeMapper.end()) {
			player = i;
			code = keyCode->second;
			break;
		}
	}
	switch (code) {
	case KeyCode::KEY_ESCAPE:
	case KeyCode::KEY_ENTER:
	case KeyCode::KEY_KP_ENTER:
		pauseAndShowMenu();
		break;
	case cocos2d::EventKeyboard::KeyCode::KEY_SPACE:
		saveGame();
		break;
	default:
		onPlayerKeyPressed(player, code, event);
	}
}

void ZVP::pauseAndShowMenu() {
	if (infoLayer->getChildByName("shadow")) {
		return;
	}
	pauseGame();
	
	auto shadow = ButtonMenuBuilder::createShadow(visibleSize, visibleSize / 2);
	shadow->setName("shadow");
	infoLayer->addChild(shadow);

	auto dismissMenu = [=](Ref *ref) {
		(dynamic_cast<Button*>(ref))->getParent()->removeFromParent();
		infoLayer->getChildByName("shadow")->removeFromParent();
		Director::getInstance()->resume();
	};

	auto menu = ButtonMenuBuilder()
		.font("fonts/ChronicGothic.ttf")
		.fontSize(46)
		.margin(10)
		.addItem("RESUME", [=](Ref* ref) {
			dismissMenu(ref);
			resumeGame();
		})
		.addItem("SAVE", [=](Ref* ref) {
			dismissMenu(ref);
			saveGame();
			resumeGame();
		})
		.addItem("SAVE AND BACK", [=](Ref* ref) {
			saveGame();
			resumeGame();
			Director::getInstance()->replaceScene(StartScene::createScene());
		})
		.addItem("BACK", [=](Ref* ref) {
			resumeGame();
			Director::getInstance()->replaceScene(StartScene::createScene());
		})
		.build();
	menu->setPosition(visibleSize / 2);
	infoLayer->addChild(menu);
}

void ZVP::onMenuTouchEvent(Ref* ref, Widget::TouchEventType type) {
	auto shadow = infoLayer->getChildByName("shadow");
	if (shadow) {
		shadow->removeFromParent();
	}
	auto button = dynamic_cast<Button*>(ref);
	string title = button->getTitleText();
	button->getParent()->removeFromParent();
	if (title == "SAVE" || title == "EXIT & SAVE") {
		saveGame();
	}
	if (title == "EXIT" || title == "EXIT & SAVE") {
		
	}
}

void ZVP::onPlayerKeyPressed(int i, EventKeyboard::KeyCode code, Event * event) {
	if (players.size() <= i) {
		return;
	}
	string plants[] = { "WallNut", "Repeater", "Jalapeno", "SnowPea", "PeaShooter" };
	auto player = players[i];
	auto playerInfo = (PlayerInfo*)player->getUserData();
	auto &plantInfo = playerInfo->plantInfo[player->getName()];
	switch (code) {
	case KeyCode::KEY_A:
	case KeyCode::KEY_CAPITAL_A:
	case KeyCode::KEY_D:
	case KeyCode::KEY_CAPITAL_D:
	case KeyCode::KEY_W:
	case KeyCode::KEY_CAPITAL_W:
			movePlayer(player, code);
		break;
	case KeyCode::KEY_J: {
		if (Tools::currentTimeInMillis() - playerInfo->lastAttackingTime < plantInfo.attackDelay) {
			break;
		}
		auto attackAction = RepeatForever::create(Sequence::create(CallFunc::create([=] {
			makeAttack(player);
		}), DelayTime::create(plantInfo.repeatedAttackDelay), nullptr));
		attackAction->setTag(tagActionPlayerAttacking);
		player->runAction(attackAction);
		break;
	}
	case KeyCode::KEY_K: {
		//切换植物
		if (playerInfo->plantCount() <= 1) {
			break;
		}
		if (isAudioEnabled) {
			audio.play2d("music/Plant.mp3", false);
		}
		playerInfo->switchPlant();
		setPlayerPlant(player, playerInfo->currentPlantName());
		break;
	}
	case KeyCode::KEY_L:
		//复活植物
		if (plantInfo.healthPoint > 0 || plantInfo.isFrozen || playerInfo->sunlight < plantInfo.requiredSunlight) {
			break;
		}
		if (isAudioEnabled) {
			audio.play2d("music/GrowPlant.mp3", false);
		}
		playerInfo->sunlight -= plantInfo.requiredSunlight;
		plantInfo.healthPoint = plantInfo.totalHealthPoint;
		break;
	case KeyCode::KEY_S:
		if (GameService::allowCheat) {
			//作弊。生成卡片。
			createPlantCardAbove(plants[random(0, 4)], player->getPosition());
		}
		break;
	case KeyCode::KEY_U: {
		if (!GameService::allowCheat) {
			return;
		}
		//作弊
		playerInfo->sunlight += 10000;
		for (int i = 0; i < 5; i++) {
			addPlant(player, plants[i], PlantInfo::createWithFullHp(plants[i]));
		}
		plantInfo.healthPoint = plantInfo.totalHealthPoint;
		break;
	}
	case KeyCode::KEY_I: {
		//测试：生成一堆僵尸
		string zombies[] = { "Zombie", "Ballon", "Gargantuar", "Snowman", "Spaceman", "CurlyHair", "Imp", "Diver", "Tie", "Conehead" };
		for (int i = 0; i < 10; i++) {
			auto zombie = createZombie(zombies[i]);
			zombie->setPosition(player->getPosition() + Vec2(i * 32, 0));
			enemys.insert(zombie);
			addChild(zombie, 2);
		}
		break;
	}
	case KeyCode::KEY_O: {
		//作弊: 加血
		plantInfo.healthPoint++;
		break;
	}
	}
}

void ZVP::onKeyReleased(EventKeyboard::KeyCode code, Event * event) {
	auto player = 0;
	for (size_t i = 1; i < players.size(); i++) {
		auto keyCodeMapper = config.getKeyCodeMapper(i);
		auto keyCode = keyCodeMapper.find(code);
		if (keyCode != keyCodeMapper.end()) {
			player = i;
			code = keyCode->second;
			break;
		}
	}
	switch (code) {
	case KeyCode::KEY_A:
	case KeyCode::KEY_CAPITAL_A:
	case KeyCode::KEY_D:
	case KeyCode::KEY_CAPITAL_D:
	case KeyCode::KEY_W:
	case KeyCode::KEY_CAPITAL_W:
		if (players.size() > player) {
			stopPlayer(players[player], code);
		}
		break;
	case KeyCode::KEY_J:
		if (players.size() > player) {
			players[player]->stopActionByTag(tagActionPlayerAttacking);
		}
		break;
	}
}

void ZVP::movePlayer(Sprite* player, EventKeyboard::KeyCode code) {
	auto body = player->getPhysicsBody();
	auto velocity = body->getVelocity();
	auto playerInfo = (PlayerInfo*)player->getUserData();
	auto &plantInfo = playerInfo->currentPlantInfo();
	switch (code) {
	case cocos2d::EventKeyboard::KeyCode::KEY_A:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_A:
		playerInfo->velocityX = plantInfo.healthPoint > 0 ? -plantInfo.velocity : -30;
		body->setVelocity(Vec2(playerInfo->velocityX, velocity.y));
		player->setFlippedX(true);
		break;
	case cocos2d::EventKeyboard::KeyCode::KEY_D:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_D:
		playerInfo->velocityX = plantInfo.healthPoint > 0 ? plantInfo.velocity : 30;
		body->setVelocity(Vec2(playerInfo->velocityX, velocity.y));
		player->setFlippedX(false);
		break;
	case cocos2d::EventKeyboard::KeyCode::KEY_W:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_W:
		if (playerInfo->isJumping) {
			break;
		}
		playerInfo->isJumping = true;
		body->setVelocity(Vec2(velocity.x, plantInfo.healthPoint > 0 ? plantInfo.jumpingVelocity : 100));
		if (isAudioEnabled) {
			player->runAction(CallFunc::create([=] {
				audio.play2d("music/jump.mp3", false);
			}));
		}
		
		break;
	}
}

void ZVP::stopPlayer(Sprite* player, EventKeyboard::KeyCode code) {
	auto body = player->getPhysicsBody();
	auto playerInfo = (PlayerInfo*)player->getUserData();
	switch (code) {
	case cocos2d::EventKeyboard::KeyCode::KEY_A:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_A:
	case cocos2d::EventKeyboard::KeyCode::KEY_D:
	case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_D:
		playerInfo->velocityX = 0;
		break;
	}
}


void ZVP::makeAttack(Sprite *player) {
	PlayerInfo* playerInfo = (PlayerInfo*)player->getUserData();
	auto &plantInfo = playerInfo->plantInfo[player->getName()];
	if (plantInfo.healthPoint <= 0) {
		return;
	}
	if (player->getName() == "PeaShooter" || player->getName() == "Repeater" || player->getName() == "SnowPea") {
		shootPea(player);
		playerInfo->lastAttackingTime = Tools::currentTimeInMillis();
		return;
	}
	if (player->getName() == "Jalapeno") {
		boom(player);
		return;
	}
}

void ZVP::shootPea(Sprite *player) {
	string peaName = "Pea";
	float peaVolecity = 800, peaOffsetY = 14;
	if (player->getName() == "SnowPea") {
		peaName = "FrozenPea";
		peaOffsetY = 18;
	}
	else if (player->getName() == "Repeater") {
		peaOffsetY = 13;
	}
	auto shootPea = [=]()->void {
		auto pea = createSprite(peaName);
		pea->setName(peaName);
		pea->setTag(tagPea);
		auto body = PhysicsBody::createBox(pea->getContentSize(), PHYSICSSHAPE_MATERIAL_DEFAULT);
		body->setContactTestBitmask(0xFFFFFFFF);
		body->setGravityEnable(false);
		pea->setPhysicsBody(body);
		if (player->isFlippedX()) {
			body->setVelocity(Vec2(-800, 0));
			pea->setPosition(player->getPosition() + Vec2(-player->getContentSize().width / 2 + 9, player->getContentSize().height / 2 - peaOffsetY));
		}
		else {
			body->setVelocity(Vec2(800, 0));
			pea->setPosition(player->getPosition() + Vec2(player->getContentSize().width / 2 - 9, player->getContentSize().height / 2 - peaOffsetY));
		}
		addChild(pea, 2);
		if (isAudioEnabled) {
			pea->runAction(CallFunc::create([=] {
				audio.play2d(("music/" + player->getName() + "Attack.mp3").c_str(), false);
			}));
		}
		
	};
	if (player->getName() == "Repeater") {
		player->runAction(Sequence::create(CallFunc::create(shootPea), DelayTime::create(0.02f), CallFunc::create(shootPea), nullptr));
	}
	else if (player->getName() == "SnowPea") {
		shootPea();
	}
	else {
		player->stopActionByTag(tagAnimationPlayerAttack);
		auto seq = Sequence::create(Animate::create(AnimationCache::getInstance()->getAnimation(player->getName() + "Attack")), CallFunc::create(shootPea), nullptr);
		seq->setTag(tagAnimationPlayerAttack);
		player->runAction(seq);
	}
}

void ZVP::boom(Sprite *player) {
	loadAnimation("Fire", 14, Size(26, 20), 0.25f);
	player->setScale(0.7f);
	player->runAction(Sequence::create(ScaleTo::create(2.0f, 1.3f), CallFunc::create([=]()->void {
		auto playerInfo = (PlayerInfo*)player->getUserData();
		playerInfo->currentPlantInfo().die();
		setPlayerPlant(player, playerInfo->switchPlant());
		player->setScale(1);
		auto fireAnim = AnimationCache::getInstance()->getAnimation("Fire");
		for (int i = -5; i <= 5; i++) {
			Sprite* fire = Sprite::create();
			fire->setTag(tagFire);
			auto body = PhysicsBody::createBox(Size(26, 20));
			body->setDynamic(false);
			body->setCollisionBitmask(0);
			body->setContactTestBitmask(0xFFFFFFFF);
			fire->setPhysicsBody(body);
			fire->setPosition(player->getPosition().x + i * 32, player->getPosition().y - 20);
			addChild(fire, 3);
			fire->runAction(Sequence::create(Animate::create(fireAnim), CallFunc::create([fire]()->void {
				fire->removeFromParent();
			}), nullptr));
			if (isAudioEnabled) {
				audio.play2d("music/fire.mp3", false);
			}
		}

	}), nullptr));


}

void  ZVP::killEnemy(Sprite *enemy) {
	delete enemy->getUserData();
	enemys.erase(enemy);
	enemy->removeFromParent();
	if (isAudioEnabled) {
		runAction(CallFunc::create([] {
			//audio.play2d("music/enemyDie.mp3", false);
		}));
	}
	
}

bool ZVP::onConcactBegin(PhysicsContact & contact) {
	auto sprite1 = contact.getShapeA()->getBody()->getOwner();
	auto sprite2 = contact.getShapeB()->getBody()->getOwner();
	if (sprite1 == nullptr || sprite2 == nullptr) {
		return true;
	}
	if (sprite1->getTag() & tagPlayer || sprite2->getTag() & tagPlayer) {
		auto player = sprite1->getTag() & tagPlayer ? sprite1 : sprite2;
		auto anotherSprite = sprite1->getTag() & tagPlayer ? sprite2 : sprite1;
		return handlePlayerCollision((Sprite*) player, anotherSprite);
	}
	if (sprite1->getTag() == tagPea || sprite2->getTag() == tagPea) {
		auto pea = sprite1->getTag() == tagPea ? sprite1 : sprite2;
		auto anotherSprite = sprite1->getTag() == tagPea ? sprite2 : sprite1;
		if ((anotherSprite->getTag() & tagEnemy) != 0) {
			handlePeaCollisionWithEnemy((Sprite*)pea, (Sprite*)anotherSprite);
		}
		else {
			pea->removeFromParent();
		}
		return true;
	}
	if (sprite1->getTag() == tagFire || sprite2->getTag() == tagFire) {
		auto anotherSprite = sprite1->getTag() == tagFire ? sprite2 : sprite1;
		if ((anotherSprite->getTag() & tagEnemy) != 0) {
			killEnemy((Sprite*)anotherSprite);
		}
		return true;
	}
	
	/*
	if (sprite1->getTag() & tagEnemy || sprite2->getTag() & tagEnemy) {
		auto anotherSprite = sprite1->getTag() & tagEnemy ? sprite2 : sprite1;
		auto enemy = (Sprite*) (sprite1->getTag() & tagEnemy ? sprite1 : sprite2);
		auto enemyInfo = (EnemyInfo*)enemy->getUserData();
		if (enemyInfo->beheavior != EnemyBehavior::REPEAT || anotherSprite->getTag() != tagFixedBlock | tagLand) {
			return true;
		}
		auto enemyBoundingBox = enemy->getBoundingBox();
		auto anotherBoundingBox = anotherSprite->getBoundingBox();
		if (anotherBoundingBox.getMinY() < enemyBoundingBox.getMaxY() || anotherBoundingBox.getMaxY() > enemyBoundingBox.getMinY()) {
			if ((enemy->isFlippedX() && enemyBoundingBox.getMaxX() < anotherBoundingBox.getMinX()) || 
				(!enemy->isFlippedX() && enemyBoundingBox.getMinX() > anotherBoundingBox.getMaxX())) {
				enemy->setFlippedX(!enemy->isFlippedX());
			}
		}
	}*/
	return true;
}

bool ZVP::handlePlayerCollision(Sprite *player, Node *anotherSprite) {
	//如果玩家和精灵碰撞时是"踩着"他的，则玩家着陆，isPlayerJumping设为false
	if (player->getPositionY() > anotherSprite->getPositionY() + player->getContentSize().height / 2) {
		((PlayerInfo*)player->getUserData())->isJumping = false;
	}
	auto tag = anotherSprite->getTag();
	if (tag & tagLand) {
		handlePlayerCollisionWithLand(player, (Sprite*)anotherSprite);
	}
	else if (tag & tagSunlight) {
		handlePlayerCollisionWithSublight(player, (Sprite*)anotherSprite);
	}
	else if (tag & tagEnemy) {
		handlePlayerCollisionWithEnemy(player, (Sprite*)anotherSprite);
		EnemyInfo *enemyInfo = (EnemyInfo*)anotherSprite->getUserData();
		//如果僵尸一直粘着玩家，则每隔一段时间就攻击他一次
		auto attackAction = RepeatForever::create(Sequence::create(DelayTime::create(enemyInfo->attackDelay), CallFunc::create([=] {
			handlePlayerCollisionWithEnemy(player, (Sprite*)anotherSprite);
		}), nullptr));
		attackAction->setTag(tagActionZombieAttacking);
		//这个攻击Action需要在玩家和僵尸分开时停止
		anotherSprite->runAction(attackAction);
	}
	else if (tag & tagPlantCard) {
		handlePlayerCollisionWithPlantCard(player, (Sprite*)anotherSprite);
		return false;
	}
	else if (tag & tagStar) {
		win((Sprite*)anotherSprite);
		return false;
	}
	return true;
}

void ZVP::handlePlayerCollisionWithLand(Sprite* player, Sprite* block) {
	auto playerBoundingBox = player->getBoundingBox();
	auto blockBoundingBox = block->getBoundingBox();
	auto tag = block->getTag();
	//如果玩家y小于精灵，则玩家是跳起来顶到方块的
	if (playerBoundingBox.getMaxY() <= blockBoundingBox.getMinY() + 5) {
		//如果是固定的方块，则不作处理
		if ((tag & tagFixedBlock) != 0) {
			return;
		}
		//如果是坚果，则把方块打碎
		if (player->getName() == "WallNut") {
			runAction(CallFunc::create([=]()->void {
				breakBlock(block);
				block->removeFromParent();
			}));
			return;
		}
		//否则方块会被顶上去然后"弹回来"
		auto moveUp = MoveBy::create(0.2f, Vec2(0, block->getContentSize().height / 3));
		//如果是奖励方块（问号方块），则处理奖励
		if (tag & tagRewardBlock) {
			handleRewardBlockCollided(player, block);
		}
		block->runAction(Sequence::create(moveUp, moveUp->reverse(), NULL));
	}
}

void ZVP::handleRewardBlockCollided(Sprite* player, Sprite *block) {
	if (block->getName() == "Sunlight") {
		ValueMap *m = (ValueMap*)block->getUserData();
		auto sunlightCount = m->at("sunlight").asInt();
		if (sunlightCount <= 0) {
			return;
		}
		auto sunlightIcon = ((PlayerInfo*)player->getUserData())->infoViews.sunlightIcon;
		Sprite* sunlight = createSprite("sunlight");
		infoLayer->addChild(sunlight);
		sunlight->setPosition(block->getPosition() + getPosition() + Vec2(0, 16));
		sunlight->runAction(Sequence::create(MoveBy::create(0.2f, Vec2(0, 48)), MoveTo::create(0.5f, sunlightIcon->getPosition() - sunlightIcon->getContentSize()), CallFunc::create([=] {
			PlayerInfo* playerInfo = (PlayerInfo*)player->getUserData();
			playerInfo->sunlight += 10;
			sunlight->removeFromParent();
		}), nullptr));
		if (isAudioEnabled) {
			audio.play2d("music/getSomething.mp3", false);
		}
		(*m)["sunlight"] = sunlightCount - 1;
		if (sunlightCount > 1) {
			return;
		}
	}
	else {
		runAction(CallFunc::create(CC_CALLBACK_0(ZVP::createPlantCardAbove, this, block->getName(), block->getPosition())));
	}
	auto emptyQuestionBlock = SpriteFrameCache::getInstance()->getSpriteFrameByName("EmptyQuestionBlock");
	if (emptyQuestionBlock == nullptr) {
		emptyQuestionBlock = SpriteFrame::createWithTexture(Director::getInstance()->getTextureCache()->addImage("question_block_no_reward.png"), CC_RECT_PIXELS_TO_POINTS(Rect(0, 0, 32, 32)));
		SpriteFrameCache::getInstance()->addSpriteFrame(emptyQuestionBlock, "EmptyQuestionBlock");
	}
	block->setSpriteFrame(emptyQuestionBlock);
	block->setTag(tagLand | tagFixedBlock);
}

void ZVP::handlePlayerCollisionWithSublight(Sprite* player, Sprite* sunlight) {
	sunlight->removeFromParentAndCleanup(false);
	infoLayer->addChild(sunlight);
	sunlight->setPosition(sunlight->getPosition() + getPosition());
	auto sunlightIcon = ((PlayerInfo*)player->getUserData())->infoViews.sunlightIcon;
	sunlight->runAction(Sequence::create(MoveTo::create(0.8f, sunlightIcon->getPosition() - sunlightIcon->getContentSize()), CallFunc::create([=] {
		PlayerInfo* playerInfo = (PlayerInfo*)player->getUserData();
		playerInfo->sunlight += 10;
		sunlight->removeFromParent();
	}), nullptr));
	if (isAudioEnabled) {
		audio.play2d("music/getSomething.mp3", false);
	}
}

void ZVP::handlePlayerCollisionWithEnemy(Sprite* player, Sprite* enemy) {
	//如果还是闪烁状态，不受攻击，直接返回
	if (player->getActionByTag(tagAnimationPlayerAttacked) != nullptr) {
		return;
	}
	auto anim = Blink::create(2.0f, 6);
	anim->setTag(tagAnimationPlayerAttacked);
	player->runAction(anim);
	if (isAudioEnabled) {
		player->runAction(CallFunc::create([=] {
			audio.play2d("music/plantAttacked.mp3", false);
		}));
	}
	PlayerInfo* playerInfo = (PlayerInfo*)player->getUserData();
	auto &plantInfo = playerInfo->currentPlantInfo();
	EnemyInfo *enemyInfo = (EnemyInfo*)enemy->getUserData();
	if (plantInfo.healthPoint <= 0) {
		//如果植物没血了就减少阳光
		playerInfo->sunlight = MAX(0, playerInfo->sunlight - enemyInfo->attack * 10);
		//没阳光了就游戏结束
		if (playerInfo->sunlight <= 0) {
			gameOver();
		}
		return;
	}
	plantInfo.healthPoint = MAX(0, plantInfo.healthPoint - enemyInfo->attack);
	CCLOG("HP: %d", plantInfo.healthPoint);
	if (plantInfo.healthPoint <= 0) {
		plantInfo.die();
	}
}


void ZVP::handlePlayerCollisionWithPlantCard(Sprite* player, Sprite* card) {
	runAction(CallFunc::create([=]()->void {
		if (isAudioEnabled) {
			audio.play2d("music/getSomething.wav", false);
		}
		auto playerInfo = (PlayerInfo*)player->getUserData();
		//如果已经有这张卡片了，就增加100阳光
		if (playerInfo->hasPlant(card->getName())) {
			card->removeFromParent();
			playerInfo->sunlight += 100;
		}
		else {
			card->getPhysicsBody()->setCategoryBitmask(0);
			card->getPhysicsBody()->setContactTestBitmask(0);
			card->getPhysicsBody()->setCollisionBitmask(0);
			addPlant(player, card->getName(), PlantInfo::create(card->getName()), card);
		}
	}));
}

void ZVP::handlePeaCollisionWithEnemy(Sprite *pea, Sprite* enemy) {
	bool enemyDead = onEnemyAttacked(enemy, 1);
	if (enemyDead) {
		pea->removeFromParent();
		return;
	}
	enemy->runAction(MoveBy::create(0.3f, Vec2(pea->getPhysicsBody()->getVelocity().x > 0 ? 16 : -16, 0)));
	EnemyInfo *info = (EnemyInfo*)enemy->getUserData();
	if (pea->getName() == "FrozenPea") {
		if (isAudioEnabled) {
			enemy->runAction(CallFunc::create([=] {
				audio.play2d("music/frozen.mp3", false);
			}));
		}
		enemy->setColor(COLOR_SPRITE_FROZEN);
		info->frozenTimeStamp = Tools::currentTimeInMillis();
		info->frozenDuration = MAX(info->frozenDuration, 10000);
		info->velocity = info->primaryVelocity / 3;
	}
	pea->removeFromParent();
}

bool ZVP::onEnemyAttacked(Sprite* enemy, int attack) {
	EnemyInfo *info = (EnemyInfo*)enemy->getUserData();
	info->healthPoint -= attack;
	if (info->healthPoint <= 0) {
		killEnemy(enemy);
		return true;
	}
	if (isAudioEnabled) {
		runAction(CallFunc::create([=] {
			audio.play2d("music/enemyAttacked.mp3", false);
		}));
	}
	enemy->stopActionByTag(tagActionEnemyDisplayingHp);
	enemy->getChildByName("hp")->setVisible(true);
	auto hideHp = Sequence::createWithTwoActions(DelayTime::create(3.0f), CallFunc::create([=] {
		enemy->getChildByName("hp")->setVisible(false);
	}));
	hideHp->setTag(tagActionEnemyDisplayingHp);
	enemy->runAction(hideHp);
	return false;
}

void ZVP::onContactSeparate(PhysicsContact & contact) {
	auto sprite1 = contact.getShapeA()->getBody()->getOwner();
	auto sprite2 = contact.getShapeB()->getBody()->getOwner();
	if (sprite1 == nullptr || sprite2 == nullptr) {
		return;
	}
	if (sprite1->getTag() & tagPlayer || sprite2->getTag() & tagPlayer) {
		auto player = sprite1->getTag() & tagPlayer ? sprite1 : sprite2;
		auto anotherSprite = sprite1->getTag() & tagPlayer ? sprite2 : sprite1;
		if (anotherSprite->getTag() & tagEnemy) {
			//停止攻击的行动
			((Sprite*)anotherSprite)->stopActionByTag(tagActionZombieAttacking);
		}
	}
}

void  ZVP::breakBlock(Sprite *block) {
	//四个碎片的区域
	Rect rects[4] = { Rect(0, 0, 16, 16), Rect(16, 0, 16, 16) , Rect(0, 16, 16, 16) , Rect(16, 16, 16, 16) };
	//四个碎片的位置
	Vec2 postionOffsets[4] = { Vec2(10.5, 25.5) , Vec2(26.5, 25.5), Vec2(10.5, 9.5) ,Vec2(26.5, 9.5) };
	//四个碎片的初始速度
	Vec2 velocities[4] = { Vec2(-50, 100) , Vec2(50, 100) , Vec2(-50, 30) , Vec2(50, 30) };
	for (int i = 0; i < 4; i++) {
		auto frame = SpriteFrameCache::getInstance()->getSpriteFrameByName("Brick_" + i);
		if (frame == nullptr) {
			auto brickImage = Director::getInstance()->getTextureCache()->addImage("grassland.png");
			frame = SpriteFrame::createWithTexture(brickImage, CC_RECT_PIXELS_TO_POINTS(rects[i]));
			SpriteFrameCache::getInstance()->addSpriteFrame(frame, "Brick_" + i);
		}

		auto brick = Sprite::createWithSpriteFrame(frame);
		auto body = PhysicsBody::createBox(brick->getContentSize(), PhysicsMaterial(1.0f, 0.0f, 1.0f));
		body->setCategoryBitmask(0);
		body->setVelocity(velocities[i]);
		brick->setPosition(block->getPosition() + postionOffsets[i]);
		body->applyForce(Vec2(0, -100000));
		brick->setPhysicsBody(body);

		brick->runAction(RepeatForever::create(RotateBy::create(0.3f, i % 2 == 0 ? 360 : -360)));

		addChild(brick, 2);
		if (isAudioEnabled) {
			audio.play2d("music/breakBlock.mp3", false);
		}
	}
}

void ZVP::update(float dt) {
	for (size_t i = 0; i < players.size(); i++) {
		updatePlayer(players[i]);
	}
	if (players.size() > 0) {
		setViewPoint(players[0]->getPosition());
	}
}

void ZVP::updatePlayer(Sprite *player) {
	auto playerInfo = (PlayerInfo*)player->getUserData();
	auto &plantInfo = playerInfo->currentPlantInfo();
	if (plantInfo.healthPoint <= 0) {
		player->setColor(COLOR_ATTACKED);
	}
	else {
		player->setColor(Color3B::WHITE);
	}
	if (player->getPosition().y < -player->getContentSize().height) {
		gameOver();
	}
	if (player->getPositionX() < 0) {
		player->setPositionX(0);
	}
	if (playerInfo->isJumping) {
		player->stopActionByTag(tagAnimationPlayerWalking);
		auto jumpingFrame = SpriteFrameCache::getInstance()->getSpriteFrameByName(player->getName() + "Jumping");
		if (jumpingFrame) {
			player->setSpriteFrame(jumpingFrame);
		}
		return;
	}
	player->getPhysicsBody()->setVelocity(Vec2(playerInfo->velocityX, player->getPhysicsBody()->getVelocity().y));
	Action* animation = player->getActionByTag(tagAnimationPlayerWalking);
	// playVelocity.x ! = 0
	//如果水平速度不为0，播放行走动画
	if (abs(playerInfo->velocityX) > 1e-3f) {
		if (animation == nullptr) {
			animation = RepeatForever::create(Animate::create(AnimationCache::getInstance()->getAnimation(player->getName() + "Walking")));
			animation->setTag(tagAnimationPlayerWalking);
			player->runAction(animation);
		}
	}
	else {
		//否则停止行走动画
		player->stopActionByTag(tagAnimationPlayerWalking);
		//如果此时没有其他动画（例如攻击动画），那就设置为静止时的样子
		if (player->getActionManager()->getNumberOfRunningActionsInTarget(player) == 0) {
			player->setSpriteFrame(SpriteFrameCache::getInstance()->getSpriteFrameByName(player->getName()));
		}
	}
}

void ZVP::updateEnemys(float dt) {
	for (Sprite* enemy : enemys) {
		auto enemyInfo = (EnemyInfo*)enemy->getUserData();
		//如果过了解冻时间，则恢复僵尸速度
		if (enemyInfo->frozenDuration > 0 && Tools::currentTimeInMillis() - enemyInfo->frozenTimeStamp > enemyInfo->frozenDuration) {
			enemy->setColor(Color3B::WHITE);
			enemyInfo->frozenDuration = 0;
			enemyInfo->velocity = enemyInfo->primaryVelocity;
		}

		//更新血条
		auto hp = (ProgressTimer*) enemy->getChildByName("hp");
		hp->setPercentage(100 * enemyInfo->healthPoint / enemyInfo->totalHealthPoint);
		
		switch (enemyInfo->beheavior) {
		case EnemyBehavior::TRACE:
			updateEnemyTraceBehavior(enemy);
			break;
		case EnemyBehavior::RANDOM:
			updateEnemyRandomBehavior(enemy);
			break;
		case EnemyBehavior::REPEAT:
			updateEnemyRepeatBehavior(enemy);
			break;
		case EnemyBehavior::JUMP:
			updateEnemyJumpBehavior(enemy);
			break;
		case EnemyBehavior::AI:
			updateEnemyAiBehavior(enemy);
			break;
		}
	}
}

void ZVP::updateEnemyTraceBehavior(Sprite *enemy) {
	auto enemyInfo = (EnemyInfo*)enemy->getUserData();
	if (players.empty()) {
		return;
	}
	if (enemyInfo->tracingPlayer < 0 || enemyInfo->tracingPlayer >= players.size()) {
		enemyInfo->tracingPlayer = random(0, (int) players.size() - 1);
	}
	auto player = players[enemyInfo->tracingPlayer];
	auto tv = player->getPosition() - enemy->getPosition();
	tv.normalize();
	enemy->getPhysicsBody()->setVelocity(tv * enemyInfo->velocity);
	enemy->setFlippedX(tv.x > 0);
	
}

void ZVP::updateEnemyRandomBehavior(Sprite *enemy) {
	// TODO 
}

void ZVP::updateEnemyJumpBehavior(Sprite *enemy) {
	// TODO 
}

void ZVP::updateEnemyRepeatBehavior(Sprite *enemy) {
	auto enemyInfo = (EnemyInfo*)enemy->getUserData();
	auto boundingBox = enemy->getBoundingBox();
	//获取脚下的方块
	auto underfootingBlock = getLandTileAt(Vec2(enemy->isFlippedX() ? boundingBox.getMaxX() : boundingBox.getMinX(),
		boundingBox.getMinY() - 16));
	//如果脚下没有方块则掉头
	if (underfootingBlock == nullptr) {
		enemy->setFlippedX(!enemy->isFlippedX());
	}
	else {
		//来回走的僵尸碰到障碍物自动掉头。
		//获取前面的方块
		for (int i = 0; i <= enemy->getContentSize().height / 32; i++) {
			auto frontBlock = getLandTileAt(Vec2(enemy->isFlippedX() ? boundingBox.getMaxX() : boundingBox.getMinX(),
				boundingBox.getMinY() + 16 + i * 32));
			if (frontBlock != nullptr) {
				enemy->setFlippedX(!enemy->isFlippedX());
				break;
			}
		}
	}
	enemy->getPhysicsBody()->setVelocity(Vec2(enemy->isFlippedX() ? enemyInfo->velocity : -enemyInfo->velocity, enemy->getPhysicsBody()->getVelocity().y));
}

void ZVP::updateEnemyAiBehavior(Sprite *enemy) {
	// TODO 
}

void  ZVP::updateGameInfo(float dt) {
	for (size_t i = 0; i < players.size(); i++) {
		auto player = players[i];
		PlayerInfo* playerInfo = (PlayerInfo*)player->getUserData();
		auto w = visibleSize.width / players.size() * (i + 1);
		auto sunlightLabel = playerInfo->infoViews.sunlightLabel;
		auto sunlightIcon = playerInfo->infoViews.sunlightIcon;
		auto hpLabel = playerInfo->infoViews.hpLabel;
		auto &plantCards = playerInfo->infoViews.plantCards;

		//更新阳光信息
		sunlightLabel->setString(to_string(playerInfo->sunlight));
		sunlightLabel->setPosition(w - sunlightLabel->getContentSize().width / 2 - 10, visibleSize.height - 30);
		sunlightIcon->setPosition(sunlightLabel->getBoundingBox().getMinX() - sunlightIcon->getContentSize().width / 2 - 5, sunlightLabel->getPosition().y + 3);
		//更新生命值信息
		hpLabel->setString("HP " + to_string(playerInfo->currentPlantInfo().healthPoint));
		hpLabel->setPosition(sunlightIcon->getBoundingBox().getMinX() - hpLabel->getContentSize().width / 2 - 15, visibleSize.height - 30);
		//更新植物信息
		for (unsigned int j = 0; j < playerInfo->plants.size(); j++) {
			if (j >= plantCards.size()) {
				break;
			}
			const string &name = playerInfo->plants[j];
			auto &plantInfo = playerInfo->plantInfo[name];
			if (plantInfo.isFrozen) {
				auto shadow = plantCards[j]->getChildByName("shadow");
				auto remainingTime = plantInfo.frozenDuration - (Tools::currentTimeInMillis() - plantInfo.deadTime);
				if (remainingTime <= 0) {
					plantInfo.isFrozen = false;
					shadow->setContentSize(Size(shadow->getContentSize().width, 0));
				}
				else {
					shadow->setContentSize(Size(shadow->getContentSize().width, plantCards[j]->getContentSize().height * remainingTime / plantInfo.frozenDuration));
				}
			}
			//更新生命值
			auto hpLabel = (Label*)plantCards[j]->getChildByName("hp");
			hpLabel->setString(to_string(plantInfo.healthPoint) + " / " + to_string(plantInfo.totalHealthPoint));
			hpLabel->setPosition(plantCards[j]->getContentSize().width / 2, -15);
			plantCards[j]->setColor(plantInfo.healthPoint > 0 ? Color3B::WHITE : Color3B::GRAY);
		}
	}
	
}

void ZVP::initGameInfoLayer() {
	infoLayer = Layer::create();
	for (size_t i = 0; i < players.size(); i++) {
		PlayerInfo* playerInfo = (PlayerInfo*)players[i]->getUserData();
		vector<string> initialPlants = playerInfo->plants;
		playerInfo->plants.clear();
		for (size_t j = 0; j < initialPlants.size(); j++) {
			addPlant(players[i], initialPlants[j], playerInfo->plantInfo[initialPlants[j]]);
		}
		auto sunlightLabel = Label::createWithTTF(TTF_SUNLIGHT_LABEL, "");
		playerInfo->infoViews.sunlightLabel = sunlightLabel;
		infoLayer->addChild(sunlightLabel);
		auto hpLabel = Label::createWithTTF(TTF_SUNLIGHT_LABEL, "HP 0");
		playerInfo->infoViews.hpLabel = hpLabel;
		infoLayer->addChild(hpLabel);
		auto sunlightIcon = createSprite("sunlight");
		playerInfo->infoViews.sunlightIcon = sunlightIcon;
		infoLayer->addChild(sunlightIcon);
	}
	auto pauseButton = ButtonMenuBuilder()
		.font("fonts/ChronicGothic.ttf")
		.fontSize(20)
		.addItem("PAUSE", [=](Ref* ref) {
			pauseAndShowMenu();
		})
		.build();
	pauseButton->setPosition(45, 10); 
	infoLayer->addChild(pauseButton);

	

}

void ZVP::setViewPoint(const Point& playerPosition)
{
	float originalX = getPositionX();
	float x;
	auto playerX = playerPosition.x;

	if (playerX < visibleSize.width / 2) {
		x = 0;
	}
	else if (playerX > mapPixelWidth - visibleSize.width / 2) {
		x = visibleSize.width - mapPixelWidth;
	}
	else {
		x = visibleSize.width / 2 - playerX;
	}
	if (abs(x - originalX) < 1e-3) {
		return;
	}
	setPositionX(x);
	renderVisibleMapObjects();
}

void ZVP::renderVisibleMap() {
	auto visibleRegionStartX = (int)MAX(0, floor(-getPositionX() / 37.5f) - 5);
	auto visibleRegionEndX = (int) MIN(visibleRegionStartX + mapVisibleRegionWidth + 5, m_map->getMapSize().width);
	auto renderStartX = MAX(visibleRegionStartX, mapVisibleRegionEndX);
	renderMapTiles(renderStartX, visibleRegionEndX);
	renderVisibleMapObjects();
	mapVisibleRegionStartX = visibleRegionStartX;
	mapVisibleRegionEndX = visibleRegionEndX;
}

void ZVP::renderMapTiles(int renderStartX, int renderEndX) {
	for (int x = renderStartX; x < renderEndX; x++) {
		for (int y = 0; y < m_map->getMapSize().height; y++)
		{
			Vec2 pos(x, y);
			for (auto iter = tileRenderers.begin(); iter != tileRenderers.end(); iter++) {
				Sprite *tile = iter->first->getTileAt(pos);
				if (tile) {
					auto renderer = iter->second;
					renderer(tile);
				}
			}
		}
	}
}

void ZVP::renderVisibleMapObjects() {
	for (auto iter = objectRenders.begin(); iter != objectRenders.end(); iter++) {
		auto &objects = iter->first->getObjects();
		for (auto &obj : objects) {
			ValueMap &m = obj.asValueMap();
			float x = m.at("x").asFloat() + getPositionX();
			bool notRendered = m.find("__rendered__") == m.end();
			if (x >= 0 && x <= visibleSize.width && notRendered) {
				auto renderer = iter->second;
				renderer(m);
				m["__rendered__"] = true;
			}
		}
	}
}

Point ZVP::getTileCoordFromPosition(Point pos)
{
	int x = pos.x / 37.5;
	int y = (mapPixelHeight - pos.y) / 37.5;
	return Point(x, y);
}

Sprite * ZVP::getLandTileAt(Vec2 pos) {
	auto tilePos = getTileCoordFromPosition(pos);
	if (tilePos.x >= 0 && tilePos.x < m_map->getMapSize().width &&
		tilePos.y >= 0 && tilePos.y < m_map->getMapSize().height) {
		auto tile = grasslandLayer->getTileAt(tilePos);
		return tile;
	}
	return nullptr;
}


void ZVP::gameOver() {
	if (isGameOver) {
		return;
	}
	isGameOver = true;
	pauseGame();

	auto shadow = ButtonMenuBuilder::createShadow(visibleSize, visibleSize / 2, 220);
	shadow->setName("shadow");
	infoLayer->addChild(shadow);

	auto gameover = Sprite::create("gameOver2.png");
	gameover->setName("gameover");
	gameover->setPosition(visibleSize.width / 2 - 30, visibleSize.height / 2);
	infoLayer->addChild(gameover);
	
	if (isAudioEnabled) {
		audio.play2d("music/gameover.wav", false);
	}

	auto dismissMenu = [=](Ref *ref) {
		(dynamic_cast<Button*>(ref))->getParent()->removeFromParent();
		infoLayer->getChildByName("shadow") ->removeFromParent();
		infoLayer->getChildByName("gameover")->removeFromParent();
		Director::getInstance()->resume();
		isGameOver = false;
	};

	auto menu = ButtonMenuBuilder()
		.font("fonts/ChronicGothic.ttf")
		.fontSize(30)
		.margin(20)
		.addItem("REINCARNATE", [=](Ref* ref) {
			dismissMenu(ref);
			for (size_t i = 0; i < players.size(); i++) {
				if (players[i]->getPositionY() <= 0) {
					players[i]->setPositionY(visibleSize.height - 80);
				}
				auto info = (PlayerInfo*)players[i]->getUserData();
				if (info->sunlight <= 0 || info->currentPlantInfo().healthPoint <= 0) {
					info->sunlight = MAX(info->sunlight, 50);
					info->currentPlantInfo().healthPoint = info->currentPlantInfo().totalHealthPoint;
				}
			}
			resumeGame();
		})
		.addItem("BACK", [=](Ref* ref) {
			Director::getInstance()->resume();
			Director::getInstance()->replaceScene(StartScene::createScene());
		})
		.build();
	menu->setPosition(Vec2(visibleSize.width - menu->getContentSize().width / 2 - 20, menu->getContentSize().height / 2));
	infoLayer->addChild(menu);
}

void ZVP::win(Sprite *star) {
	finish();
	if (isAudioEnabled) {
		audio.play2d("music/Win.mp3", false);
	}
	star->removeFromParentAndCleanup(false);
	infoLayer->addChild(star);
	star->setPosition(getPosition() + star->getPosition());
	star->runAction(Sequence::create(
		MoveTo::create(1.0f, visibleSize / 2), 
		DelayTime::create(0.5f), 
		CallFunc::create([=] {
			enterNextGameLevel(0.4f);
	    }),
		ScaleBy::create(0.5f, 45), 
	nullptr));
}

void ZVP::enterNextGameLevel(float delay) {
	config.clearPlayers();
	for (auto &player : players) {
		auto playerInfo = (PlayerInfo*)player->getUserData();
		playerInfo->initialPosX = 10;
		playerInfo->initialPosY = 400;
		config.addPlayer(*playerInfo);
	}
	config.setGameLevel(config.getGameLevel() + 1);
	config.clearTransitveValues();
	runAction(Sequence::create(DelayTime::create(0.5f),CallFunc::create([=] {
		Director::getInstance()->replaceScene(TransitionFade::create(0.8f, ZVP::createScene(config)));
	}), nullptr));
}

void ZVP::finish() {
	CocosDenshion::SimpleAudioEngine::getInstance()->stopBackgroundMusic();
	unscheduleAllCallbacks();
	Director::getInstance()->getEventDispatcher()->removeAllEventListeners();
}

void ZVP::pauseGame() {
	Director::getInstance()->pause();
	getScene()->getPhysicsWorld()->setAutoStep(false);
	CocosDenshion::SimpleAudioEngine::getInstance()->stopBackgroundMusic();

}

void ZVP::resumeGame() {
	Director::getInstance()->resume();
	getScene()->getPhysicsWorld()->setAutoStep(true);
	CocosDenshion::SimpleAudioEngine::getInstance()->resumeBackgroundMusic();

}

void ZVP::saveGame() {
	config.clearPlayers();
	for (auto &player : players) {
		auto playerInfo = (PlayerInfo*)player->getUserData();
		playerInfo->initialPosX = player->getPositionX();
		playerInfo->initialPosY = player->getPositionY();
		config.addPlayer(*playerInfo);
	}
	GameService::saveGameConfig(config);
}

