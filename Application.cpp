#include "Application.hpp"
#include "Utility.hpp"
#include "State.hpp"
#include "StateIdentifiers.hpp"
#include "TitleState.hpp"
#include "GameState.hpp"
#include "MultiplayerGameState.hpp"
#include "MenuState.hpp"
#include "PauseState.hpp"
#include "SettingsState.hpp"
#include "GameOverState.hpp"


const sf::Time Application::TimePerFrame = sf::seconds(1.f / 60.f);

Application::Application()
: mWindow(sf::VideoMode(1024, 768), "Crank 08-Gameplay", sf::Style::Close)
, mTextures()
, mFonts()
, mMusic()
, mSounds()
, mKeyBinding1(1)
, mKeyBinding2(2)
, mStateStack(State::Context(mWindow, mTextures, mFonts, mMusic, mSounds, mKeyBinding1, mKeyBinding2))
, mStatisticsText()
, mStatisticsUpdateTime()
, mStatisticsNumFrames(0)
{
    mWindow.setKeyRepeatEnabled(false);
    // mWindow.setVerticalSyncEnabled(true);

    mFonts.load(Fonts::Main, "res/fonts/sansation.ttf");

    mTextures.load(Textures::TitleScreen,   "res/textures/title-screen.png");
    mTextures.load(Textures::Buttons,       "res/textures/buttons.png");

    mStatisticsText.setFont(mFonts.get(Fonts::Main));
    mStatisticsText.setPosition(10.f, 10.f);
    mStatisticsText.setCharacterSize(10u);

    registerStates();
    mStateStack.pushState(States::Title);

    mMusic.setVolume(25.f);
}

void Application::run()
{
    // floating time step, most smooth rendering but can't
    // guarantee repeated results for the same scene
    /* sf::Clock clock;
    sf::Time dt = sf::Time::Zero;

    while (mWindow.isOpen())
    {
        dt = clock.restart();

        processInput();
        update(dt);
        updateStatistics(dt);
        render();
    } */

    // fixed time step, jagged rendering but accurate physics
    sf::Clock clock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;

    while (mWindow.isOpen())
    {
        sf::Time dt = clock.restart();
        timeSinceLastUpdate += dt;
        while (timeSinceLastUpdate > TimePerFrame)
        {
            timeSinceLastUpdate -= TimePerFrame;

            processInput();
            update(TimePerFrame);

            // Check inside this loop, because stack might be empty before update() call
            if (mStateStack.isEmpty())
                mWindow.close();
        }

        updateStatistics(dt);
        render();
    }
}

void Application::processInput()
{
    sf::Event event;
    while (mWindow.pollEvent(event))
    {
        mStateStack.handleEvent(event);

        if (event.type == sf::Event::Closed)
            mWindow.close();
    }
}

void Application::update(sf::Time dt)
{
    mStateStack.update(dt);
}

void Application::render()
{
    mWindow.clear();

    mStateStack.draw();

    mWindow.setView(mWindow.getDefaultView());
    mWindow.draw(mStatisticsText);

    mWindow.display();
}

void Application::updateStatistics(sf::Time dt)
{
    mStatisticsUpdateTime += dt;
    mStatisticsNumFrames += 1;
    if (mStatisticsUpdateTime >= sf::seconds(1.0f))
    {
        mStatisticsText.setString(toString(mStatisticsNumFrames) + " fps");

        mStatisticsUpdateTime -= sf::seconds(1.0f);
        mStatisticsNumFrames = 0;
    }
}

void Application::registerStates()
{
    mStateStack.registerState<TitleState>(States::Title);
    mStateStack.registerState<MenuState>(States::Menu);
    mStateStack.registerState<GameState>(States::Game);
    mStateStack.registerState<MultiplayerGameState>(States::HostGame, true);
    mStateStack.registerState<MultiplayerGameState>(States::JoinGame, false);
    mStateStack.registerState<PauseState>(States::Pause);
    mStateStack.registerState<PauseState>(States::NetworkPause, true);
    mStateStack.registerState<SettingsState>(States::Settings);
    mStateStack.registerState<GameOverState>(States::GameOver, "Mission Failed!");
    mStateStack.registerState<GameOverState>(States::MissionSuccess, "Mission Successful!");
}
