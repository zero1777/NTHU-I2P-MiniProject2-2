#include <allegro5/allegro.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <vector>
#include <queue>
#include <string>
#include <memory>

#include "AudioHelper.hpp"
#include "DirtyEffect.hpp"
#include "Enemy.hpp"
#include "GameEngine.hpp"
#include "Group.hpp"
#include "IObject.hpp"
#include "Image.hpp"
#include "Label.hpp"
// Turret
#include "LaserTurret.hpp"
#include "MachineGunTurret.hpp"
#include "RedMachineGunTurret.hpp"
#include "RocketTurret.hpp"
#include "MissileTurret.hpp"
#include "PlugGunTurret.hpp"
#include "Plane.hpp"
// Enemy
#include "PlaneEnemy.hpp"
#include "SoldierEnemy.hpp"
#include "RedNormalEnemy.hpp"
#include "TankEnemy.hpp"
#include "DiceOneEnemy.hpp"
#include "DiceTwoEnemy.hpp"
#include "PlayScene.hpp"
#include "Resources.hpp"
#include "Sprite.hpp"
#include "Turret.hpp"
#include "TurretButton.hpp"
#include "ToolButton.hpp"
#include "LOG.hpp"

bool PlayScene::DebugMode = false;
const std::vector<Engine::Point> PlayScene::directions = { Engine::Point(-1, 0), Engine::Point(0, -1), Engine::Point(1, 0), Engine::Point(0, 1) };
const int PlayScene::MapWidth = 20, PlayScene::MapHeight = 13;
const int PlayScene::BlockSize = 64;
const float PlayScene::DangerTime = 7.61;
const Engine::Point PlayScene::SpawnGridPoint = Engine::Point(-1, 0);
const Engine::Point PlayScene::EndGridPoint = Engine::Point(MapWidth, MapHeight - 1);
const std::vector<int> PlayScene::code = { ALLEGRO_KEY_UP, ALLEGRO_KEY_DOWN, ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT,
									ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT, ALLEGRO_KEY_ENTER };
