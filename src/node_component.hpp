#pragma once
#include "component.hpp"
#include "resource_identifiers.hpp"
#include "resource_holder.hpp"
#include "scene_node.hpp"

#include <SFML/Graphics/Text.hpp>


namespace GUI
{
// Represents a GUI component that has a Scene node in it
class SceneNodeComponent : public Component
{
    public:
        typedef std::shared_ptr<SceneNodeComponent> Ptr;
            

	public:
							SceneNodeComponent(SceneNode* node);

        virtual bool		IsSelectable() const;
		void				SetNode(SceneNode* node);
        SceneNode*          GetNode() const;

        virtual void		HandleEvent(const sf::Event& event);


    private:
        void				Draw(sf::RenderTarget& target, sf::RenderStates states) const;


    private:
        SceneNode*			node_;
};

}