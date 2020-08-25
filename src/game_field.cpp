
#include "game_field.hpp"

#include <iostream> // for debugging
#include <limits>

#include <SFML/System/Time.hpp>
#include <cassert>
#include "data_tables.hpp"
#include "utility.hpp"


GameField::GameField(sf::RenderWindow& window, sf::Vector2f viewOffset)
	: window_(window),
	viewOffset_(viewOffset), 
	 gameFieldView_(window.getDefaultView()),
	 sceneGraph_(),
	 gameFieldBounds_(0.f, 0.f, // x and y of the game field
	 				gameFieldView_.getSize().x + viewOffset_.x, //world width is a bit bigger than the view's width
					gameFieldView_.getSize().y + viewOffset_.y), // world height is same as view's height
	 spawnPosition_(gameFieldBounds_.left + 100,
	 				 (gameFieldBounds_.top + gameFieldBounds_.height)/3.f),
	commandQueue_(),
	enemySpeed_(50.f),
	firstEnemy_(),
	spawnCountdown_(sf::seconds(2)),
	spawnInterval_(2), //this should maybe be a parameter
	leftToSpawn_(5),
    activeEnemies_(),
	difficultyLevel_(0), //0 is the first level and increases by 1 by each wave
	levelCount_(5), // can be added to parameter list or askef for player, but for now it's just harcoded to be 5
	levelBreakTimer_(sf::seconds(15)),
	newEnemyReachedEnd_(false),
	roundScore_(0),
	hasActiveEnemies_(false)
	{ 
		LoadTextures();
		BuildScene();
		gameFieldView_.setCenter(gameFieldBounds_.left + (gameFieldBounds_.width + viewOffset_.x)/2.f, gameFieldBounds_.top + (gameFieldBounds_.height + viewOffset_.y)/2.f);
	}

void GameField::AddTower(Tower::Type type, sf::Vector2f pos)
{
	std::cout << "Here we are!" << std::endl;

	std::unique_ptr<Tower> newTower;
	switch (type)
	{
	case Tower::Type::Super:
		newTower.reset(new SuperTower(textures_));
		break;
	case Tower::Type::Bombing:
		newTower.reset(new BombingTower(textures_));
		break;
	case Tower::Type::Slowing:
		newTower.reset(new SlowingTower(textures_));
	default:
		newTower.reset(new BasicTower(textures_));
		break;
	}
	//std::unique_ptr<Tower> newTower(new Tower(type, textures_));
	//newTower->setOrigin(newTower->GetBoundingRect().width/2.f, newTower->GetBoundingRect().height/2.f);
	/*newTower->setOrigin(pos.x - (newTower->getPosition().x - newTower->getOrigin().x),
                                            pos.y - (newTower->getPosition().y - newTower->getOrigin().y));
	*/

	newTower->setPosition(pos);
	newTower->AllowMoving();
	newTower->Move();
	newTower->Activate();
	
	sceneLayers_[Field]->AttachChild(std::move(newTower));
	//Tower::ActiveTower(newTower, commandQueue_);
	std::cout << "Tower pos: " << pos.x <<", " << pos.y << std::endl;
}

void GameField::Update(sf::Time dt) {
	newEnemyReachedEnd_ = false; // new enemies have not reached end at the beginning of an update
	roundScore_ = 0; // set round score to zero 

	//std::cout << "difficulty: " << difficultyLevel_ << std::endl;

	DestroyEntitiesOutsideView();
	//DestroyDetonatedBombs();

	//makes towers shoot
	MakeTowersShoot();

//std::cout << "made the towers shoot" << std::endl;
	
	// Forwards the commands to self or the scene graph
	while(!commandQueue_.IsEmpty()) {
		Command next = commandQueue_.Pop();
		if (next.category_ == Category::GameField)
		{
			OnCommand(next, dt);
		}
		else 
		{
			sceneGraph_.OnCommand(next, dt);
		}
	}
	//std::cout << "forwarded the commands" << std::endl;
	HandleCollisions();

	sceneGraph_.RemoveDestroyedNodes();
	SpawnEnemies(dt);
	sceneGraph_.Update(dt, commandQueue_);
}

