#include <allegro5/base.h>
#include <cmath>
#include <string>

#include "AudioHelper.hpp"
#include "RocketBullet.hpp"
#include "Group.hpp"
#include "RocketTurret.hpp"
#include "PlayScene.hpp"
#include "Point.hpp"

const int RocketTurret::Price = 90;
RocketTurret::RocketTurret(float x, float y) :
	// TODO 2 (2/8): You can imitate the 2 files: 'MachineGunTurret.hpp', 'MachineGunTurret.cpp' to create a new turret.
	Turret("play/tower-base.png", "play/planet.png", x, y, 200, Price, 1, 3), period(5), ammo(bulletNum) {
	// Move center downward, since we the turret head is slightly biased upward.
	Anchor.y += 8.0f / GetBitmapHeight();
	for (int i = 0; i < bulletNum; i++) {
		bullets[i] = nullptr;
	}
}
void RocketTurret::Update(float deltaTime) {
	Sprite::Update(deltaTime);
	PlayScene* scene = getPlayScene();
	imgBase.Position = Position;
	imgBase.Tint = Tint;
	if (!Enabled)
		return;
}
void RocketTurret::CreateBullet() {
	Engine::Point diff = Engine::Point(cos(Rotation - ALLEGRO_PI / 2), sin(Rotation - ALLEGRO_PI / 2));
	float rotation = atan2(diff.y, diff.x);
	Engine::Point normalized = diff.Normalize();
	Engine::Point normal = Engine::Point(-normalized.y, normalized.x);
	// Create four new bullets.
	Engine::Point starts[bulletNum] = {
		Position + Engine::Point(CollisionRadius, 0),
		Position + Engine::Point(-CollisionRadius, 0),
		Position + Engine::Point(0, CollisionRadius),
		Position + Engine::Point(0, -CollisionRadius)
	};
	Engine::Point coords[bulletNum] = {
		Engine::Point(1, 0),
		Engine::Point(-1, 0),
		Engine::Point(0, 1),
		Engine::Point(0, -1)
	};
	for (int i = 0; i < bulletNum; i++) {
		getPlayScene()->BulletGroup->AddNewObject(bullets[i] = new RocketBullet(starts[i], diff, rotation, this, CollisionRadius, period, coords[i], i));
	}
}
void RocketTurret::Activate() {
	CreateBullet();
	ammo = bulletNum;
}
void RocketTurret::Deactivate() {
	for (int i = 0; i < bulletNum; i++) {
		if (bullets[i]) {
			getPlayScene()->BulletGroup->RemoveObject(bullets[i]->GetObjectIterator());
			bullets[i] = nullptr;
		}
	}
}
