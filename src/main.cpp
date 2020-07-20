#include<iostream>
#include<vector>

#include "game_field.hpp"
#include "tower.hpp"
#include "enemy.hpp"
#include "bullet.hpp"
#include "test_tower.hpp"
#include "position.hpp"
#include "enemies/test_enemy.hpp"
 
int main(){
	Position pos = Position(1,2);   
    std::cout << pos << std::endl;
   
	TestTower testi = TestTower(Position(1,2));
	
	std::cout << "Testing tower" << std::endl;
	testi.Shoot();
	testi.Update();
	std::cout << "Testing done" << std::endl;

   std::cout << "Testing Enemy" << std::endl;
   std::vector<Position*> path1;
   Position* p1 = new Position(1,1);
   path1.push_back(p1);
   std::cout << *p1 << std::endl;
   TestEnemy* enemy1 = new TestEnemy(path1);


   return 0;
}