void GameField::LoadTextures() {
	
	textures_.Load(Textures::ID::Fire, "../media/textures/Doge.png");
	textures_.Load(Textures::ID::Leaf, "../media/textures/cat.png");
	textures_.Load(Textures::ID::Grass, "../media/textures/grass.jpg");
	textures_.Load(Textures::ID::BasicTower, "../media/textures/tower.png");
	textures_.Load(Textures::ID::SuperTower, "../media/textures/harvester.png");
	textures_.Load(Textures::ID::SlowingTower, "../media/textures/tower.png");
	textures_.Load(Textures::ID::BombingTower, "../media/textures/tower.png");
	textures_.Load(Textures::ID::BasicBullet, "../media/textures/bullet.png");
	textures_.Load(Textures::ID::SuperBullet, "../media/textures/bullet.png");
	textures_.Load(Textures::ID::Bomb, "../media/textures/bullet.png");
	textures_.Load(Textures::ID::NoTexture,      "../media/textures/noTexture.png");
	textures_.Load(Textures::ID::DeathAnimation,      "../media/textures/deathAnimation.png");
	textures_.Load(Textures::ID::Leppis,      "../media/textures/leppakerttu.png");
	textures_.Load(Textures::ID::HamahakkiIso,      "../media/textures/hamahakki.png");
}

void GameField::BuildScene() {
	// Initialize all the scene layers, i.e., the SceneNodes that are rendered together
	for (std::size_t i = 0; i < LayerCount; i++) {
		Category::Type category = (i == Field) ? Category::Scene : Category::None;

		SceneNode::Ptr layer(new SceneNode(category));
		sceneLayers_[i] = layer.get();
		sceneGraph_.AttachChild(std::move(layer));
	}

	//Make the background
	sf::Texture& texture = textures_.Get(Textures::ID::Grass);
	sf::IntRect textureRect(gameFieldBounds_);
	texture.setRepeated(true);

	std::unique_ptr<SpriteNode> backgroundSprite(new SpriteNode(texture, textureRect));
	backgroundSprite->setPosition(gameFieldBounds_.left, gameFieldBounds_.top);
	sceneLayers_[Background]->AttachChild(std::move(backgroundSprite));


	//Initialize an enemy
	std::unique_ptr<Enemy> firstEnemy(new BasicEnemy(textures_));
	firstEnemy_ = firstEnemy.get();
	//firstEnemy_->setOrigin(firstEnemy_->GetBoundingRect().width/2, firstEnemy_->GetBoundingRect().height/2);
	firstEnemy_->setPosition(spawnPosition_);
	//firstEnemy_->setScale(0.5f, 0.5f);

	std::cout << "DEBUG: spawn position:" << firstEnemy_->getPosition().x << "," << firstEnemy_->getPosition().y << std::endl;

	firstEnemy_->SetVelocity(enemySpeed_, 0.f);
	std::cout << "DEBUG: initial velocity: " << firstEnemy_->GetVelocity().x << "," << firstEnemy_->GetVelocity().y << std::endl;
 
	sceneLayers_[Field] -> AttachChild(std::move(firstEnemy));

	//Initialize a tower that can be moved with hard-coded bullet
	std::unique_ptr<Tower> firstTower(new SuperTower(textures_));
	// firstTower_ = firstTower.get();
	//firstTower->setOrigin(firstTower->GetBoundingRect().width/2, firstTower->GetBoundingRect().height/2);

	firstTower->setPosition((gameFieldBounds_.left + gameFieldBounds_.width)/2.f, (gameFieldBounds_.top + gameFieldBounds_.height)/2.f+250);
	firstTower->DisallowMoving();
	sceneLayers_[Field] -> AttachChild(std::move(firstTower));


	//Initialize a slowing tower
	std::unique_ptr<Tower> secondTower(new SlowingTower(textures_));
	//firstTower->setOrigin(firstTower->GetBoundingRect().width/2, firstTower->GetBoundingRect().height/2);
	secondTower->setPosition((gameFieldBounds_.left + gameFieldBounds_.width)/3.f +200.f, (gameFieldBounds_.top + gameFieldBounds_.height)/3.f);
	secondTower->DisallowMoving();
	sceneLayers_[Field] -> AttachChild(std::move(secondTower));

	// Initialize a bombing tower
	std::unique_ptr<Tower> thirdTower(new BombingTower(textures_));
	//firstTower->setOrigin(firstTower->GetBoundingRect().width/2, firstTower->GetBoundingRect().height/2);
	thirdTower->setPosition((gameFieldBounds_.left + gameFieldBounds_.width)/4.f-150.f, (gameFieldBounds_.top + gameFieldBounds_.height) -530.f);
	thirdTower->DisallowMoving();
	sceneLayers_[Field] -> AttachChild(std::move(thirdTower));


}

