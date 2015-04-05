#ifndef CRANK_WORLD_HPP
#define CRANK_WORLD_HPP

#include "ResourceManager.hpp"
#include "ResourceIdentifiers.hpp"
#include "SceneNode.hpp"
#include "SpriteNode.hpp"
#include "Ship.hpp"
#include "CommandQueue.hpp"
#include "Command.hpp"

#include <SFML/System/NonCopyable.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <queue>


// Forward declaration
namespace sf
{
    class RenderWindow;
}

class World : private sf::NonCopyable
{
    public:
        explicit                                World(sf::RenderWindow& window, FontManager& fonts);
        void                                    update(sf::Time frameTime);
        void                                    draw();

        CommandQueue&                           getCommandQueue();

        bool                                    hasAlivePlayer() const;
        bool                                    hasPlayerReachedEnd() const;


    private:
        void                                    loadTextures();
        void                                    adaptPlayerPosition();
        void                                    adaptPlayerVelocity();
        void                                    handleCollisions();

        void                                    buildScene();
        void                                    addEnemies();
        void                                    addEnemy(Ship::Type type, float relX, float relY);
        void                                    spawnEnemies();
        void                                    destroyEntitiesOutsideView();
        void                                    guideMissiles();
        sf::FloatRect                           getViewBounds() const;
        sf::FloatRect                           getBattlefieldBounds() const;


    private:
        enum Layer
        {
            Background,
            Space,
            LayerCount
        };

        struct SpawnPoint
        {
            SpawnPoint(Ship::Type type, float x, float y)
            : type(type)
            , x(x)
            , y(y)
            {
            }

            Ship::Type type;
            float x;
            float y;
        };


    private:
        sf::RenderWindow&                       mWindow;
        sf::View                                mWorldView;
        FontManager&                            mFonts;
        TextureManager                          mTextures;

        SceneNode                               mSceneGraph;
        std::array<SceneNode*, LayerCount>      mSceneLayers;
        CommandQueue                            mCommandQueue;

        sf::FloatRect                           mWorldBounds;
        sf::Vector2f                            mSpawnPosition;
        float                                   mScrollSpeed;
        Ship*                                   mPlayerShip;

        std::vector<SpawnPoint>                 mEnemySpawnPoints;
        std::vector<Ship*>                      mActiveEnemies;
};

#endif // CRANK_WORLD_HPP
