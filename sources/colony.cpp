#include "colony.hpp"
#include <cassert>
#include <stdexcept> //invalid argument

namespace kape {
explicit Colony::Colony(Vector2d position, double radius, int food_counter)
    : position_{position}
    , radius_{radius}
    , food_counter_{food_counter}
{
  assert(radius_ > 0. && food_counter_ >= 0);

  if (radius_ <= 0.) {
    throw std::invalid_argument{"the radius can't be null or negative"};
  }
  if (food_counter_ < 0) {
    throw std::invalid_argument{"the food counter can't be negative"};
  }
}

void Colony::addFood(int amount)
{
  assert(amount >= 0);
  if (amount < 0) {
    throw std::invalid_argument{"the amount of food added can't be negative"};
  }

  food_counter_ += amount;
}

Vector2d Colony::getPosition() const
{
  return position_;
}
double Colony::getRadius() const
{
  return radius_;
}
int Colony::getFoodCounter() const
{
  return food_counter_;
}

} // namespace kape