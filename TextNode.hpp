#ifndef CRANK_TEXTNODE_HPP
#define CRANK_TEXTNODE_HPP

#include "SceneNode.hpp"
#include "ResourceManager.hpp"
#include "ResourceIdentifiers.hpp"

class TextNode : public SceneNode
{
    public:
        explicit        TextNode(const FontManager& fonts, const std::string& text);
        void            setString(const std::string& text);


    private:
        virtual void    drawCurrent(sf::RenderTarget& target,
                            sf::RenderStates states const);


    private:
        sf::Text        mText;
};

#endif // CRANK_TEXTNODE_HPP
