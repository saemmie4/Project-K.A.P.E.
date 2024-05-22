#include "environment.hpp"
#include "geometry.hpp"
#include "logger.hpp"
#include <algorithm> //for find_if and remove_if any_of
#include <cmath>
#include <fstream>
#include <numeric>   //for accumulate
#include <stdexcept> //invalid_argument
#include <vector>

#include <cassert>
// TODO:
//  - find a way to use algorithms in removeOneFoodParticleInCircle()

namespace kape {

// implementation of class Obstacles-----------------------------------

Obstacles::Obstacles()
{}

void Obstacles::addObstacle(Vector2d const& top_left_corner, double width,
                            double height)
{
  obstacles_vec_.push_back(Rectangle{top_left_corner, width, height});
}
void Obstacles::addObstacle(Rectangle const& obstacle)
{
  obstacles_vec_.push_back(obstacle);
}

bool Obstacles::anyObstaclesInCircle(Circle const& circle) const
{
  return std::any_of(obstacles_vec_.begin(), obstacles_vec_.end(),
                     [&circle](Rectangle const& obstacle) {
                       return doShapesIntersect(circle, obstacle);
                     });
}

std::vector<Rectangle>::const_iterator Obstacles::begin() const
{
  return obstacles_vec_.cbegin();
}
std::vector<Rectangle>::const_iterator Obstacles::end() const
{
  return obstacles_vec_.cend();
}

bool Obstacles::loadFromFile(std::string const& filepath)
{
  std::ifstream file_in{filepath, std::ios::in};

  // failed to open the file
  if (!file_in.is_open()) {
    kape::log << "[ERROR]:\tfrom Obstacles::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tCouldn't open file at \""
              << filepath << "\"\n";
    return false;
  }

  std::size_t num_obstacles;
  file_in >> num_obstacles;

  // badly formatted file
  if (file_in.eof()) {
    kape::log << "[ERROR]:\tfrom Obstacles::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tTried to load from \""
              << filepath << "\" but it was badly formatted\n";
    return false;
  }

  obstacles_vec_.reserve(num_obstacles);
  try {
    std::generate_n(
        std::back_inserter(obstacles_vec_), num_obstacles, [&file_in]() {
          double top_left_corner_x;
          double top_left_corner_y;
          double width;
          double height;

          file_in >> top_left_corner_x >> top_left_corner_y >> width >> height;
          return Rectangle{Vector2d{top_left_corner_x, top_left_corner_y},
                           width, height};
        });
  } catch (std::invalid_argument const& error) {
    kape::log << "[ERROR]:\tfrom Obstacles::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tthrown exception std::invalid_argument "
                 "with message: \n\t\t\t"
              << error.what() << '\n';
  }

  std::string end_check;
  file_in >> end_check;
  // reached the eof too early->the read failed
  if (end_check != "END") {
    kape::log << "[ERROR]:\tfrom Obstacles::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tTried to load from \""
              << filepath << "\" but it was badly formatted\n";
    obstacles_vec_.clear();
    return false;
  }

  return true;
}
bool Obstacles::saveToFile(std::string const& filepath)
{
  std::ofstream file_out{filepath, std::ios::out | std::ios::trunc};

  // failed to open the file
  if (!file_out.is_open()) {
    kape::log << "[ERROR]:\tfrom Obstacles::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tCouldn't open file at \""
              << filepath << "\"\n";
    return false;
  }

  file_out << obstacles_vec_.size() << '\n';
  for (auto const& obs : obstacles_vec_) {
    file_out << obs.getRectangleTopLeftCorner().x << '\t'
             << obs.getRectangleTopLeftCorner().y << '\t'
             << obs.getRectangleWidth() << '\t' << obs.getRectangleHeight()
             << '\n';
  }

  file_out << "END\n";

  // no need to call file_out.close() because it's done by its deconstructor
  return true;
}

// FoodParticle class Implementation--------------------------
FoodParticle::FoodParticle(Vector2d const& position)
    : position_{position}
{}

// FoodParticle::FoodParticle(FoodParticle const& food_particle)
//     : FoodParticle{food_particle.getPosition()}
// {}

Vector2d const& FoodParticle::getPosition() const
{
  return position_;
}

// FoodParticle& FoodParticle::operator=(FoodParticle const& rhs)
// {
//   if (&rhs != this) {
//     position_ = rhs.position_;
//   }
//   return *this;
// }

// PheromoneParticle class Implementation--------------------------
PheromoneParticle::PheromoneParticle(Vector2d const& position, int intensity)
    : position_{position}
    , intensity_{intensity}
{
  if (intensity_ < 0 || intensity_ > 100)
    throw std::invalid_argument{
        "The pheromones intensity must be between 0 and 100"};
}

// PheromoneParticle::PheromoneParticle(
//     PheromoneParticle const& pheromone_particle)
//     : PheromoneParticle{pheromone_particle.getPosition(),
//                         pheromone_particle.getIntensity()}
// {}

Vector2d const& PheromoneParticle::getPosition() const
{
  return position_;
}

int PheromoneParticle::getIntensity() const
{
  return intensity_;
}

void PheromoneParticle::decreaseIntensity(int amount)
{
  if (amount < 0) {
    throw std::invalid_argument{"The amount must be a positive number"};
  }

  intensity_ -= amount;
  if (intensity_ < 0) {
    intensity_ = 0;
  }
}

bool PheromoneParticle::hasEvaporated() const
{
  return intensity_ == 0;
}

// PheromoneParticle& PheromoneParticle::operator=(PheromoneParticle const& rhs)
// {
//   if (&rhs != this) {
//     position_  = rhs.position_;
//     intensity_ = rhs.intensity_;
//   }

//   return *this;
// }

// Food Class implementation -----------------------------------

// Food::CircleWithFood class implementation--------------------
// may throw std::invalid_argument if number_of_food_particles < 0 or if the
// circle intersects with any of the obstacles
Food::CircleWithFood::CircleWithFood(Circle const& circle,
                                     std::size_t number_of_food_particles,
                                     Obstacles const& obstacles,
                                     std::default_random_engine& engine)
    : circle_{circle}
    , food_vec_{}
{
  // if the circle intersects any obstacles
  if (std::any_of(obstacles.begin(), obstacles.end(),
                  [&circle](Rectangle const& rectangle) {
                    return doShapesIntersect(circle, rectangle);
                  })) {
    throw std::invalid_argument{"can't construct a CircleWithFood object if "
                                "its circle intersects any obstacle"};
  }

  std::uniform_real_distribution angle_distribution{0., 2 * PI};
  // sigma = radius/3. makes the probability of having a generated distance from
  // the center = 99.7%.
  std::normal_distribution center_distance_distribution{
      0., circle.getCircleRadius() / 3.};

  std::generate_n(
      std::back_inserter(food_vec_), number_of_food_particles,
      [&circle, &center_distance_distribution, &angle_distribution, &engine]() {
        double angle{angle_distribution(engine)};
        double center_distance{std::abs(center_distance_distribution(engine))};
        if (center_distance > circle.getCircleRadius()) {
          center_distance = circle.getCircleRadius();
        }

        Vector2d position{rotate(Vector2d{0., 1.}, angle)};
        position *= center_distance;
        position += circle.getCircleCenter();
        return FoodParticle{position};
      });
}

Circle const& Food::CircleWithFood::getCircle() const
{
  return circle_;
}

std::size_t Food::CircleWithFood::getNumberOfFoodParticles() const
{
  return food_vec_.size();
}

bool Food::CircleWithFood::removeOneFoodParticleInCircle(Circle const& circle)
{
  auto food_particle_it{
      std::find_if(food_vec_.begin(), food_vec_.end(),
                   [&circle](FoodParticle const& food_particle) {
                     return circle.isInside(food_particle.getPosition());
                   })};

  if (food_particle_it == food_vec_.end()) {
    return false;
  }

  food_vec_.erase(food_particle_it);
  return true;
}

bool Food::CircleWithFood::isThereFoodLeft() const
{
  return !food_vec_.empty();
}

std::vector<FoodParticle>::const_iterator Food::CircleWithFood::begin() const
{
  return food_vec_.cbegin();
}
std::vector<FoodParticle>::const_iterator Food::CircleWithFood::end() const
{
  return food_vec_.cend();
}

// actual Food class implementation-------------------------------------------

Food::Food(unsigned int seed)
    : circles_with_food_vec_{}
    , engine_{seed}
{}

std::size_t Food::getNumberOfFoodParticles() const
{
  return std::accumulate(
      circles_with_food_vec_.begin(), circles_with_food_vec_.end(), 0ul,
      [](std::size_t sum, CircleWithFood const& circle_with_food) {
        return sum + circle_with_food.getNumberOfFoodParticles();
      });
}
// void Food::addFoodParticle(Vector2d const& position)
// {
//   food_vec_.push_back(FoodParticle{position});
// }

// void Food::addFoodParticle(FoodParticle const& food_particle)
// {
//   food_vec_.push_back(food_particle);
// }

// returns:
//  - true if it generated the food_particles (0 if number_of_particles==0 ->
//    the function did nothing)
//  - false if it didn't generate any particle, i.e. the circle intersects at
//    least in part one obstacle
//
// iterators of class Food::Iterator are invalidated
bool Food::generateFoodInCircle(Circle const& circle,
                                std::size_t number_of_food_particles,
                                Obstacles const& obstacles)
{
  // if the circle intersects any obstacles
  if (std::any_of(obstacles.begin(), obstacles.end(),
                  [&circle](Rectangle const& rectangle) {
                    return doShapesIntersect(circle, rectangle);
                  })) {
    return false;
  }

  // nothing to do...
  if (number_of_food_particles == 0) {
    return true;
  }

  // generating a new circle_with_food.
  // no need for try catch because we already checked that number of particles
  // is > 0 and that the circle doesn't intersect any obstacle
  circles_with_food_vec_.push_back(
      CircleWithFood{circle, number_of_food_particles, obstacles, engine_});

  return true;
}

bool Food::isThereFoodLeft() const
{
  return !circles_with_food_vec_.empty();
}

// iterators of class Food::Iterator are invalidated if true
bool Food::removeOneFoodParticleInCircle(Circle const& circle)
{
  auto circles_with_food_it{circles_with_food_vec_.begin()};
  for (; circles_with_food_it != circles_with_food_vec_.end();
       ++circles_with_food_it) {
    if (!doShapesIntersect(circle, circles_with_food_it->getCircle())) {
      continue;
    }

    if (circles_with_food_it->removeOneFoodParticleInCircle(circle)) {
      if (!circles_with_food_it->isThereFoodLeft()) {
        circles_with_food_vec_.erase(circles_with_food_it);
      }
      return true;
    }
  }
  // no particle found in any of the circles
  return false;
}

bool Food::loadFromFile(Obstacles const& obstacles, std::string const& filepath)
{
  std::ifstream file_in{filepath, std::ios::in};

  // failed to open the file
  if (!file_in.is_open()) {
    kape::log << "[ERROR]:\tfrom Food::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tCouldn't open file at \""
              << filepath << "\"\n";
    return false;
  }

  std::size_t num_circles_with_food;
  file_in >> num_circles_with_food;

  // badly formatted file
  if (file_in.eof()) {
    kape::log << "[ERROR]:\tfrom Food::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tTried to load from \""
              << filepath << "\" but it was badly formatted\n";
    return false;
  }

  circles_with_food_vec_.reserve(num_circles_with_food);
  try {
    std::generate_n(
        std::back_inserter(circles_with_food_vec_), num_circles_with_food,
        [&file_in, &obstacles, this]() {
          double circle_center_x;
          double circle_center_y;
          double circle_radius;
          std::size_t number_of_particles;

          file_in >> circle_center_x >> circle_center_y >> circle_radius
              >> number_of_particles;
          return CircleWithFood{
              Circle{Vector2d{circle_center_x, circle_center_y}, circle_radius},
              number_of_particles, obstacles, engine_};
        });
  } catch (std::invalid_argument const& error) {
    kape::log << "[ERROR]:\tfrom Food::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tthrown exception std::invalid_argument "
                 "with message: \n\t\t\t"
              << error.what() << '\n';
  }

  std::string end_check;
  file_in >> end_check;
  // reached the eof too early->the read failed
  if (end_check != "END") {
    kape::log << "[ERROR]:\tfrom Food::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tTried to load from \""
              << filepath << "\" but it was badly formatted\n";
    circles_with_food_vec_.clear();
    return false;
  }

  return true;
}

bool Food::saveToFile(std::string const& filepath)
{
  std::ofstream file_out{filepath, std::ios::out | std::ios::trunc};

  // failed to open the file
  if (!file_out.is_open()) {
    kape::log << "[ERROR]:\tfrom Food::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tCouldn't open file at \""
              << filepath << "\"\n";
    return false;
  }

  file_out << circles_with_food_vec_.size() << '\n';
  for (auto const& circle_with_food : circles_with_food_vec_) {
    file_out << circle_with_food.getCircle().getCircleCenter().x << '\t'
             << circle_with_food.getCircle().getCircleCenter().y << '\t'
             << circle_with_food.getCircle().getCircleRadius() << '\t'
             << circle_with_food.getNumberOfFoodParticles() << '\n';
  }

  file_out << "END\n";

  // no need to call file_out.close() because it's done by its deconstructor
  return true;
}

// class Food::iterator implementation-------------------------------------
Food::Iterator::Iterator(
    std::vector<FoodParticle>::const_iterator const& food_particle_it,
    std::vector<CircleWithFood>::const_iterator const& circle_with_food_it,
    std::vector<CircleWithFood>::const_iterator const& circle_with_food_back_it)
    : food_particle_it_{food_particle_it}
    , circle_with_food_it_{circle_with_food_it}
    , circle_with_food_back_it_{circle_with_food_back_it}
{}

Food::Iterator& Food::Iterator::operator++() // prefix ++
{
  ++food_particle_it_;

  // if we're at the end of the current circle with food
  if (food_particle_it_ == circle_with_food_it_->end()) {
    // if we're NOT the end of the last circle with food
    if (circle_with_food_it_ != circle_with_food_back_it_) {
      food_particle_it_ = (++circle_with_food_it_)->begin();
    }
  }
  return *this;
}

FoodParticle const& Food::Iterator::operator*() const
{
  return *food_particle_it_;
}

bool operator==(Food::Iterator const& lhs, Food::Iterator const& rhs)
{
  return lhs.food_particle_it_ == rhs.food_particle_it_;
}

bool operator!=(Food::Iterator const& lhs, Food::Iterator const& rhs)
{
  return lhs.food_particle_it_ != rhs.food_particle_it_;
}

Food::Iterator Food::begin() const
{
  if (circles_with_food_vec_.empty()) {
    return Food::Iterator{std::vector<FoodParticle>::iterator{nullptr},
                          circles_with_food_vec_.end(),
                          circles_with_food_vec_.end()};
  }

  return Food::Iterator{circles_with_food_vec_.begin()->begin(),
                        circles_with_food_vec_.begin(),
                        circles_with_food_vec_.end() - 1};
}

Food::Iterator Food::end() const
{
  if (circles_with_food_vec_.empty()) {
    return Food::Iterator{std::vector<FoodParticle>::iterator{nullptr},
                          circles_with_food_vec_.end(),
                          circles_with_food_vec_.end()};
  }

  return Food::Iterator{circles_with_food_vec_.back().end(),
                        circles_with_food_vec_.end() - 1,
                        circles_with_food_vec_.end() - 1};
}

// std::vector<FoodParticle>::const_iterator Food::begin() const
// {
//   return food_vec_.cbegin();
// }
// std::vector<FoodParticle>::const_iterator Food::end() const
// {
//   return food_vec_.cend();
// }

// Pheromones class implementation ------------------------------
Pheromones::Pheromones(Type type)
    : pheromones_vec_{}
    , type_{type}
    , time_since_last_evaporation_{0.}
{}

int Pheromones::getPheromonesIntensityInCircle(Circle const& circle) const
{
  return std::accumulate(
      pheromones_vec_.begin(), pheromones_vec_.end(), 0,
      [&circle](int sum, PheromoneParticle const& pheromone) {
        return sum
             + (circle.isInside(pheromone.getPosition())
                    ? pheromone.getIntensity()
                    : 0);
      });
}

Pheromones::Type Pheromones::getPheromonesType() const
{
  return type_;
}
std::size_t Pheromones::getNumberOfPheromones() const
{
  return pheromones_vec_.size();
}

void Pheromones::addPheromoneParticle(Vector2d const& position, int intensity)
{
  pheromones_vec_.push_back(PheromoneParticle{position, intensity});
}

void Pheromones::addPheromoneParticle(PheromoneParticle const& particle)
{
  pheromones_vec_.push_back(particle);
}

// may throw std::invalid_argument if delta_t<0.
void Pheromones::updateParticlesEvaporation(double delta_t)
{
  if (delta_t < 0.) {
    throw std::invalid_argument{"delta_t can't be negative"};
  }

  time_since_last_evaporation_ += delta_t;
  if (time_since_last_evaporation_ < PERIOD_BETWEEN_EVAPORATION_UPDATE_) {
    return;
  }
  time_since_last_evaporation_ -= PERIOD_BETWEEN_EVAPORATION_UPDATE_;

  for (auto& pheromone : pheromones_vec_) {
    pheromone.decreaseIntensity();
  }

  // remove phermones that have evaporated
  pheromones_vec_.erase(std::remove_if(pheromones_vec_.begin(),
                                       pheromones_vec_.end(),
                                       [](PheromoneParticle const& particle) {
                                         return particle.hasEvaporated();
                                       }),
                        pheromones_vec_.end());
}

std::vector<PheromoneParticle>::const_iterator Pheromones::begin() const
{
  return pheromones_vec_.cbegin();
}
std::vector<PheromoneParticle>::const_iterator Pheromones::end() const
{
  return pheromones_vec_.cend();
}

// implementation of class Anthill
Anthill::Anthill(Vector2d center, double radius, int food_counter)
    : circle_{center, radius}
    , food_counter_{food_counter}
{
  if (food_counter_ < 0) {
    throw std::invalid_argument{"the food counter can't be negative"};
  }
}
Anthill::Anthill(Circle const& circle, int food_counter)
    : Anthill{circle.getCircleCenter(), circle.getCircleRadius(), food_counter}
{}

Circle const& Anthill::getCircle() const
{
  return circle_;
}

Vector2d const& Anthill::getCenter() const
{
  return circle_.getCircleCenter();
}
double Anthill::getRadius() const
{
  return circle_.getCircleRadius();
}
int Anthill::getFoodCounter() const
{
  return food_counter_;
}
bool Anthill::isInside(Vector2d const& position) const
{
  return circle_.isInside(position);
}

void Anthill::addFood(int amount)
{
  if (amount < 0) {
    throw std::invalid_argument{"the amount of food added can't be negative"};
  }

  food_counter_ += amount;
}

bool Anthill::loadFromFile(std::string const& filepath)
{
  Circle initial_circle{circle_};
  int initial_food_counter{food_counter_};

  std::ifstream file_in{filepath, std::ios::in};

  // failed to open the file
  if (!file_in.is_open()) {
    kape::log << "[ERROR]:\tfrom Anthill::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tCouldn't open file at \""
              << filepath << "\"\n";
    return false;
  }

  double circle_center_x;
  double circle_center_y;
  double circle_radius;
  int food_counter;

  file_in >> circle_center_x >> circle_center_y >> circle_radius
      >> food_counter;

  circle_.setCircleCenter(Vector2d{circle_center_x, circle_center_y});
  circle_.setCircleRadius(circle_radius);
  food_counter_ = food_counter;

  std::string end_check;
  file_in >> end_check;

  // reached the eof too early or too late->the read failed
  if (end_check != "END") {
    circle_       = initial_circle;
    food_counter_ = initial_food_counter;
    kape::log << "[ERROR]:\tfrom Anthill::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tTried to load from \""
              << filepath << "\" but it was badly formatted\n";
    return false;
  }
  return true;
}

bool Anthill::saveToFile(std::string const& filepath)
{
  std::ofstream file_out{filepath, std::ios::out | std::ios::trunc};

  // failed to open the file
  if (!file_out.is_open()) {
    kape::log << "[ERROR]:\tfrom Anthill::loadFromFile(std::string const& "
                 "filepath):\n\t\t\tCouldn't open file at \""
              << filepath << "\"\n";
    return false;
  }

  file_out << circle_.getCircleCenter().x << '\t' << circle_.getCircleCenter().y
           << '\t' << circle_.getCircleRadius() << '\t' << food_counter_
           << '\n';

  file_out << "END\n";

  // no need to call file_out.close() because it's done by its deconstructor
  return true;
}

} // namespace kape