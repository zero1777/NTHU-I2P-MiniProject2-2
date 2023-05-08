#include <allegro5/base.h>
#include <random>
#include <string>

#include "DirtyEffect.hpp"
#include "Enemy.hpp"
#include "RocketBullet.hpp"
#include "Group.hpp"
#include "PlayScene.hpp"
#include "GameEngine.hpp"
#include "Collider.hpp"
#include "Point.hpp"
#include "Turret.hpp"
#include "RocketTurret.hpp"

class Turret;

RocketBullet::RocketBullet(Engine::Point position, Engine::Point forwardDirection, float rotation, Turret* parent, float radius, float period, Engine::Point coord, int id) :
    Bullet("play/virus.png", 10, 0.5, position, forwardDirection, rotation + ALLEGRO_PI / 2, parent), 
	radius(radius), period(period), parent(parent), coord(coord), id(id) {
}
void RocketBullet::Update(float deltaTime) {
	// Update the coordinate of the bullet  
	float theta = atan2(coord.y, coord.x);
	theta = fmod(theta + deltaTime * 2 * ALLEGRO_PI / period, 2 * ALLEGRO_PI);
	coord = Engine::Point(cos(theta), sin(theta));
	// calculate the target position
	Engine::Point Target = radius * coord;
	Target = Target + parent->Position;
	Velocity = (Target - Position) * speed;
	// Update forward direction
	Rotation = atan2(Velocity.y, Velocity.x) + ALLEGRO_PI / 2;

	// Bullet update 
	Sprite::Update(deltaTime);
	PlayScene* scene = getPlayScene();
	// Can be improved by Spatial Hash, Quad Tree, ...
	// However simply loop through all enemies is enough for this program.
	for (auto& it : scene->EnemyGroup->GetObjects()) {
		Enemy* enemy = dynamic_cast<Enemy*>(it);
		if (!enemy->Visible)
			continue;
		if (Engine::Collider::IsCircleOverlap(Position, CollisionRadius, enemy->Position, enemy->CollisionRadius)) {
			OnExplode(enemy);
			enemy->Hit(damage);
			RocketTurret *turret = dynamic_cast<RocketTurret*>(parent);
			turret->bullets[id] = nullptr;
			getPlayScene()->BulletGroup->RemoveObject(objectIterator);
			if (--turret->ammo == 0)
				turret->Activate();
			return ;
		}
	}
}
void RocketBullet::OnExplode(Enemy* enemy) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(2, 5);
    getPlayScene()->GroundEffectGroup->AddNewObject(new DirtyEffect("play/dirty-1.png", dist(rng), enemy->Position.x, enemy->Position.y));
}
