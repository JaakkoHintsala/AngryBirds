#include "game.hpp"
#include <string>

Game::Game() : window_(sf::VideoMode(viewwidth, viewheight), "Angry Birds")
{
    window_.setFramerateLimit(framerate);
}

void Game::LoadLevel(std::string filename)
{
    victory_achieved_ = 0; // Reset victory sound status
    std::ifstream file(filename);
    if (file.rdstate() & (file.failbit | file.badbit))
    {
        std::cerr << "Level loading failed for file: " << filename << std::endl;
    }
    else
    {
        current_level_file_name_ = filename;
        current_level_ = Level(file);
    }
}

void Game::LoadIcon()
{
    sf::Image icon;
    icon.loadFromFile("resources/images/bird.png");
    sf::Vector2u size = icon.getSize();
    window_.setIcon(size.x, size.y, icon.getPixelsPtr());
}

void Game::SaveLevel()
{
    std::ofstream file = utils::OpenFileSafe("testi");
    current_level_.SaveState(file);
}

void Game::Start()
{
    victory_achieved_ = 0;
    sf::SoundBuffer victory_sound_buffer;
    victory_sound_buffer.loadFromFile("resources/sounds/victory_royale.wav");
    sf::Sound victory_sound;
    victory_sound.setBuffer(victory_sound_buffer);
    victory_sound.setVolume(20);

    sf::SoundBuffer bg_music_buffer;
    bg_music_buffer.loadFromFile("resources/sounds/angry_birds_bg_music.wav");
    sf::Sound bg_music;
    bg_music.setBuffer(bg_music_buffer);
    bg_music.setVolume(1);
    bg_music.setLoop(true);
    bg_music.play();

    background_texture_.loadFromFile("./resources/images/bg_img.jpeg");
    background_texture_.setRepeated(true);
    bg_sprite_.setTexture(background_texture_);
    bg_sprite_.setTextureRect({0, 0, viewwidth * 10, viewheight * 10});
    bg_sprite_.setScale(1, 3);
    bg_sprite_.setOrigin(0, 2 * background_texture_.getSize().y - 450); // background_texture_.getSize().y - viewheight - 25

    sf::View game_view(window_.getDefaultView());

    MainMenu main_menu = MainMenu();

    LevelSelector level_selector = LevelSelector();

    PauseMenu pause_menu = PauseMenu();

    LevelEndMenu end_screen = LevelEndMenu(0);

    HighScores high_scores = HighScores(current_level_);
    high_scores.Close();

    auto IsMenuOpen = [&]()
    {
        return main_menu.IsOpen() || level_selector.IsOpen() || pause_menu.IsOpen() || end_screen.IsOpen() || high_scores.IsOpen();
    };

    sf::Font font;
    font.loadFromFile("resources/fonts/Raleway-Medium.ttf");
    sf::Text score;
    score.setFont(font);
    score.setFillColor(sf::Color::White);
    score.setOutlineColor(sf::Color::Black);
    score.setOutlineThickness(3.0f);
    score.setString(std::string("Score: ") + std::to_string(current_level_.GetScore()));
    score.setCharacterSize(40);
    sf::Text high_score;
    high_score.setFont(font);
    high_score.setFillColor(sf::Color::White);
    high_score.setOutlineColor(sf::Color::Black);
    high_score.setOutlineThickness(3.0f);
    high_score.setString(std::string("High Score: ") + std::to_string(std::get<1>(current_level_.GetHighScore())));
    high_score.setCharacterSize(40);
    sf::RectangleShape pause(sf::Vector2f(100.0f, 100.0f));
    sf::Texture pauseImage;
    pauseImage.loadFromFile("resources/images/pause.png");
    pause.setTexture(&pauseImage);
    sf::RectangleShape obj_images[4];
    sf::Texture obj_textures[4];
    obj_textures[0].loadFromFile("resources/images/bird.png");
    obj_textures[1].loadFromFile("resources/images/bird2.png");
    obj_textures[2].loadFromFile("resources/images/bird3.png");
    obj_textures[3].loadFromFile("resources/images/pig.png");
    sf::Text obj_indicators[4];
    for (int i = 0; i < 4; i++)
    {
        obj_images[i].setSize(sf::Vector2f(100.0f, 100.0f));
        obj_images[i].setTexture(&obj_textures[i]);
        obj_indicators[i].setFont(font);
        obj_indicators[i].setFillColor(sf::Color::White);
        obj_indicators[i].setCharacterSize(20);
        obj_indicators[i].setOutlineColor(sf::Color::Black);
        obj_indicators[i].setOutlineThickness(3.0f);
    }

    bool settled = false;            // Is the world in a settled state (nothing is moving)
    bool has_just_settled = settled; // Has the world settled on the previous simulation step
    float direction = 0;             // Direction of the aiming arrow in degrees
    float power = 0;                 // Power of the aiming arrow (0-100)
    while (window_.isOpen())
    {
        sf::Vector2f mouse_position = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
        sf::Event event;
        while (window_.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::EventType::Closed:
                window_.close();
                break;
            case sf::Event::EventType::MouseButtonPressed:

                if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
                {
                    sf::Vector2f pause_position = pause.getPosition();
                    if (mouse_position.x < pause_position.x + 100 && mouse_position.y < pause_position.y + 100 && !IsMenuOpen())
                    {

                        game_view = window_.getDefaultView();
                        window_.setView(game_view);
                        pause_menu.Open();
                    }
                    else if (current_level_.GetBird()->IsThrown() && !IsMenuOpen())
                    {
                        current_level_.GetBird()->NewPower();
                    }
                    else if (settled && !IsMenuOpen() && power != 0)
                    {
                        float x = cos(utils::DegreesToRadians(direction)) * power / 20;
                        float y = sin(utils::DegreesToRadians(direction)) * power / 20;
                        current_level_.ThrowBird(0, b2Vec2(x, y));
                    }
                }
                break;
            case sf::Event::EventType::Resized:
            {
                float width = static_cast<float>(event.size.width);
                float height = static_cast<float>(event.size.height);
                float aspect_ratio = width / height;
                sf::View default_view = window_.getDefaultView();
                sf::Vector2f default_size = default_view.getSize();
                float default_aspect_ratio = default_size.x / default_size.y;
                float k;

                if (aspect_ratio > default_aspect_ratio)
                {
                    k = default_size.y / height;
                }
                else
                {
                    k = default_size.x / width;
                }

                game_view.setSize(k * width, k * height);
                game_view.setCenter(default_view.getCenter());
                break;
            }
            case sf::Event::TextEntered:
            {
                if (main_menu.IsOpen())
                {
                    main_menu.ChangeNickname(event.text.unicode);
                }
            }
            case sf::Event::EventType::KeyPressed:

                switch (event.key.code)
                {
                case sf::Keyboard::Up:
                    if (settled && game_view.getCenter().y > window_.getDefaultView().getCenter().y - 1500) // Bounded from top
                        game_view.move(0, -10);
                    break;

                case sf::Keyboard::Down:
                    if (settled && game_view.getCenter().y < window_.getDefaultView().getCenter().y) //  Bounded from below
                        game_view.move(0, 10);
                    break;

                case sf::Keyboard::Left:
                    if (settled && game_view.getCenter().x > window_.getDefaultView().getCenter().x) // Bounded from left
                        game_view.move(-10, 0);
                    break;

                case sf::Keyboard::Right:
                    if (settled && game_view.getCenter().x < window_.getDefaultView().getCenter().x + 1500) // Bounded from right
                        game_view.move(10, 0);
                    break;
                case sf::Keyboard::Space:
                    if (settled) // Move the view back to its original position
                        game_view.move(window_.getDefaultView().getCenter().x - game_view.getCenter().x, window_.getDefaultView().getCenter().y - game_view.getCenter().y);
                    break;
                case sf::Keyboard::Escape:
                    if (level_selector.IsOpen())
                    {
                        main_menu.Open();
                        level_selector.Close();
                    }
                    else
                    {
                        game_view = window_.getDefaultView();
                        window_.setView(game_view);
                        pause_menu.Open();
                    }
                    break;

                default:
                    break;
                }
            }
        }
        window_.clear(sf::Color::Blue);
        window_.draw(bg_sprite_);
        if (high_scores.IsOpen())
        {
            high_scores.SetLevel(current_level_);
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                if (mouse_position.x >= 7 && mouse_position.x <= 183 && mouse_position.y >= 120 && mouse_position.y <= 180)
                {
                    high_scores.Close();
                }
                else if (mouse_position.x >= 1200 && mouse_position.x <= 1300 && mouse_position.y >= 300 && mouse_position.y <= 360)
                {
                    LoadLevel("resources/levels/level1.ab");
                }
                else if (mouse_position.x >= 1200 && mouse_position.x <= 1300 && mouse_position.y >= 400 && mouse_position.y <= 460)
                {
                    LoadLevel("resources/levels/level2.ab");
                }
                else if (mouse_position.x >= 1200 && mouse_position.x <= 1300 && mouse_position.y >= 500 && mouse_position.y <= 560)
                {
                    LoadLevel("resources/levels/level3.ab");
                }
            }
            high_scores.Draw(window_);
        }
        else if (main_menu.IsOpen())
        {
            level_selector.Open();
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {

                if (mouse_position.x >= 1006 && mouse_position.x <= 1160 && mouse_position.y >= 220 && mouse_position.y <= 300)
                {
                    main_menu.Close();
                }
                else if (mouse_position.x >= 1007 && mouse_position.x <= 1448 && mouse_position.y >= 320 && mouse_position.y <= 400)
                {
                    high_scores.Open();
                }
                else if (mouse_position.x >= 1006 && mouse_position.x <= 1136 && mouse_position.y >= 420 && mouse_position.y <= 480)
                {
                    window_.close();
                }
            }
            main_menu.Draw(window_);
        }
        else if (level_selector.IsOpen())
        {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                if (mouse_position.x >= 7 && mouse_position.x <= 183 && mouse_position.y >= 120 && mouse_position.y <= 180)
                {
                    main_menu.Open();
                    level_selector.Close();
                }
                else if (mouse_position.x >= 100 && mouse_position.x <= 400 && mouse_position.y >= 400 && mouse_position.y <= 680)
                {
                    LoadLevel("resources/levels/level1.ab");
                    end_screen.SetLevel(1);
                    pause_menu.Close();
                    end_screen.Close();
                    level_selector.Close();
                }
                else if (mouse_position.x >= 600 && mouse_position.x <= 900 && mouse_position.y >= 400 && mouse_position.y <= 680)
                {
                    LoadLevel("resources/levels/level2.ab");
                    end_screen.SetLevel(2);
                    pause_menu.Close();
                    end_screen.Close();
                    level_selector.Close();
                }
                else if (mouse_position.x >= 1100 && mouse_position.x <= 1500 && mouse_position.y >= 400 && mouse_position.y <= 680)
                {
                    LoadLevel("resources/levels/level3.ab");
                    end_screen.SetLevel(3);
                    pause_menu.Close();
                    end_screen.Close();
                    level_selector.Close();
                }
            }
            level_selector.Draw(window_);
        }
        else if (pause_menu.IsOpen())
        {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                if (mouse_position.x >= 1000 && mouse_position.x <= 1300 && mouse_position.y >= 200 && mouse_position.y <= 280)
                {
                    pause_menu.Close();
                }
                else if (mouse_position.x >= 1000 && mouse_position.x <= 1400 && mouse_position.y >= 300 && mouse_position.y <= 380)
                {
                    main_menu.Open();
                }
            }
            pause_menu.Draw(window_);
        }
        else if (end_screen.IsOpen())
        {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                if (current_level_.GetLevelNumber() == 3)
                {
                    if (mouse_position.x >= 600 && mouse_position.x <= 775 && mouse_position.y >= 530 && mouse_position.y <= 645)
                    {
                        end_screen.Close();
                        main_menu.Open();
                    }
                    else if (mouse_position.x >= 864 && mouse_position.x <= 1025 && mouse_position.y >= 520 && mouse_position.y <= 645)
                    {
                        //  int next_level = std::min(current_level_.GetLevelNumber() + 1, 3);
                        LoadLevel("resources/levels/level" + std::to_string(current_level_.GetLevelNumber()) + ".ab");
                        end_screen.SetLevel(current_level_.GetLevelNumber());
                        end_screen.Close();
                    }
                }
                else
                {
                    if (mouse_position.x >= 520 && mouse_position.x <= 675 && mouse_position.y >= 515 && mouse_position.y <= 597)
                    {
                        end_screen.Close();
                        main_menu.Open();
                    }
                    else if (mouse_position.x >= 925 && mouse_position.x <= 1080 && mouse_position.y >= 515 && mouse_position.y <= 635)
                    {
                        int next_level = current_level_.GetLevelNumber() + 1;
                        LoadLevel("resources/levels/level" + std::to_string(next_level) + ".ab");
                        end_screen.SetLevel(next_level);
                        end_screen.Close();
                    }
                    else if (mouse_position.x >= 715 && mouse_position.x <= 879 && mouse_position.y >= 515 && mouse_position.y <= 640)
                    {
                        LoadLevel("resources/levels/level" + std::to_string(current_level_.GetLevelNumber()) + ".ab");
                        end_screen.SetLevel(current_level_.GetLevelNumber());
                        end_screen.Close();
                    }
                }
            }
            game_view = window_.getDefaultView();
            window_.setView(game_view);
            score.setPosition(window_.mapPixelToCoords(sf::Vector2i(window_.getSize().x * 0.7, 0)));
            score.setString(std::string("Score: ") + std::to_string(current_level_.GetScore()));
            high_score.setPosition(window_.mapPixelToCoords(sf::Vector2i(window_.getSize().x * 0.7, 40)));
            high_score.setString(std::string("High Score: ") + std::to_string(std::get<1>(current_level_.GetHighScore())));
            window_.draw(score);
            window_.draw(high_score);
            current_level_.DrawLevel(window_);
            end_screen.Draw(window_);
        }
        else
        {
            window_.setView(game_view);
            current_level_.GetBird()->UsePower();

            sf::Vector2f bird_position = utils::B2ToSfCoords(current_level_.GetBird()->GetBody()->GetPosition());
            sf::Vector2f default_center = window_.getDefaultView().getCenter();

            // Follow bird when thrown
            if (!settled)
            {
                // Used std min for the y since sfml coordinates are from top left downwards
                game_view.setCenter(std::min(std::max(bird_position.x, window_.getDefaultView().getCenter().x), viewwidth * 1.f), std::min(bird_position.y, default_center.y));
            }

            current_level_.GetWorld()
                ->Step(time_step, velocity_iterations, position_iterations);
            bool prev_settled = settled;

            settled = !current_level_.DrawLevel(window_);
            has_just_settled = settled && !prev_settled;
            // Draw the aiming arrow
            std::tuple<float, float> tuple = current_level_.DrawArrow(window_);
            // Update arrow direction and power
            direction = std::get<0>(tuple);
            power = std::get<1>(tuple);
            bool bird_has_been_thrown = current_level_.GetBird()->IsThrown();
            // Reset the bird and view when world settles after a throw
            if (has_just_settled && bird_has_been_thrown && current_level_.GetBird())
            {
                current_level_.ResetBird();
                // Update bird_position after reset
                bird_position = utils::B2ToSfCoords(current_level_.GetBird()->GetBody()->GetPosition());
                game_view.setCenter(std::max(bird_position.x, window_.getDefaultView().getCenter().x), std::min(bird_position.y, default_center.y));
            }
            score.setPosition(window_.mapPixelToCoords(sf::Vector2i(static_cast<int>(window_.getSize().x * 0.7), 0)));
            score.setString(std::string("Score: ") + std::to_string(current_level_.GetScore()));
            high_score.setPosition(window_.mapPixelToCoords(sf::Vector2i(static_cast<int>(window_.getSize().x * 0.7), 40)));
            high_score.setString(std::string("High Score: ") + std::to_string(std::get<1>(current_level_.GetHighScore())));
            pause.setPosition(window_.mapPixelToCoords(sf::Vector2i(0, 0)));
            for (int i = 0; i < 4; i++)
            {
                if (current_level_.CountBirdTypes()[i] > 0 || i == 3)
                {
                    obj_images[i].setPosition(window_.mapPixelToCoords(sf::Vector2i(200 + i * 100, 0)));
                    obj_indicators[i].setPosition(window_.mapPixelToCoords(sf::Vector2i(250 + i * 100, 100)));
                    if (i != 3)
                        obj_indicators[i].setString(std::to_string((current_level_.CountBirdTypes()[i])));
                    else
                    {
                        obj_indicators[i].setString(std::to_string(current_level_.CountPigs()));
                    }
                    window_.draw(obj_images[i]);
                    window_.draw(obj_indicators[i]);
                }
            }

            window_.draw(score);
            window_.draw(high_score);

            window_.draw(pause);

            /*
            if (has_just_settled)
            {
                // Save world to file
                SaveLevel();
            }
            */

            if (current_level_.IsLevelEnded() && settled)
            {
                if (victory_achieved_ == 0)
                {
                    victory_sound.play();
                    victory_achieved_ = 1;
                }
                // Save highscore and Open endscreen
                int current = std::get<1>(current_level_.GetHighScore());
                std::list<std::tuple<std::string, int>> high_scores = current_level_.UpdateHighScore(main_menu.GetNickname());
                high_scores.sort(utils::CmpHighScore);
                UpdateSavedHighScore(high_scores);
                if (current != std::get<1>(current_level_.GetHighScore()))
                {
                    end_screen.ShowHighScore();
                }
                end_screen.SelectStars(current_level_.GetStars());
                end_screen.Open();
            }
        }

        window_.display();
    }
}

void Game::UpdateSavedHighScore(std::list<std::tuple<std::string, int>> high_scores)
{
    const int line_to_update = 2;
    // Read all lines to memory, this shouldn't be a problem since save files are quite small
    std::ifstream input(current_level_file_name_);
    std::vector<std::string> lines;
    std::string line;
    while (input.good())
    {
        std::getline(input, line);
        if (line != "")
        {
            lines.push_back(line);
        }
    }

    input.close();

    std::stringstream high_scores_stream;

    for (auto high_score : high_scores)
    {
        high_scores_stream << std::get<0>(high_score) << ":" << std::get<1>(high_score) << ";";
    }
    // Replace second row
    lines[1] = high_scores_stream.str();

    std::ofstream output(current_level_file_name_);
    for (auto line : lines)
    {
        output << line << std::endl;
    }
    output.close();
}