bool MatchesCategories(SceneNode::Pair& colliders, Category::Type type1, Category::Type type2)
{
	unsigned int category1 = colliders.first->GetCategory();
	unsigned int category2 = colliders.second->GetCategory();

	// Make sure first pair entry has category type1 and second has type2
	if (type1 & category1 && type2 & category2)
	{
		//std::cout << "matching category found" << std::endl;
		return true;
	}
	else if (type1 & category2 && type2 & category1)
	{
		//std::cout << "matching category found" << std::endl;
		std::swap(colliders.first, colliders.second);
		return true;
	}
	else
	{
		return false;
	}
}

void GameField::HandleCollisions()
{
	
	std::set<SceneNode::Pair> collisionPairs;
	sceneGraph_.CheckSceneCollision(sceneGraph_, collisionPairs);
	bool towerCollideCalled = false;

	for(SceneNode::Pair pair : collisionPairs)
	{
		//std::cout << pair.first->GetCategory() << "," << pair.second->GetCategory() << std::endl;
		if (MatchesCategories(pair, Category::Enemy, Category::Bullet))
		{
			//std::cout << "Collision happened!!!" << std::endl;
			auto& enemy = static_cast<Enemy&>(*pair.first);
			auto& bullet = static_cast<Bullet&>(*pair.second);
			if(bullet.IsDestroyed())
			{
				continue;
			}
			// Apply bullet damage to enemy, destroy bullet
			enemy.TakeHit(bullet.GetDamage(), bullet.GetCategory());
			if (enemy.IsDestroyed())
			{
				AddRoundScore(enemy.GetScorePoints());
			}
			std::cout << "HP now: " << enemy.GetHitpoints() << std::endl;
			bullet.Destroy();

			//std::cout << "Collision occurred on enemy: " << enemy.Get<< std::endl;
		}
		if (MatchesCategories(pair, Category::Tower, Category::Active))
		{
			auto& tower = static_cast<Tower&>(*pair.first);
			auto& activeTower = static_cast<Tower&>(*pair.second);
			if (tower.IsMoving())
			{
				tower.Collides(true);
				towerCollideCalled = true;
			}
			if (activeTower.IsMoving())
			{
				activeTower.Collides(true);
				towerCollideCalled = true;
			}
				
			
			

		}
	}
	if (!towerCollideCalled)
		{
			Command command;
			command.category_ = Category::Tower;
			command.action_ = DerivedAction<Tower>([=](Tower& t, sf::Time)
			{
				t.Collides(false);
			});
			commandQueue_.Push(command);

		}
}
void GameField::BuildPath()
{
	
}

void GameField::OnCommand(Command command, sf::Time dt) 
{
	command.gameFieldAction_(*this, dt);
}

//Spawns only one type of enemies and spawnInterval is constant
void GameField::SpawnEnemies(sf::Time dt) {
	if (leftToSpawn_ > 0)//TODO leftToSpawn someway better?
	{
		if (spawnCountdown_ <= sf::Time::Zero) 
    	{
			spawnCountdown_ += sf::seconds(spawnInterval_);
			//alternative way and probably better in actual game, change spawnInterval to spawnRate to make spawnrate <= 1 sec
			//spawnCountdown_ += sf::seconds(1.f / (spawnRate_+1));

			RandomEnemySpawner(difficultyLevel_+1);
			leftToSpawn_--;

		} else 
		{
			spawnCountdown_ -= dt;
		}
	}
	else if (difficultyLevel_ < levelCount_)
	{
		// temporary solution, gameState should handle level changes, if difficultyLevel_ == levelCount_ => game won
		if (levelBreakTimer_ <= sf::Time::Zero)
		{
			difficultyLevel_++;
			if (difficultyLevel_ < levelCount_)
			{
				leftToSpawn_ = 5;
				levelBreakTimer_ = sf::seconds(10);
			}
		} else
		{
			levelBreakTimer_ -= dt;
		}
	}

}

