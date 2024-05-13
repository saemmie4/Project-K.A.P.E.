#include "ants.hpp"
#include "drawing.hpp"
#include <SFML/Graphics.hpp>

// for testing, to be removed
#include <chrono>
#include <cmath>
#include <iostream>

int main()
{
  kape::Ant ant{{0., 0.}, {0., 0.}};
  kape::Circle circle{{0., 0.}, 0.01};
  kape::Rectangle rectangle{{0., 0.02}, 0.01, 0.02};
  kape::Window window{700u, 600u};

  auto start_time{std::chrono::high_resolution_clock::now()};

  while (window.isOpen()) {
    std::chrono::duration<double> duration{
        std::chrono::duration_cast<std::chrono::milliseconds>(
            (std::chrono::high_resolution_clock::now() - start_time))};
    double seconds_passed{duration.count()};

    circle.setCircleCenter(kape::Vector2d{0.1 * sin(seconds_passed), 0.});

    window.clear(sf::Color::Black);
    // window.draw(ant);
    sf::Color col =
        (kape::DoShapesIntersect(circle, rectangle) ? sf::Color::Red
                                                    : sf::Color::Green);
    window.draw(circle, col);
    window.draw(rectangle, col);
    window.display();
    window.pullAllEvents();
  }

  return 0;
}