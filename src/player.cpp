#include "player.hpp"
#include "towers/tower.hpp"


Player::Player(sf::RenderWindow& window, sf::Vector2f viewOffset) : window_(window), viewOffset_(viewOffset) { }

// Adapted from SFML Game Development-book

void Player::HandleEvent(const sf::Event& event, CommandQueue& commands) {
    // Refactor this to the Controller class if needed
    if (event.type == sf::Event::MouseButtonPressed) {
        Command activate;
        activate.category_ = Category::Tower;
        activate.action_ = DerivedAction<Tower> (
           [event, &commands] (Tower& tower, sf::Time dt) {
               if (tower.GetBoundingRect().contains(sf::Vector2f(event.mouseButton.x,event.mouseButton.y))) {
                   std::cout << "Tower was clicked!" << std::endl;
                   Tower::ActiveTower(tower, commands);
                }
                /*
                else
                {
                    tower.Deactivate();
                }*/
           }
        );
        commands.Push(activate);
        
    }


}
//Adapted from the SFML Game Development-book
void Player::HandleRealtimeInput(CommandQueue& commands) {

}