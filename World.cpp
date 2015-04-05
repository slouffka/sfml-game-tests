#include "World.hpp"
#include "Projectile.hpp"
#include "Pickup.hpp"
#include "Foreach.hpp"
#include "TextNode.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

#include <algorithm>
#include <cmath>
#include <limits>


World::World(sf::RenderWindow& window, FontManager& fonts)
: mWindow(window)
, mWorldView(window.getDefaultView())
, mFonts(fonts)
, mTextures()
, mSceneGraph()
, mSceneLayers()
, mWorldBounds(0.f, 0.f, mWorldView.getSize().x, 2000.f)
, mSpawnPosition(mWorldView.getSize().x / 2.f, mWorldBounds.height - mWorldView.getSize().y / 2.f)
, mScrollSpeed(-50.f)
, mPlayerShip(nullptr)
, mEnemySpawnPoints()
, mActiveEnemies()
{
    loadTextures();
    buildScene();

    // Prepare the view
    mWorldView.setCenter(mSpawnPosition);
}

void World::update(sf::Time frameTime)
{
    // Scroll the world
    mWorldView.move(0.f, mScrollSpeed * frameTime.asSeconds());
    mPlayerShip->setVelocity(0.f, 0.f);

    // Setup commands to destroy entities, and guide missiles
    destroyEntitiesOutsideView();
    guideMissiles();

    // Forward commands to scene graph, adapt velocity (scrolling, diagonal correction)
    while (!mCommandQueue.isEmpty())
        mSceneGraph.onCommand(mCommandQueue.pop(), frameTime);
    adaptPlayerVelocity();

    // Collision detection and response (may destroy entities)
    handleCollisions();

    // Remove all destroyed entities, create new ones
    mSceneGraph.removeWrecks();
    spawnEnemies();

    // Regular update step, adapt position (correct if outside view)
    mSceneGraph.update(frameTime, mCommandQueue);
    adaptPlayerPosition();
}

void World::draw()
{
    mWindow.setView(mWorldView);
    mWindow.draw(mSceneGraph);
}

CommandQueue& World::getCommandQueue()
{
    return mCommandQueue;
}

bool World::hasAlivePlayer() const
{
    return !mPlayerShip->isMarkedForRemoval();
}

bool World::hasPlayerReachedEnd() const
{
    return !mWorldBounds.contains(mPlayerShip->getPosition());
}

void World::loadTextures()
{
    mTextures.load(Textures::Eagle, "res/textures/eagle.png");
    mTextures.load(Textures::Raptor, "res/textures/raptor.png");
    mTextures.load(Textures::Avenger, "res/textures/avenger.png");
    mTextures.load(Textures::Background, "res/textures/background.png");

    mTextures.load(Textures::Bullet, "res/textures/bullet.png");
    mTextures.load(Textures::Missile, "res/textures/missile.png");

    mTextures.load(Textures::HealthRefill, "res/textures/health-refill.png");
    mTextures.load(Textures::MissileRefill, "res/textures/missile-refill.png");
    mTextures.load(Textures::FireSpread, "res/textures/fire-spread.png");
    mTextures.load(Textures::FireRate, "res/textures/fire-rate.png");
}

void World::adaptPlayerPosition()
{
    // Keep player's positioin inside the screen bounds, at least borderDistance units from border
    sf::FloatRect viewBounds(mWorldView.getCenter() - mWorldView.getSize() / 2.f, mWorldView.getSize());
    const float borderDistance = 40.f;

    sf::Vector2f position = mPlayerShip->getPosition();
    position.x = std::max(position.x, viewBounds.left + borderDistance);
    position.x = std::min(position.x, viewBounds.left + viewBounds.width - borderDistance);
    position.y = std::max(position.y, viewBounds.top + borderDistance);
    position.y = std::min(position.y, viewBounds.top + viewBounds.height - borderDistance);
    mPlayerShip->setPosition(position);
}

void World::adaptPlayerVelocity()
{
    sf::Vector2f velocity = mPlayerShip->getVelocity();

    // If moving diagonally, reduce velocity (to have always same velocity)
    if (velocity.x != 0.f && velocity.y != 0.f)
        mPlayerShip->setVelocity(velocity / std::sqrt(2.f));

    // Add scrolling velocity
    mPlayerShip->accelerate(0.f, mScrollSpeed);
}

bool matchesCategories(SceneNode::Pair& colliders, Category::Type type1, Category::Type type2)
{
    unsigned int category1 = colliders.first->getCategory();
    unsigned int category2 = colliders.second->getCategory();

    // Make sure first pair entry has category type1 and second has type2
    if (type1 & category1 && type2 & category2)
    {
        return true;
    }
    else if (type1 & category2 && type2 & category1)
    {
        std::swap(colliders.first, colliders.second);
        return true;
    }
    else
    {
        return false;
    }

}

void World::handleCollisions()
{
    std::set<SceneNode::Pair> collisionPairs;
    mSceneGraph.checkSceneCollision(mSceneGraph, collisionPairs);

    FOREACH(SceneNode::Pair pair, collisionPairs)
    {
        if (matchesCategories(pair, Category::PlayerShip, Category::EnemyShip))
        {
            auto& player = static_cast<Ship&>(*pair.first);
            auto& enemy = static_cast<Ship&>(*pair.second);

            // Collision: Player damage = enemy's remaining HP
            player.damage(enemy.getHitpoints());
            enemy.destroy();
        }

        else if (matchesCategories(pair, Category::PlayerShip, Category::Pickup))
        {
            auto& player = static_cast<Ship&>(*pair.first);
            auto& pickup = static_cast<Pickup&>(*pair.second);

            // Apply pickup effect to player, destroy projectile
            pickup.apply(player);
            pickup.destroy();
        }

        else if (matchesCategories(pair, Category::EnemyShip, Category::AlliedProjectile)
            || matchesCategories(pair, Category::PlayerShip, Category::EnemyProjectile))
        {
            auto& ship = static_cast<Ship&>(*pair.first);
            auto& projectile = static_cast<Projectile&>(*pair.second);

            // Apply projectile damage to ship, destroy projectile
            ship.damage(projectile.getDamage());
            projectile.destroy();
        }
    }
}

