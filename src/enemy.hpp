#pragma once

//Initial header file for abstract enemy class
#include <vector>

#include "position.hpp"
#include "bullet.hpp"

/*  ***TODO***
* - all return values are void, to be changed for right ones
*/

class Enemy {
    public:
        //Constructor
        Enemy(int hp, int speed, std::vector<Position*> path)
            : hitpoints_(hp), speed_(speed), path_(path) 
            {
                position_ = path_[0];
            }

        //Destructor
        virtual ~Enemy() {}

        //Update the state of enemy, should be virtual
        virtual void Update() = 0;

        //Take damage, should be virtual
        virtual void TakeDamage(Bullet bullet) = 0;

    protected:
        int hitpoints_;
        int speed_;
        std::vector<Position*> path_;
        Position* position_;
};