#ifndef ROCKETTURRET_HPP
#define ROCKETTURRET_HPP
#include "Turret.hpp"
#include "RocketBullet.hpp"

const int bulletNum = 4;
class RocketTurret: public Turret {
protected:
	void CreateBullet() override;
	int period;
public:
	static const int Price;
    RocketTurret(float x, float y);
	void Update(float deltaTime) override;
	void Activate();
	void Deactivate();
	RocketBullet *bullets[bulletNum];
	int ammo;
};
#endif // ROCKETTURRET_HPP
