#include "tower.hpp"
#include <cassert>

#include "../utility.hpp"

#include <math.h> 

namespace {
	const std::vector<TowerData> table = InitializeTowerData();
}

Tower* Tower::activeTower_  = nullptr;

int Tower::towerCount_ = 0;


Tower* Tower::ActiveTower()
{ 
    return activeTower_; 
}

void Tower::ActiveTower(Tower* newActive) 
{ 
    assert(newActive != nullptr);

    if (activeTower_)
        activeTower_->Deactivate();

    activeTower_ = newActive;
    activeTower_->Activate();
    // For debugging:
    std::cout << "New active tower added" << std::endl;

}

int Tower::TowerCount() { return towerCount_; }

Tower::Tower(Tower::Type type, const TextureHolder &textures)
    : Entity(1), 
    type_(type), 
    range_(table[type].range),
    sprite_(textures.Get(table[type].texture)), 
    direction_(),
    reloadTime_(table[type].reloadTime), 
    canShoot_(false), 
    bulletType_(table[type].bulletType), 
    countdown_(sf::Time::Zero),
    shootCommand_() 
    {
        sf::FloatRect bounds = sprite_.getLocalBounds();
        sprite_.setOrigin(bounds.width/2.f, bounds.height/2.f);

        shootCommand_.category_ = Category::Scene;
        shootCommand_.action_ = [this, &textures] (SceneNode& node, sf::Time) {
            CreateBullet(node, textures);
        };
        Tower::towerCount_++;
        std::cout << "towercount: " << Tower::towerCount_ << std::endl;
        if (Tower::towerCount_ == 1) {
            Tower::ActiveTower(this);
        }
            
    }


// Function for drawing the tower
void Tower::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(sprite_, states);
}


//Update the state of the tower, should be virtual
void Tower::UpdateCurrent(sf::Time dt, CommandQueue&) {
    // std::cout << "Updating tower" <<std::endl;

    //if tower hasn't shot yet (no enemies are in range), do nothing and do not reduce countdown
    if (countdown_ <= sf::Time::Zero) {
        canShoot_ = true;
        std::cout << "It can! Hurrah! " << std::endl;
        countdown_ += sf::seconds(reloadTime_);
    } else if (countdown_ > sf::Time::Zero  && !canShoot_) {
        // std::cout << "It cannot :(" << std::endl;
        countdown_ -= dt;
    }

}

void Tower::Shoot(CommandQueue& commands, sf::Vector2f direction) {
    // this is a first try at rotating the tower
    // this is maybe not necessary, if towers are depicted somewhere else than strictly from above
    /*int degrees = pow(tan(direction.y / direction.x), -1);
    this->setRotation(degrees);*/

    canShoot_ = false;
    direction_ = UnitVector(direction);
    std::cout << "direction in Shoot function: " << direction_.x << ", " << direction_.y << std::endl;
    commands.Push(shootCommand_);
}

unsigned int Tower::GetCategory() const
{
    return Category::Tower;
}

void Tower::Activate()
{
    isActive_ = true;
}

void Tower::Deactivate()
{
    isActive_ = false;
}

bool Tower::IsActive() const
{
    return isActive_;
}

// This sets the permission for the tower to move
// for now: this maybe is a clumsy way to achieve this
void Tower::AllowMoving()
{
    canMove_ = true;
}

void Tower::DisallowMoving() 
{
    Stop();
    canMove_ = false;
}

// Getter of permission to move
bool Tower::CanMove() const
{
    return canMove_;
}

bool Tower::CanShoot() const 
{
    return canShoot_;
}

// Sets the tower moving
// Returns true if setting was succesful
bool Tower::Move()
{
    if (canMove_)
        isMoving_ = true;
    return canMove_;
}

bool Tower::Stop()
{
    isMoving_ = false;
}

// Getter that tells if the tower is being moved by the player
bool Tower::IsMoving() const {
    return isMoving_;
}

sf::FloatRect Tower::GetBoundingRect() const {
    return GetWorldTransform().transformRect(sprite_.getGlobalBounds()); 
}

float Tower::GetRange() const
{
    return range_;
}


void Tower::CreateBullet(SceneNode& node, const TextureHolder& textures) const {
    std::cout << "Creating a bullet" << std::endl;

    std::unique_ptr<Bullet> bullet(new Bullet(static_cast<Bullet::Type>(bulletType_), textures));

    //sf::Vector2f offset(sprite_.getGlobalBounds().width / 2.f, sprite_.getGlobalBounds().height / 2.f);
    //sf::Vector2f velocity(0.0f, 100.0f);

    bullet->setPosition(GetWorldPosition());

    bullet->SetVelocity(bullet->GetSpeed() * direction_);
    std::cout << "Bullet velocity: " << bullet->GetVelocity().x << ", " << bullet->GetVelocity().y << std::endl;
    node.AttachChild(std::move(bullet));
}