void GameField::RandomEnemySpawner(unsigned int level)
{
	int num = RandomInt(level); //random int that is max level-1

		//this works only for current enemy types, probably cannot implemet for arbitrary count of enemy types
		switch(num)
		{
			case 0: 
			{
				std::unique_ptr<BasicEnemy> newEnemy(new BasicEnemy(textures_, difficultyLevel_));
				newEnemy->setPosition(spawnPosition_);
				newEnemy->SetVelocity(enemySpeed_, 0.f); //this need to be tought again if we have multiple paths
				sceneLayers_[Field] -> AttachChild(std::move(newEnemy));
			}
				break;
			case 1:
			{
				std::unique_ptr<MultiEnemy> newEnemy(new MultiEnemy(textures_, difficultyLevel_));
				newEnemy->setPosition(spawnPosition_);
				newEnemy->SetVelocity(enemySpeed_, 0.f); //this need to be tought again if we have multiple paths
				sceneLayers_[Field] -> AttachChild(std::move(newEnemy));
			}
				break;
			case 2:
			{
				std::unique_ptr<BulkEnemy> newEnemy(new BulkEnemy(textures_, difficultyLevel_));
				newEnemy->setPosition(spawnPosition_);
				newEnemy->SetVelocity(enemySpeed_, 0.f); //this need to be tought again if we have multiple paths
				sceneLayers_[Field] -> AttachChild(std::move(newEnemy));
			}
				break;
			case 3:
			{
				std::unique_ptr<FastEnemy> newEnemy(new FastEnemy(textures_, difficultyLevel_));
				newEnemy->setPosition(spawnPosition_);
				newEnemy->SetVelocity(enemySpeed_, 0.f); //this need to be tought again if we have multiple paths
				sceneLayers_[Field] -> AttachChild(std::move(newEnemy));
			}
				break;
			default:
			{
				std::unique_ptr<BulkEnemy> newEnemy(new BulkEnemy(textures_, difficultyLevel_));
				newEnemy->setPosition(spawnPosition_);
				newEnemy->SetVelocity(enemySpeed_, 0.f); //this need to be tought again if we have multiple paths
				sceneLayers_[Field] -> AttachChild(std::move(newEnemy));
			}
		}
}


void GameField::Draw() {
	window_.setView(gameFieldView_);
	window_.draw(sceneGraph_);
	

}

CommandQueue& GameField::GetCommandQueue() {
	return commandQueue_;
}
/*
void GameField::HandleActiveTower()
{
	Command command;
	command.category_ = Category::ActiveTower;
	command.action_ = DerivedAction<Tower>([this] (Tower& t, sf::Time)
	{

	});
	commandQueue_.Push(command);
}*/

bool GameField::HasNewEnemiesReachedEnd() {
	return newEnemyReachedEnd_;
}

//can be used to determine when current wave is finished
bool GameField::EndOfLevel()
{
	return !hasActiveEnemies_ && leftToSpawn_ <= 0;
}

bool GameField::HasEnemiesToSpawn()
{
	return leftToSpawn_ > 0 || difficultyLevel_ < levelCount_;
}

int GameField::GetRoundScore()
{
	return roundScore_;
}

void GameField::AddRoundScore(int score)
{
	roundScore_ += score;
}