void World::buildScene()
{
    // Initialize the different layers
    for (std::size_t i = 0; i < LayerCount; ++i)
    {
        Category::Type category = (i == Space) ? Category::SceneSpaceLayer : Category::None;

        SceneNode::Ptr layer(new SceneNode(category));
        mSceneLayers[i] = layer.get();

        mSceneGraph.attachChild(std::move(layer));
    }

    // Prepare the tiled background
    sf::Texture& texture = mTextures.get(Textures::Background);
    sf::IntRect textureRect(mWorldBounds);
    texture.setRepeated(true);

    // Add background sprite to the screen
    std::unique_ptr<SpriteNode> backgroundSprite(new SpriteNode(texture, textureRect));
    backgroundSprite->setPosition(mWorldBounds.left, mWorldBounds.top);
    mSceneLayers[Background]->attachChild(std::move(backgroundSprite));

    // Add player's ship
    std::unique_ptr<Ship> player(new Ship(Ship::Eagle, mTextures, mFonts));
    mPlayerShip = player.get();
    mPlayerShip->setPosition(mSpawnPosition);
    mSceneLayers[Space]->attachChild(std::move(player));

    // Add enemy ship
    addEnemies();
}

void World::addEnemies()
{
    addEnemy(Ship::Raptor,    0.f,  500.f);
    addEnemy(Ship::Raptor,    0.f, 1000.f);
    addEnemy(Ship::Raptor, +100.f, 1100.f);
    addEnemy(Ship::Raptor, -100.f, 1100.f);
    addEnemy(Ship::Avenger, -70.f, 1400.f);
    addEnemy(Ship::Avenger, -70.f, 1600.f);
    addEnemy(Ship::Avenger,  70.f, 1400.f);
    addEnemy(Ship::Avenger,  70.f, 1600.f);

    // Sort all enemies according to their y value, such that lower enemies are checked first for spawning
    std::sort(mEnemySpawnPoints.begin(), mEnemySpawnPoints.end(), [] (SpawnPoint lhs, SpawnPoint rhs)
    {
        return lhs.y < rhs.y;
    });
}

void World::addEnemy(Ship::Type type, float relX, float relY)
{
    SpawnPoint spawn(type, mSpawnPosition.x + relX, mSpawnPosition.y - relY);
    mEnemySpawnPoints.push_back(spawn);
}

void World::spawnEnemies()
{
    // Spawn all enemies entering the view area (including distance) this frame
    while (!mEnemySpawnPoints.empty()
        && mEnemySpawnPoints.back().y > getBattlefieldBounds().top)
    {
        SpawnPoint spawn = mEnemySpawnPoints.back();

        std::unique_ptr<Ship> enemy(new Ship(spawn.type, mTextures, mFonts));
        enemy->setPosition(spawn.x, spawn.y);
        enemy->setRotation(180.f);

        mSceneLayers[Space]->attachChild(std::move(enemy));

        // Enemy is spawned, remove from the list to spawn
        mEnemySpawnPoints.pop_back();
    }
}

void World::destroyEntitiesOutsideView()
{
    Command command;
    command.category = Category::Projectile | Category::EnemyShip;
    command.action = derivedAction<Entity>([this] (Entity& e, sf::Time)
    {
        if (!getBattlefieldBounds().intersects(e.getBoundingRect()))
            e.destroy();
    });

    mCommandQueue.push(command);
}

void World::guideMissiles()
{
    // Setup command that stores all enemies in mActiveEnemies
    Command enemyCollector;
    enemyCollector.category = Category::EnemyShip;
    enemyCollector.action = derivedAction<Ship>([this] (Ship& enemy, sf::Time)
    {
        if (!enemy.isDestroyed())
            mActiveEnemies.push_back(&enemy);
    });

    // Setup command that guides all missiles to the eemy which is currently closest to the player
    Command missileGuider;
    missileGuider.category = Category::AlliedProjectile;
    missileGuider.action = derivedAction<Projectile>([this] (Projectile& missile, sf::Time)
    {
        // Ignore unguided bullets
        if (!missile.isGuided())
            return;

        float minDistance = std::numeric_limits<float>::max();
        Ship* closestEnemy = nullptr;

        // Find closest enemy
        FOREACH(Ship* enemy, mActiveEnemies)
        {
            float enemyDistance = distance(missile, *enemy);

            if (enemyDistance < minDistance)
            {
                closestEnemy = enemy;
                minDistance = enemyDistance;
            }
        }

        if (closestEnemy)
            missile.guideTowards(closestEnemy->getWorldPosition());
    });

    // Push commands, reset active enemies
    mCommandQueue.push(enemyCollector);
    mCommandQueue.push(missileGuider);
    mActiveEnemies.clear();
}

sf::FloatRect World::getViewBounds() const
{
    return sf::FloatRect(mWorldView.getCenter() - mWorldView.getSize() / 2.f, mWorldView.getSize());
}

sf::FloatRect World::getBattlefieldBounds() const
{
    // Return view bounds + some area at top, where enemies spawn
    sf::FloatRect bounds = getViewBounds();
    bounds.top -= 100.f;
    bounds.height += 100.f;

    return bounds;
}
