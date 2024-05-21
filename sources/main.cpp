#include "ants.hpp"
#include "drawing.hpp"
#include <SFML/Graphics.hpp>

// for testing, to be removed
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>

int main()
{
  kape::Ants ants{};
  kape::Window window{700u, 600u};

  kape::Anthill anthill{kape::Vector2d{0.3, 0.}, 0.05};
  kape::Pheromones ph_anthill{kape::Pheromones::Type::TO_ANTHILL};
  kape::Pheromones ph_food{kape::Pheromones::Type::TO_FOOD};
  kape::Obstacles obs{};
  kape::Food food{};

  ants.addAntsAroundCircle(anthill.getCircle(), 50);

  // map walls
  obs.addObstacle(kape::Rectangle{kape::Vector2d{-2, +1}, 4, 0.02});
  obs.addObstacle(kape::Rectangle{kape::Vector2d{-2, +1}, 0.02, 2});
  obs.addObstacle(kape::Rectangle{kape::Vector2d{+2, +1}, 0.02, 2});
  obs.addObstacle(kape::Rectangle{kape::Vector2d{-2, -1}, 4, 0.02});

  // obstacles
  obs.addObstacle(kape::Rectangle{kape::Vector2d{-0.5, -0.5}, 0.5, 0.2});

  food.generateFoodInCircle(kape::Circle{kape::Vector2d{0., 0.5}, 0.1}, 500, obs);
  food.generateFoodInCircle(kape::Circle{kape::Vector2d{-1.2, 0.3}, 0.1}, 500, obs);
  food.generateFoodInCircle(kape::Circle{kape::Vector2d{1.3, -0.4}, 0.1}, 500, obs);
  food.generateFoodInCircle(kape::Circle{kape::Vector2d{-0.5, -0.85}, 0.1}, 500, obs);

  std::vector<long long int> t_count;

  while (window.isOpen()) {
    ants.update(food, ph_anthill, ph_food, anthill, obs);
    ph_anthill.updateParticlesEvaporation();
    ph_food.updateParticlesEvaporation();

    auto start{std::chrono::high_resolution_clock::now()};
    window.clear(sf::Color::Black);

    window.loadForDrawing(ph_anthill);
    window.loadForDrawing(ph_food);

    window.loadForDrawing(food);
    window.drawLoaded();

    window.draw(ants);

    // for (auto const& ant : ants) {
    //   std::array<kape::Circle, 3> circles_of_vision;
    //   ant.calculateCirclesOfVision(circles_of_vision);
    //   window.draw(circles_of_vision[0], sf::Color::Blue);
    //   window.draw(circles_of_vision[1], sf::Color::Blue);
    //   window.draw(circles_of_vision[2], sf::Color::Blue);
    // }
    window.draw(anthill);

    window.draw(obs, sf::Color::Yellow);

    window.display();

    window.inputHandling();
    t_count.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now() - start)
                          .count());
  }

  std::cout << std::accumulate(t_count.begin(), t_count.end(), 0.)
                   / t_count.size()
            << "\n";

  return 0;
}