void GameField::DestroyEntitiesOutsideView()
{
	Command bulletCommand;
	bulletCommand.category_ = Category::Bullet;
	bulletCommand.action_ = DerivedAction<Entity>([this] (Entity& e, sf::Time)
	{
		if (!GetGamefieldBounds().intersects(e.GetBoundingRect()))
		{
			std::cout << "destroying bullet outside gamefield" << std::endl;
			e.Destroy();
		}	
	});

	Command enemyCommand;
	enemyCommand.category_ = Category::Enemy;
	enemyCommand.action_ = DerivedAction<Enemy>([this] (Enemy& e, sf::Time)
	{
		if (!GetGamefieldBounds().intersects(e.GetBoundingRect()))
		{
			std::cout << "destroying enemy outside gamefield and also reduce player lives" << std::endl;
			e.Destroy();
			newEnemyReachedEnd_ = true; // this should maybe be an int, because more than one enemies can reach end at the same time
		}	
	});

	commandQueue_.Push(bulletCommand);
	commandQueue_.Push(enemyCommand);
}

/* void GameField::DestroyDetonatedBombs() {
	Command command;
	command.category_ = Category::Bomb;
	command.action_ = DerivedAction<Bomb>([] (Bomb& bomb, sf::Time) {
		if (bomb.IsDetonated()) {
			bomb.Destroy();
		}
	});

	commandQueue_.Push(command);
} */

sf::FloatRect GameField::GetViewBounds() const
{
	return sf::FloatRect(gameFieldView_.getCenter() - gameFieldView_.getSize() / 2.f, gameFieldView_.getSize());
}

sf::FloatRect GameField::GetGamefieldBounds() const
{
	// Return view bounds + some area at top, where enemies spawn
	sf::FloatRect bounds = GetViewBounds();
	bounds.top -= 100.f;
	bounds.height += 100.f;

	return bounds;
}

// Does both: shoots enemies or slows them down depending on tower category
void GameField::MakeTowersShoot()
{
	// Setup command that stores all active enemies to activeEnemies_
	Command enemyCollector;
	enemyCollector.category_ = Category::Enemy;
	enemyCollector.action_ = DerivedAction<Enemy>([this] (Enemy& enemy, sf::Time)
	{
		if (!enemy.IsDestroyed())
		{
			activeEnemies_.push_back(&enemy);
		}
		if (!enemy.IsMarkedForRemoval())
		{
			hasActiveEnemies_ = true;
		}

	});

	Command shootBullets;
	shootBullets.category_ = Category::Tower; 
	shootBullets.action_ = DerivedAction<Tower>([this] (Tower& tower, sf::Time)
	{
		// Ignore towers that can't shoot right now
		if (!tower.CanShoot())
			return;

		float minDistance = std::numeric_limits<float>::max();
		Enemy* closestEnemy = nullptr;

		// Find closest enemy
		for(Enemy* enemy : activeEnemies_)
		{
			float enemyDistance = Distance(tower, *enemy);

			if (enemyDistance <= tower.GetRange())
			{
				// Does not work yet, but should be sufficient to slow enemies if tower is category SlowingTower
				if (tower.GetCategory() == Category::SlowingTower) {
					enemy->SlowDown(); //duration?
					continue;
				}
				if (enemyDistance < minDistance)
				{
					closestEnemy = enemy;
					minDistance = enemyDistance;
				}
			}
			
		}

		if (closestEnemy)
		{
			sf::Vector2f direction(closestEnemy->GetWorldPosition() - tower.GetWorldPosition());
			//std::cout << "shooting direction: " << direction.x << ", " << direction.y << std::endl;
			tower.Shoot(commandQueue_, direction);
		}
			
	});

	Command detonateCommand;
	detonateCommand.category_ = Category::Bomb;
    detonateCommand.action_ = DerivedAction<Bomb>([this] (Bomb& bomb, sf::Time) 
	{
		if (!bomb.CanDetonate())
		{
			return;
		}
		for(Enemy* enemy : activeEnemies_)
		{
			if (Distance(bomb, *enemy) <= bomb.GetRange()) {
                enemy->TakeHit(bomb.GetDamage(), bomb.GetCategory());
				//bomb.Detonate()
            }
			if (enemy->IsDestroyed())
			{
				AddRoundScore(enemy->GetScorePoints());
			}
		}
		bomb.Destroy();
	});


	// Push commands, reset active enemies
	commandQueue_.Push(enemyCollector);
	commandQueue_.Push(shootBullets);
	commandQueue_.Push(detonateCommand);

	if (activeEnemies_.empty())
	{
		hasActiveEnemies_ = false;
	}
	activeEnemies_.clear();

}
