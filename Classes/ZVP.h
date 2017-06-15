#pragma once
#include "cocos2d.h"
#include "AnimationInfo.h"
#include "GameConfig.h"
#include "EnemyInfo.h"
#include "PlayerInfo.h"
#include "GameService.h"
#include "AudioEngine.h"
#include "ButtonMenu.h"
using namespace std;
USING_NS_CC;
class ZVP : public Layer {
public:
	static cocos2d::Scene* createScene(GameConfig config = GameConfig::defaultConfig());
	static Layer *createBackgroundLayer();

	void setPhysicsWorld(PhysicsWorld * world);
	virtual bool init();
	
	void preloadMusic();

	//加载游戏地图
	void  loadMap();
	//"渲染"地图的可见部分。渲染指的是把地图的方块转换、装饰或创建为相应的精灵。更准备的用词应该是解析。不换了。
	void renderVisibleMap();
	//"渲染"从renderStartX到renderEndX（包含）的地图方块
	void renderMapTiles(int renderStartX, int renderEndX);
	//"渲染"地图可见部分的游戏对象
	void renderVisibleMapObjects();
	//"渲染"道路方块。例如草地和黄色的土地。
	void renderLandTile(Sprite* tile);
	//"渲染"来回移动的道路方块。就像超级玛丽第三关那种空中的一块移来移去的那种。
	// 以及上下移动的道路方块。就像超级玛丽第二关那种空中的、从上到下、不断循环、有点像电梯的那种。
	void renderMovingBlock(const ValueMap &obj);
	//"渲染"奖励方块。就是那种问号的方块。顶一下可以有阳光、卡片等。
	void renderRewardBlock(const ValueMap &obj);
	//"渲染"僵尸对象。
	void renderZombieObject(const ValueMap &obj);
	//"渲染"阳光方块
	void renderSunlightTile(Sprite* tile);
	
	void addPlayer();
	//加载角色动画信息。包括一个精灵的动画、图片文件的帧数、大小。以便加载这个精灵时可以使用。
	void loadIdleAnimationInfo();
	//加载角色。包括一个精灵的动画、图片。
	void loadIdle(const string &name);
	//根据name找到相应的图片文件，根据帧数和大小切割。
	void loadAnimation(const string &name, size_t frameCount, const Size &frameSize, float interval = 0.1f);
	//设置玩家的当前植物。（切换植物）
	void setPlayerPlant(Sprite *player, const string &name);

	void addListeners();

	//控制
	void onKeyPressed(EventKeyboard::KeyCode code, Event * event);
	//暂停并显示游戏菜单
	void pauseAndShowMenu();
	void onMenuTouchEvent(Ref * ref, Widget::TouchEventType type);
	void onPlayerKeyPressed(int i, EventKeyboard::KeyCode code, Event * event);
	void onKeyReleased(EventKeyboard::KeyCode code, Event * event);
	void movePlayer(Sprite * player, EventKeyboard::KeyCode code);
	void stopPlayer(Sprite* player, EventKeyboard::KeyCode code);

	//让玩家做出攻击
	void makeAttack(Sprite *player);
	//发射豌豆。用于豌豆射手、连发豌豆射手、寒冰射手的攻击。
	void shootPea(Sprite *player);
	//爆炸。用于火爆辣椒等炸弹的攻击。
	void boom(Sprite *player);
	//回收敌人
	void killEnemy(Sprite *enemy);

	bool onConcactBegin(PhysicsContact & contact);
	bool handlePlayerCollision(Sprite *player, Node *anotherSprite);
	//处理玩家与道路的碰撞
	void handlePlayerCollisionWithLand(Sprite* player, Sprite* land);

	void handleRewardBlockCollided(Sprite* player, Sprite *block);
	//处理玩家与阳光的碰撞
	void handlePlayerCollisionWithSublight(Sprite* player, Sprite* goldCoin);
	//处理玩家与敌人的碰撞。包括所有僵尸。
	void handlePlayerCollisionWithEnemy(Sprite* player, Sprite* enemy);
	//处理玩家与植物卡片的碰撞。玩家将会获取这张植物卡片。
	void handlePlayerCollisionWithPlantCard(Sprite* player, Sprite* mushroom);
	//处理发射的豌豆和敌人的碰撞。敌人生命值减少。如果是寒冰射手的豌豆，则速度同时变慢、攻击力降低。
	void handlePeaCollisionWithEnemy(Sprite *pea, Sprite* enemy);
	//僵尸收到攻击的处理。attack为攻击力，即僵尸减少的生命值。
	bool onEnemyAttacked(Sprite * enemy, int attack);
	void onContactSeparate(PhysicsContact & contact);