Engine::Point PlayScene::GetClientSize() {
	return Engine::Point(MapWidth * BlockSize, MapHeight * BlockSize);
}
void PlayScene::Initialize() {
	mapState.clear();
	keyStrokes.clear();
	ticks = 0;
	deathCountDown = -1;
	lives = 10;
	money = 1500;
	SpeedMult = 1;
	// Add groups from bottom to top.
	AddNewObject(TileMapGroup = new Group());
	AddNewObject(GroundEffectGroup = new Group());
	AddNewObject(DebugIndicatorGroup = new Group());
	AddNewObject(TowerGroup = new Group());
	AddNewObject(EnemyGroup = new Group());
	AddNewObject(BulletGroup = new Group());
	AddNewObject(EffectGroup = new Group());
	// Should support buttons.
	AddNewControlObject(UIGroup = new Group());
	ReadMap();
	ReadEnemyWave();
	mapDistance = CalculateBFSDistance();
	ConstructUI();
	imgTarget = new Engine::Image("play/target.png", 0, 0);
	imgTarget->Visible = false;
	preview = nullptr;
	view = nullptr;
	viewType = -1;
	UIGroup->AddNewObject(imgTarget);
	// Preload Lose Scene
	deathBGMInstance = Engine::Resources::GetInstance().GetSampleInstance("astronomia.ogg");
	Engine::Resources::GetInstance().GetBitmap("lose/benjamin-happy.png");
	// Start BGM.
	// bgmId = AudioHelper::PlayBGM("play.ogg");
	if (!mute)
        bgmInstance = AudioHelper::PlaySample("play.ogg", true, AudioHelper::BGMVolume);
    else
        bgmInstance = AudioHelper::PlaySample("play.ogg", true, 0.0);
}
void PlayScene::Terminate() {
	AudioHelper::StopBGM(bgmId);
	AudioHelper::StopSample(deathBGMInstance);
	deathBGMInstance = std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE>();
	IScene::Terminate();
}
void PlayScene::Update(float deltaTime) {
	// If we use deltaTime directly, then we might have Bullet-through-paper problem.
	// Reference: Bullet-Through-Paper
	if (SpeedMult == 0)
		deathCountDown = -1;
	else if (deathCountDown != -1)
		SpeedMult = 1;
	// Calculate danger zone.
	std::vector<float> reachEndTimes;
	for (auto& it : EnemyGroup->GetObjects()) {
		reachEndTimes.push_back(dynamic_cast<Enemy*>(it)->reachEndTime);
	}
	// Can use Heap / Priority-Queue instead. But since we won't have too many enemies, sorting is fast enough.
	std::sort(reachEndTimes.begin(), reachEndTimes.end());
	float newDeathCountDown = -1;
	int danger = lives;
	for (auto& it : reachEndTimes) {
		if (it <= DangerTime) {
			danger--;
			if (danger <= 0) {
				// Death Countdown
				float pos = DangerTime - it;
				if (it > deathCountDown) {
					// Restart Death Count Down BGM.
					AudioHelper::StopSample(deathBGMInstance);
					if (SpeedMult != 0)
						deathBGMInstance = AudioHelper::PlaySample("astronomia.ogg", false, AudioHelper::BGMVolume, pos);
				}
				float alpha = pos / DangerTime;
				alpha = std::max(0, std::min(255, static_cast<int>(alpha * alpha * 255)));
				dangerIndicator->Tint = al_map_rgba(255, 255, 255, alpha);
				newDeathCountDown = it;
				break;
			}
		}
	}
	deathCountDown = newDeathCountDown;
	if (SpeedMult == 0)
		AudioHelper::StopSample(deathBGMInstance);
	if (deathCountDown == -1 && lives > 0) {
		AudioHelper::StopSample(deathBGMInstance);
		dangerIndicator->Tint.a = 0;
	}
	if (SpeedMult == 0)
		deathCountDown = -1;
	for (int i = 0; i < SpeedMult; i++) {
		IScene::Update(deltaTime);
		// Check if we should create new enemy.
		ticks += deltaTime;
		if (enemyWaveData.empty()) {
			if (EnemyGroup->GetObjects().empty()) {
				// Free resources.
                /*
				delete TileMapGroup;
				delete GroundEffectGroup;
				delete DebugIndicatorGroup;
				delete TowerGroup;
				delete EnemyGroup;
				delete BulletGroup;
				delete EffectGroup;
				delete UIGroup;
				delete imgTarget;
                */
				// Win.
				Engine::GameEngine::GetInstance().ChangeScene("win");
			}
			continue;
		}
		auto current = enemyWaveData.front();
		if (ticks < current.second)
			continue;
		ticks -= current.second;
		enemyWaveData.pop_front();
		const Engine::Point SpawnCoordinate = Engine::Point(SpawnGridPoint.x * BlockSize + BlockSize / 2, SpawnGridPoint.y * BlockSize + BlockSize / 2);
		Enemy* enemy;
		switch (current.first) {
		case 1:
			EnemyGroup->AddNewObject(enemy = new DiceOneEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
			break;
		case 2:
			EnemyGroup->AddNewObject(enemy = new DiceTwoEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
			break;
		// case 3:
		// 	EnemyGroup->AddNewObject(enemy = new TankEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
		// 	break;
        case 4:
            EnemyGroup->AddNewObject(enemy = new RedNormalEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
            break;
		default:
			continue;
		}
		enemy->UpdatePath(mapDistance);
		// Compensate the time lost.
		enemy->Update(ticks);
	}
	// new enemies
	for (auto info : newEnemies) {
		Enemy *enemy;
		if (info.first == 1) {
			EnemyGroup->AddNewObject(enemy = new DiceOneEnemy(info.second.x, info.second.y));
		}
		enemy->UpdatePath(mapDistance);
	}
	newEnemies.clear();
	// preview
	if (preview) {
		preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
		// To keep responding when paused.
		preview->Update(deltaTime);
	}
	// view
	if (view) {
		view->Position = Engine::GameEngine::GetInstance().GetMousePosition();
	}
}
void PlayScene::Draw() const {
	IScene::Draw();
	if (DebugMode) {
		// Draw reverse BFS distance on all reachable blocks.
		for (int i = 0; i < MapHeight; i++) {
			for (int j = 0; j < MapWidth; j++) {
				if (mapDistance[i][j] != -1) {
					// Not elegant nor efficient, but it's quite enough for debugging.
					Engine::Label label(std::to_string(mapDistance[i][j]), "pirulen.ttf", 32, (j + 0.5) * BlockSize, (i + 0.5) * BlockSize);
					label.Anchor = Engine::Point(0.5, 0.5);
					label.Draw();
				}
			}
		}
	}
}
void PlayScene::OnMouseDown(int button, int mx, int my) {
	if ((button & 1) && !imgTarget->Visible && preview) {
		// Cancel turret construct.
		UIGroup->RemoveObject(preview->GetObjectIterator());
		preview = nullptr;
	}
	if ((button & 1) && view) {
		int x = mx / BlockSize;
		int y = my / BlockSize;
		if (x < 0 || x >= MapWidth || y < 0 || y >= MapHeight) {
			UIGroup->RemoveObject(view->GetObjectIterator());
			view = nullptr;
			viewType = -1;
		}
	}
	IScene::OnMouseDown(button, mx, my);
}
void PlayScene::OnMouseMove(int mx, int my) {
	IScene::OnMouseMove(mx, my);
	const int x = mx / BlockSize;
	const int y = my / BlockSize;
	if (!preview || x < 0 || x >= MapWidth || y < 0 || y >= MapHeight) {
		imgTarget->Visible = false;
		return;
	}
	if (view) {
		imgTarget->Visible = false;
		return ;
	} 
	imgTarget->Visible = true;
	imgTarget->Position.x = x * BlockSize;
	imgTarget->Position.y = y * BlockSize;
}
void PlayScene::OnMouseUp(int button, int mx, int my) {
	IScene::OnMouseUp(button, mx, my);
	if (!imgTarget->Visible && !view)
		return;
	const int x = mx / BlockSize;
	const int y = my / BlockSize;
	if (button & 1) {
		if (!preview && !view) return ;
		if (preview) {
			if (mapState[y][x] != TILE_OCCUPIED) {
				// Check if valid.
				if (!CheckSpaceValid(x, y)) {
					Engine::Sprite* sprite;
					GroundEffectGroup->AddNewObject(sprite = new DirtyEffect("play/target-invalid.png", 1, x * BlockSize + BlockSize / 2, y * BlockSize + BlockSize / 2));
					sprite->Rotation = 0;
					return;
				}
				// Purchase.
				if (!preview->Moved)
					EarnMoney(-preview->GetPrice());
				// Remove Preview.
				preview->GetObjectIterator()->first = false;
				UIGroup->RemoveObject(preview->GetObjectIterator());
				// Construct real turret.
				preview->Position.x = x * BlockSize + BlockSize / 2;
				preview->Position.y = y * BlockSize + BlockSize / 2;
				preview->Enabled = true;
				preview->Preview = false;
				preview->Tint = al_map_rgba(255, 255, 255, 255);
				TowerGroup->AddNewObject(preview);
				// To keep responding when paused.
				preview->Update(0);
				// Only for Rocket Turret.
				// TODO 3 (new turret with bullets moving in circular orbit)
				if (preview->GetLevel() == 3) {
					RocketTurret *tmp = dynamic_cast<RocketTurret*>(preview);
					tmp->Activate();
				}
				// Remove Preview.
				preview = nullptr;

				mapState[y][x] = TILE_OCCUPIED;
				OnMouseMove(mx, my);
			} else if (preview->GetLevel() == 1) { // mapState[y][x] = TILE_OCCUPIED & possible upgrade
				for (auto &it : TowerGroup->GetObjects()) {
					Turret *turret = dynamic_cast<Turret*>(it);
					int tx = static_cast<int>(floor(turret->Position.x / BlockSize));
					int ty = static_cast<int>(floor(turret->Position.y / BlockSize));
					if (tx == x && ty == y) {
						// TODO 2 (new turret)
						if (turret->GetLevel() == 1) {
							turret->Destroy();
							// Remove Preview.
							preview->GetObjectIterator()->first = false;
							UIGroup->RemoveObject(preview->GetObjectIterator());	
							preview = nullptr;
							// Construct real turret
							Turret *newTurret = new RedMachineGunTurret(0, 0);
							EarnMoney(-newTurret->GetPrice());
							newTurret->Position.x = x * BlockSize + BlockSize / 2;
							newTurret->Position.y = y * BlockSize + BlockSize / 2;
							newTurret->Enabled = true;
							newTurret->Preview = false;
							newTurret->Tint = al_map_rgba(255, 255, 255, 255);
							TowerGroup->AddNewObject(newTurret);	
						}	
					}
				}
				mapState[y][x] = TILE_OCCUPIED;
				OnMouseMove(mx, my);
			}
		}
		if (view) {
			if (mapState[y][x] == TILE_OCCUPIED) {
				// TODO 4, 5 (shovel and shifter)
				if (viewType == 0) { // shovel
					// Remove view
					view->GetObjectIterator()->first = false;
					UIGroup->RemoveObject(view->GetObjectIterator());
					view = nullptr;
					viewType = -1;
					for (auto &it : TowerGroup->GetObjects()) {
						Turret *turret = dynamic_cast<Turret*>(it);
						int tx = static_cast<int>(floor(turret->Position.x / BlockSize));
						int ty = static_cast<int>(floor(turret->Position.y / BlockSize));
						if (tx == x && ty == y) {
							EarnMoney(turret->GetPrice() / 2);
							// TODO 3 (new turret with bullets moving in circular orbit)
							if (turret->GetLevel() == 3) {
								RocketTurret *tmp = dynamic_cast<RocketTurret*>(turret);
								tmp->Deactivate();
							}
							turret->Destroy();
						}
					}
					OnMouseMove(mx, my);
				} else if (viewType == 1) { // move
					// Remove view 
					view->GetObjectIterator()->first = false;
					UIGroup->RemoveObject(view->GetObjectIterator());
					view = nullptr;
					viewType = -1;
					for (auto &it : TowerGroup->GetObjects()) {
						Turret *turret = dynamic_cast<Turret*>(it);
						int tx = static_cast<int>(floor(turret->Position.x / BlockSize));
						int ty = static_cast<int>(floor(turret->Position.y / BlockSize));
						if (tx == x && ty == y) {
							if (turret->GetLevel() == 0) {
								preview = new PlugGunTurret(0, 0);
							} else if (turret->GetLevel() == 1) {
								preview = new MachineGunTurret(0, 0);
							} else if (turret->GetLevel() == 2) {
								preview = new RedMachineGunTurret(0, 0);
							} else if (turret->GetLevel() == 3) {
								preview = new RocketTurret(0, 0);
							}
							preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
							preview->Tint = al_map_rgba(255, 255, 255, 200);
							preview->Enabled = false;
							preview->Preview = true;
							preview->Moved = true;
							UIGroup->AddNewObject(preview);
							// TODO 3 (new turret with bullets moving in circular orbit)
							if (turret->GetLevel() == 3) {
								RocketTurret *tmp = dynamic_cast<RocketTurret*>(turret);
								tmp->Deactivate();
							}
							turret->Destroy();
						}
					}
					OnMouseMove(mx, my);
				}
			} 
		}
		// -----------
		// if (mapState[y][x] != TILE_OCCUPIED) {
		// 	if (!preview)
		// 		return;
		// 	// Check if valid.
		// 	if (!CheckSpaceValid(x, y)) {
		// 		Engine::Sprite* sprite;
		// 		GroundEffectGroup->AddNewObject(sprite = new DirtyEffect("play/target-invalid.png", 1, x * BlockSize + BlockSize / 2, y * BlockSize + BlockSize / 2));
		// 		sprite->Rotation = 0;
		// 		return;
		// 	}
		// 	// Purchase.
		// 	EarnMoney(-preview->GetPrice());
		// 	// Remove Preview.
		// 	preview->GetObjectIterator()->first = false;
		// 	UIGroup->RemoveObject(preview->GetObjectIterator());
		// 	// Construct real turret.
		// 	preview->Position.x = x * BlockSize + BlockSize / 2;
		// 	preview->Position.y = y * BlockSize + BlockSize / 2;
		// 	preview->Enabled = true;
		// 	preview->Preview = false;
		// 	preview->Tint = al_map_rgba(255, 255, 255, 255);
		// 	TowerGroup->AddNewObject(preview);
		// 	// To keep responding when paused.
		// 	preview->Update(0);
		// 	// Remove Preview.
		// 	preview = nullptr;

		// 	mapState[y][x] = TILE_OCCUPIED;
		// 	OnMouseMove(mx, my);
		// } else { // mapState[y][x] = TILE_OCCUPIED
		// 	if (!view)
		// 		return ;
		// 	// Remove view
		// 	view->GetObjectIterator()->first = false;
		// 	UIGroup->RemoveObject(view->GetObjectIterator());
		// 	view = nullptr;
		// 	for (auto &it : TowerGroup->GetObjects()) {
		// 		Turret *turret = dynamic_cast<Turret*>(it);
		// 		int tx = static_cast<int>(floor(turret->Position.x / BlockSize));
		// 		int ty = static_cast<int>(floor(turret->Position.y / BlockSize));
		// 		if (tx == x && ty == y) {
		// 			EarnMoney(turret->GetPrice() / 2);
		// 			turret->Destroy();
		// 		}
		// 	}
		// 	OnMouseMove(mx, my);
		// }
	}
}
void PlayScene::OnKeyDown(int keyCode) {
	IScene::OnKeyDown(keyCode);
	if (keyCode == ALLEGRO_KEY_TAB) {
		DebugMode = !DebugMode;
	}
	else {
		keyStrokes.push_back(keyCode);
		if (keyStrokes.size() > code.size())
			keyStrokes.pop_front();
		if (keyCode == ALLEGRO_KEY_ENTER && keyStrokes.size() == code.size()) {
			auto it = keyStrokes.begin();
			for (int c : code) {
				if (!((*it == c) ||
					(c == ALLEGRO_KEYMOD_SHIFT &&
					(*it == ALLEGRO_KEY_LSHIFT || *it == ALLEGRO_KEY_RSHIFT))))
					return;
				++it;
			}
			EffectGroup->AddNewObject(new Plane());
			money += 10000;
		}
	}
	if (keyCode == ALLEGRO_KEY_Q) {
		// Hotkey for MachineGunTurret.
		UIBtnClicked(0);
	}
	else if (keyCode == ALLEGRO_KEY_W) {
		// Hotkey for LaserTurret.
		UIBtnClicked(1);
	}
	else if (keyCode == ALLEGRO_KEY_E) {
		// Hotkey for MissileTurret.
		UIBtnClicked(2);
	}
    else if (keyCode == ALLEGRO_KEY_R) {
        // Hotkey for PlugGunTurret
        UIBtnClicked(3);
    }
	else if (keyCode >= ALLEGRO_KEY_0 && keyCode <= ALLEGRO_KEY_9) {
		// Hotkey for Speed up.
		SpeedMult = keyCode - ALLEGRO_KEY_0;
	}
	else if (keyCode == ALLEGRO_KEY_M) {
		// Hotkey for mute / unmute.
        if (mute)
            AudioHelper::ChangeSampleVolume(bgmInstance, AudioHelper::BGMVolume);
        else
            AudioHelper::ChangeSampleVolume(bgmInstance, 0.0);
        mute = !mute;
	}
}
void PlayScene::Hit() {
	lives--;
	UILives->Text = std::string("Life ") + std::to_string(lives);
	if (lives <= 0) {
		Engine::GameEngine::GetInstance().ChangeScene("lose");
	}
}
int PlayScene::GetMoney() const {
	return money;
}
void PlayScene::EarnMoney(int money) {
	this->money += money;
	UIMoney->Text = std::string("$") + std::to_string(this->money);
}
void PlayScene::ReadMap() {
	std::string filename = std::string("resources/map") + std::to_string(MapId) + ".txt";
	// Read map file.
	char c;
	std::vector<bool> mapData;
	std::ifstream fin(filename);
	while (fin >> c) {
		switch (c) {
		case '0': mapData.push_back(false); break;
		case '1': mapData.push_back(true); break;
		case '\n':
		case '\r':
			if (static_cast<int>(mapData.size()) / MapWidth != 0)
				throw std::ios_base::failure("Map data is corrupted.");
			break;
		default: throw std::ios_base::failure("Map data is corrupted.");
		}
	}
	fin.close();
	// Validate map data.
	if (static_cast<int>(mapData.size()) != MapWidth * MapHeight)
		throw std::ios_base::failure("Map data is corrupted.");
	// Store map in 2d array.
	mapState = std::vector<std::vector<TileType>>(MapHeight, std::vector<TileType>(MapWidth));
	for (int i = 0; i < MapHeight; i++) {
		for (int j = 0; j < MapWidth; j++) {
			const int num = mapData[i * MapWidth + j];
			mapState[i][j] = num ? TILE_FLOOR : TILE_DIRT;
			if (num)
				TileMapGroup->AddNewObject(new Engine::Image("play/floor.png", j * BlockSize, i * BlockSize, BlockSize, BlockSize));
			else
				TileMapGroup->AddNewObject(new Engine::Image("play/dirt.png", j * BlockSize, i * BlockSize, BlockSize, BlockSize));
		}
	}
}
void PlayScene::ReadEnemyWave() {
	std::string filename = std::string("resources/enemy") + std::to_string(MapId) + ".txt";
	// Read enemy file.
	float type, wait, repeat;
	enemyWaveData.clear();
	std::ifstream fin(filename);
	while (fin >> type && fin >> wait && fin >> repeat) {
		for (int i = 0; i < repeat; i++)
			enemyWaveData.emplace_back(type, wait);
	}
	fin.close();
}
void PlayScene::ConstructUI() {
	// Background
	UIGroup->AddNewObject(new Engine::Image("play/sand.png", 1280, 0, 320, 832));
	// Text
	UIGroup->AddNewObject(new Engine::Label(std::string("Stage ") + std::to_string(MapId), "pirulen.ttf", 32, 1294, 0));
	UIGroup->AddNewObject(UIMoney = new Engine::Label(std::string("$") + std::to_string(money), "pirulen.ttf", 24, 1294, 48));
	UIGroup->AddNewObject(UILives = new Engine::Label(std::string("Life ") + std::to_string(lives), "pirulen.ttf", 24, 1294, 88));
	// Buttons
	ConstructButton(0, "play/turret-6.png", PlugGunTurret::Price);
	ConstructButton(1, "play/turret-1.png", MachineGunTurret::Price);
	ConstructButton(2, "play/planet.png", RocketTurret::Price);
	// ConstructButton(2, "play/turret-3.png", MissileTurret::Price);
	// TurretButton* btn;
	// Button 1
	// btn = new TurretButton("play/floor.png", "play/dirt.png",
	// 	Engine::Sprite("play/tower-base.png", 1294, 136, 0, 0, 0, 0),
	// 	Engine::Sprite("play/turret-1.png", 1294, 136 - 8, 0, 0, 0, 0)
	// 	, 1294, 136, MachineGunTurret::Price);
	// // Reference: Class Member Function Pointer and std::bind.
	// btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 0));
	// UIGroup->AddNewControlObject(btn);
	// // Button 2
	// btn = new TurretButton("play/floor.png", "play/dirt.png",
	// 	Engine::Sprite("play/tower-base.png", 1370, 136, 0, 0, 0, 0),
	// 	Engine::Sprite("play/turret-2.png", 1370, 136 - 8, 0, 0, 0, 0)
	// 	, 1370, 136, LaserTurret::Price);
	// btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 1));
	// UIGroup->AddNewControlObject(btn);
	// Button 3
	// btn = new TurretButton("play/floor.png", "play/dirt.png",
	// 	Engine::Sprite("play/tower-base.png", 1446, 136, 0, 0, 0, 0),
	// 	Engine::Sprite("play/shovel.png", 1446, 136, 0, 0, 0, 0)
	// 	, 1446, 136, MissileTurret::Price);
	// btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 2));
	// UIGroup->AddNewControlObject(btn);
    // // Button 4
    // btn = new TurretButton("play/floor.png", "play/dirt.png",
    //         Engine::Sprite("play/tower-base.png", 1522, 136, 0, 0, 0, 0),
    //         Engine::Sprite("play/turret-7.png", 1522, 136, 0, 0, 0, 0)
    //         , 1522, 136, PlugGunTurret::Price);
    // btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 3));
    // UIGroup->AddNewControlObject(btn);
	ToolButton *btn;
	// shovel
	btn = new ToolButton("play/floor.png", "play/dirt.png", 
	Engine::Sprite("play/shovel.png", 1294, 212, 0, 0, 0, 0), 1294, 212);
	btn->SetOnClickCallback(std::bind(&PlayScene::UIToolClicked, this, 0));
	UIGroup->AddNewControlObject(btn);
	// move
	btn = new ToolButton("play/floor.png", "play/dirt.png", 
	Engine::Sprite("play/move.png", 1370, 212, 0, 0, 0, 0), 1370, 212);
	btn->SetOnClickCallback(std::bind(&PlayScene::UIToolClicked, this, 1));
	UIGroup->AddNewControlObject(btn);
    
	int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
	int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
	int shift = 135 + 25;
	dangerIndicator = new Engine::Sprite("play/benjamin.png", w - shift, h - shift);
	dangerIndicator->Tint.a = 0;
	UIGroup->AddNewObject(dangerIndicator);
}

void PlayScene::ConstructButton(int id, std::string sprite, int price) {
	TurretButton* btn;
	btn = new TurretButton("play/floor.png", "play/dirt.png",
		Engine::Sprite("play/tower-base.png", 1294 + id * 76, 136, 0, 0, 0, 0),
		Engine::Sprite(sprite, 1294 + id * 76, 136 - 8, 0, 0, 0, 0)
		, 1294 + id * 76, 136, price);
	// Reference: Class Member Function Pointer and std::bind.
	btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, id));
	UIGroup->AddNewControlObject(btn);
}

void PlayScene::UIBtnClicked(int id) {
	if (preview) {
		UIGroup->RemoveObject(preview->GetObjectIterator());
        preview = nullptr;
    }
	if (id == 0 && money >= PlugGunTurret::Price) 
		preview = new PlugGunTurret(0, 0);
	else if (id == 1 && money >= MachineGunTurret::Price)
		preview = new MachineGunTurret(0, 0);
	else if (id == 2 && money >= RocketTurret::Price)
		preview = new RocketTurret(0, 0);
	// if (id == 0 && money >= MachineGunTurret::Price)
	// 	preview = new MachineGunTurret(0, 0);
	// else if (id == 1 && money >= LaserTurret::Price)
	// 	preview = new LaserTurret(0, 0);
	// else if (id == 2 && money >= MissileTurret::Price)
	// 	preview = new MissileTurret(0, 0);
    // else if (id == 3 && money >= PlugGunTurret::Price)
    //     preview = new PlugGunTurret(0, 0);
	if (!preview)
		return;
	preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
	preview->Tint = al_map_rgba(255, 255, 255, 200);
	preview->Enabled = false;
	preview->Preview = true;
	UIGroup->AddNewObject(preview);
	OnMouseMove(Engine::GameEngine::GetInstance().GetMousePosition().x, Engine::GameEngine::GetInstance().GetMousePosition().y);
}

// TODO 4, 5 (shovel and shifter)
void PlayScene::UIToolClicked(int id) {
	if (view) {
		UIGroup->RemoveObject(view->GetObjectIterator());
		view = nullptr;
		viewType = -1;
	}
	if (id == 0) 
		view = new Engine::Sprite("play/shovel.png", 0, 0);
	else if (id == 1) 
		view = new Engine::Sprite("play/move.png", 0, 0);
	viewType = id;
	view->Position = Engine::GameEngine::GetInstance().GetMousePosition();
	view->Tint = al_map_rgba(255, 255, 255, 200);
	UIGroup->AddNewObject(view);
	OnMouseMove(Engine::GameEngine::GetInstance().GetMousePosition().x, Engine::GameEngine::GetInstance().GetMousePosition().y);
}

bool PlayScene::CheckSpaceValid(int x, int y) {
	if (x < 0 || x >= MapWidth || y < 0 || y >= MapHeight)
		return false;
	auto map00 = mapState[y][x];
	mapState[y][x] = TILE_OCCUPIED;
	std::vector<std::vector<int>> map = CalculateBFSDistance();
	mapState[y][x] = map00;
	if (map[0][0] == -1)
		return false;
	for (auto& it : EnemyGroup->GetObjects()) {
		Engine::Point pnt;
		pnt.x = floor(it->Position.x / BlockSize);
		pnt.y = floor(it->Position.y / BlockSize);
		if (pnt.x < 0) pnt.x = 0;
		if (pnt.x >= MapWidth) pnt.x = MapWidth - 1;
		if (pnt.y < 0) pnt.y = 0;
		if (pnt.y >= MapHeight) pnt.y = MapHeight - 1;
		if (map[pnt.y][pnt.x] == -1)
			return false;
	}
	// All enemy have path to exit.
	mapState[y][x] = TILE_OCCUPIED;
	mapDistance = map;
	for (auto& it : EnemyGroup->GetObjects())
		dynamic_cast<Enemy*>(it)->UpdatePath(mapDistance);
	return true;
}
std::vector<std::vector<int>> PlayScene::CalculateBFSDistance() {
	// Reverse BFS to find path.
	std::vector<std::vector<int>> map(MapHeight, std::vector<int>(std::vector<int>(MapWidth, -1)));
	std::queue<Engine::Point> que;
	// Push end point.
	// BFS from end point.
	if (mapState[MapHeight - 1][MapWidth - 1] != TILE_DIRT)
		return map;
	que.push(Engine::Point(MapWidth - 1, MapHeight - 1));
	map[MapHeight - 1][MapWidth - 1] = 0;
	while (!que.empty()) {
		Engine::Point p = que.front();
		que.pop();
        for (auto &c : directions) {
            int x = p.x + c.x;
            int y = p.y + c.y;
            if (x < 0 || x >= MapWidth || y < 0 || y >= MapHeight ||
                map[y][x] != -1 || mapState[y][x] != TILE_DIRT) {
                continue;
            } else {
                map[y][x] = map[p.y][p.x] + 1;
                que.push(Engine::Point(x, y));
            }
        }
	}
	return map;
}
// TODO 1 (new enemy)
void PlayScene::GenNewEnemy(int type, Engine::Point pos) {
	newEnemies.push_back(std::make_pair(type, pos));
}
void PlayScene::ResetMapState(int x, int y) {
	mapState[y][x] = TILE_FLOOR;
}
