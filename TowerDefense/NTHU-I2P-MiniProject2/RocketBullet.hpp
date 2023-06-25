#ifndef ROCKETBULLET_HPP
#define ROCKETBULLET_HPP
#include "Bullet.hpp"

class Enemy;
class Turret;
namespace Engine {
    struct Point;
}  // namespace Engine

class RocketBullet : public Bullet {
protected:
    float radius;
    float period;
    Engine::Point coord;
    Turret* parent;
    int id;
public:
    explicit RocketBullet(Engine::Point position, Engine::Point forwardDirection, float rotation, Turret* parent, float radius, float period, Engine::Point coord, int id);
    void Update(float deltaTime) override;
    void OnExplode(Enemy* enemy) override;
};
#endif // ROCKETBULLET_HPP