	//创建一张植物卡片。
	Sprite *createPlantCard(const string &plantName);
	//在pos上方创建一张植物卡片。
	void createPlantCardAbove(const string &plantName, const Vec2 &pos);
	//在位置(x, y)创建一个问号方块。
	Sprite *createQuestionBlockAt(float x, float y);
	//在位置(x, y)创建一个草地方块。
	Sprite *createGrasslandBlock();
	//根据僵尸名称创建僵尸
	Sprite *createZombie(const string &name);
	//创建一个精灵。将根据精灵名称找到相应的图片文件（例如Sunlight对应Sunlight.png），不指定宽高则为图片的宽高。
	Sprite *createSprite(const string &name, float w = -1, float  h = -1);
	//为玩家增加一个植物。
	void addPlant(Sprite *player, const string &plantName, PlantInfo plantInfo, Sprite* card = nullptr);
	//给植物卡片添加植物信息。比如生命值、所需阳光。
	void appendPlantCardInfo(Sprite* card,  PlantInfo *plantInfo);
	//打碎砖块效果。用于坚果顶碎方块时的动画。
	void breakBlock(Sprite *block);

	//让精灵来回移动
	void makeSpriteMoveBackAndForth(Sprite *sprite, float distance, float duration);

	void update(float dt);
	// 设置"摄像头"位置。根据玩家位置调整Layer位置。
	void setViewPoint(const Point & point);
	void updatePlayer(Sprite *player);

	void updateEnemys(float dt);
	//更新僵尸的跟踪行为
	void updateEnemyTraceBehavior(Sprite *enemy);
	//更新僵尸的随机行为
	void updateEnemyRandomBehavior(Sprite *enemy);
	//更新跳跃僵尸的行为
	void updateEnemyJumpBehavior(Sprite *enemy);
	//更新走来走去的僵尸的行为
	void updateEnemyRepeatBehavior(Sprite *enemy);
	//更新智能僵尸的行为
	void updateEnemyAiBehavior(Sprite *enemy);

	void initGameInfoLayer();
	//更新游戏信息。包括阳光、生命值等。
	void updateGameInfo(float dt);

	//将坐标转换为方块坐标。前者是正常的坐标。后者(方块坐标)是方块在地图中的位置，例如第一个方块为(0, 0)，第一行第二个方块为(1, 0)
	Point getTileCoordFromPosition(Point pos);
	//获取在某个位置的道路方块。pos是正常坐标。
	Sprite *getLandTileAt(Vec2 pos);

	void gameOver();

	void win(Sprite * star);

	void enterNextGameLevel(float delay);

	void finish();

	void pauseGame();

	void resumeGame();

	void saveGame();

	ZVP(const GameConfig &config) {
		this->config = config;
	}

	static ZVP* create(const GameConfig &config) 
	{ 
		ZVP *pRet = new(std::nothrow) ZVP(config);
		if (pRet && pRet->init()) 
		{ 
			pRet->autorelease(); 
			return pRet; 
		} 
		else 
		{ 
			delete pRet; 
			pRet = nullptr; 
			return nullptr; 
		} 
	}

private:
	PhysicsWorld* m_world;
	//游戏地图
	TMXTiledMap* m_map;
	//地图的草地层
	TMXLayer* grasslandLayer;
	//地图宽度和高度
	float mapPixelWidth, mapPixelHeight;
	Size visibleSize;
	//地图可见区域在横坐标上的方块数量，地图可见区域的开始方块坐标、结束方块坐标
	int mapVisibleRegionWidth, mapVisibleRegionStartX = 0, mapVisibleRegionEndX = 0;
	//方块"渲染器"的map。保存各种方块对应的渲染器。参见renderVisibleMap函数。
	map<TMXLayer*, function<void(Sprite*)> > tileRenderers;
	//对象"渲染器"的map。对象指的是方块以外的对象。例如僵尸、道具等。参见renderVisibleMap函数。
	map<TMXObjectGroup*, function<void(const ValueMap &obj)> >  objectRenders;

	//角色的动画信息
	map<string, AnimationInfo> idleAnimInfos;
	
	Layer* infoLayer = nullptr;

	vector<Sprite*> players;
	set<Sprite*> enemys;
	GameConfig config;
	bool isGameOver = false;
	experimental::AudioEngine audio;

};