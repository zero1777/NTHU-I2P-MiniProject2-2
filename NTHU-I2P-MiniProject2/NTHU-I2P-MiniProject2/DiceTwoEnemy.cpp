#include "PlayScene.hpp"
#include "Turret.hpp"
#include "Bullet.hpp"
#include "AudioHelper.hpp"
#include "DiceTwoEnemy.hpp"
#include "DiceOneEnemy.hpp"
#include "GameEngine.hpp"
#include "LOG.hpp"
#include <string>

DiceTwoEnemy::DiceTwoEnemy(int x, int y) : Enemy("play/dice-2.png", x, y, 20, 35, 7, 7) {}

void DiceTwoEnemy::Hit(float damage) {
    hp -= damage;
	if (hp <= 0) {
		OnExplode();
		// Remove all turret's reference to target.
		for (auto& it: lockedTurrets)
			it->Target = nullptr;
		for (auto& it: lockedBullets)
			it->Target = nullptr;
		getPlayScene()->EarnMoney(money);
		getPlayScene()->EnemyGroup->RemoveObject(objectIterator);
        getPlayScene()->GenNewEnemy(1, Position);
		AudioHelper::PlayAudio("explosion.wav");
	}
}