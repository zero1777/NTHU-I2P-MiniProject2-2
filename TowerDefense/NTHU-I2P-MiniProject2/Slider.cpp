#include <algorithm>
#include <string>

#include "Point.hpp"
#include "Slider.hpp"

Slider::Slider(float x, float y, float w, float h) :
	ImageButton("stage-select/slider.png", "stage-select/slider-blue.png", x, y),
	Bar("stage-select/bar.png", x, y, w, h),
	End1("stage-select/end.png", x, y + h / 2, 0, 0, 0.5, 0.5),
	End2("stage-select/end.png", x + w, y + h / 2, 0, 0, 0.5, 0.5) {
	Position.x += w;
	Position.y += h / 2;
	Anchor = Engine::Point(0.5, 0.5);
}
void Slider::Draw() const {
    Bar.Draw();
    End1.Draw();
    End2.Draw();
    ImageButton::Draw();
}
void Slider::SetOnValueChangedCallback(std::function<void(float value)> onValueChangedCallback) {
    OnValueChangedCallback = onValueChangedCallback;
}
void Slider::SetValue(float value) {
    OnValueChangedCallback(value);
    ImageButton::Position.x = End1.Position.x + (Bar.Size.x) * value;
}
void Slider::OnMouseDown(int button, int mx, int my) {
    if (mx >= ImageButton::Position.x - ImageButton::Size.x / 2 && mx <= ImageButton::Position.x + ImageButton::Size.x / 2
        && my >= ImageButton::Position.y - ImageButton::Size.y / 2 && my <= ImageButton::Position.y + ImageButton::Size.y / 2) {
        Down = true;
        ImageButton::bmp = ImageButton::imgIn;
    }
}
void Slider::OnMouseUp(int button, int mx, int my) {
    Down = false;
    ImageButton::bmp = ImageButton::imgOut;
}
void Slider::OnMouseMove(int mx, int my) {
    if (Down) {
        if (mx > End1.Position.x && mx < End2.Position.x) {
            float value = (mx - End1.Position.x) / Bar.Size.x;
            SetValue(value);
            ImageButton::bmp = ImageButton::imgIn;
        } else if (mx <= End1.Position.x) {
            SetValue(0.0);
        } else {
            SetValue(1.0);
        }
    }
}
