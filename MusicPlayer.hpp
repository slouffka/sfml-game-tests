#ifndef CRANK_MUSICPLAYER_HPP
#define CRANK_MUSICPLAYER_HPP

#include "ResourceManager.hpp"
#include "ResourceIdentifiers.hpp"

#include <SFML/System/NonCopyable.hpp>
#include <SFML/Audio/Music.hpp>

#include <map>
#include <string>


class MusicPlayer : private sf::NonCopyable
{
    public:
        MusicPlayer();

        void                        play(Music::ID theme);
        void                        stop();

        void                        setPaused(bool paused);
        void                        setVolume(float volume);


    private:
        sf::Music                           mMusic;
        std::map<Music::ID, std::string>    mFilenames;
        float                               mVolume
};

#endif // CRANK_MUSICPLAYER_HPP
