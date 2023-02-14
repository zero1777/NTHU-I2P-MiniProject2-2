#include <allegro5/color.h>

#include "GameEngine.hpp"
#include "IScene.hpp"
#include "PlayScene.hpp"
#include "ToolButton.hpp"

PlayScene* ToolButton::getPlayScene() {
	return dynamic_cast<PlayScene*>(Engine::GameEngine::GetInstance().GetActiveScene());
}
ToolButton::ToolButton(std::string img, std::string imgIn, Engine::Sprite Tool, float x, float y) :
	ImageButton(img, imgIn, x, y), Tool(Tool) {
}
void ToolButton::Update(float deltaTime) {
	ImageButton::Update(deltaTime);
}
void ToolButton::Draw() const {
	ImageButton::Draw();
	Tool.Draw();
}
