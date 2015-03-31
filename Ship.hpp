#ifndef CRANK_SHIP_HPP
#define CRANK_SHIP_HPP

#include "Entity.hpp"
#include "Command.hpp"
#include "ResourceIdentifiers.hpp"
#include "Projectile.hpp"
#include "TextNode.hpp"

#include <SFML/Graphics/Sprite.hpp>


class Ship : public Entity
{
    public:
        enum Type
        {
            Eagle,
            Raptor,
            Avenger,
            TypeCount
        };


    public:
                                Ship(Type type, const TextureManager& textures);

        virtual unsigned int    getCategory() const;
        virtual sf::FloatRect   getBoundingRect() const;
        virtual bool            isMarkedForRemoval() const;
        bool                    isAllied() const;
        float                   getMaxSpeed() const;

        void                    increaseFireRate();
        void                    increaseSpread();
        void                    collectMissiles(unsigned int count);

        void                    fire();
        void                    launchMissile();


    private:
        virtual void            drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
        virtual void            updateCurrent(sf::Time frameTime, CommandQueue& commands);
        void                    updateMovementPattern(sf::Time frameTime);
        void                    checkPickupDrop(CommandQueue& commands);
        void                    checkProjectileLaunch(sf::Time frameTime, CommandQueue& commands);

        void                    craateBullets(SceneNode& node, const TextureManager& textures) const;
        void                    createProjectile(SceneNode& node, Projectile::Type type, float xOffset, float yOffset, const TextureManager& textures) const;
        void                    createPickup(SceneNode& node, const TextureManager& textures) const;

        void                    updateTexts();


    private:
        Type                    mType;
        sf::Sprite              mSprite;
        Command                 mFireCommand;
        Command                 mMissileCommand;
        sf::Time                mFireCountdown;
        bool                    mIsFiring;
        bool                    mIsLaunchingMissile;
        bool                    mIsMarkedForRemoval;

        int                     mFireRateLevel;
        int                     mSpreadLevel;
        int                     mMissileAmmo;

        Command                 mDropPickupCommand;
        float                   mTravelledDistance;
        std::size_t             mDirectionIndex;
        TextNode*               mHealthDisplay;
        TextNode*               mMissileDisplay;
};

#endif // CRANK_SHIP_HPP
