#include <allegro5/base.h>
#include <cmath>
#include <string>

#include "AudioHelper.hpp"
#include "BlazeBullet.hpp"
#include "Group.hpp"
#include "RedMachineGunTurret.hpp"
#include "PlayScene.hpp"
#include "Point.hpp"

const int RedMachineGunTurret::Price = 90;
RedMachineGunTurret::RedMachineGunTurret(float x, float y) :
	// TODO 2 (2/8): You can imitate the 2 files: 'MachineGunTurret.hpp', 'MachineGunTurret.cpp' to create a new turret.
	Turret("play/tower-base.png", "play/turret-2.png", x, y, 300, Price, 1, 2) {
	// Move center downward, since we the turret head is slightly biased upward.
	Anchor.y += 8.0f / GetBitmapHeight();
}
void RedMachineGunTurret::CreateBullet() {
	Engine::Point diff = Engine::Point(cos(Rotation - ALLEGRO_PI / 2), sin(Rotation - ALLEGRO_PI / 2));
	float rotation = atan2(diff.y, diff.x);
	Engine::Point normalized = diff.Normalize();
	Engine::Point normal = Engine::Point(-normalized.y, normalized.x);
	// Change bullet position to the front of the gun barrel.
	getPlayScene()->BulletGroup->AddNewObject(new BlazeBullet(Position + normalized * 36 - normal * 6, diff, rotation, this));
	getPlayScene()->BulletGroup->AddNewObject(new BlazeBullet(Position + normalized * 36 + normal * 6, diff, rotation, this));
	AudioHelper::PlayAudio("gun.wav");
}
