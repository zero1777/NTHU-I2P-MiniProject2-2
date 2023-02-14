#ifndef BLAZEBULLET_HPP
#define BLAZEBULLET_HPP
#include "Bullet.hpp"

class Enemy;
class Turret;
namespace Engine {
struct Point;
}  // namespace Engine

class BlazeBullet : public Bullet {
public:
	explicit BlazeBullet(Engine::Point position, Engine::Point forwardDirection, float rotation, Turret* parent);
	void Update(float deltaTime) override;
	void OnExplode(Enemy* enemy) override;
};
#endif // BLAZEBULLET_HPP
