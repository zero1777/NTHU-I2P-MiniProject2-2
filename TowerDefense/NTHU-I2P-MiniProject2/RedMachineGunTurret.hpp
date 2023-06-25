#ifndef REDMACHINEGUNTURRET_HPP
#define REDMACHINEGUNTURRET_HPP
#include "Turret.hpp"

class RedMachineGunTurret: public Turret {
public:
	static const int Price;
    RedMachineGunTurret(float x, float y);
	void CreateBullet() override;
};
#endif // REDMACHINEGUNTURRET_HPP
