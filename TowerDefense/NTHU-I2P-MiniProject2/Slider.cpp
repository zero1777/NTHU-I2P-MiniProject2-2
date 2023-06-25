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
// TODO 4 (0/6): Finish the 6 functions below and ensure it can control both BGM and SFX volume.
//               The slider shouldn't be dragged outside the bar, and you should also keep the mouse-in / mouse-out effect.
void Slider::Draw() const {
	// TODO 4 (1/6): Draw all components.
    Bar.Draw();
    End1.Draw();
    End2.Draw();
    ImageButton::Draw();
}
void Slider::SetOnValueChangedCallback(std::function<void(float value)> onValueChangedCallback) {
	// TODO 4 (2/6): Set the function pointer. Can imitate ImageButton's 'SetOnClickCallback'.
    OnValueChangedCallback = onValueChangedCallback;
}
void Slider::SetValue(float value) {
	// TODO 4 (3/6): Call 'OnValueChangedCallback' when value changed. Can imitate ImageButton's 'OnClickCallback'.
	//               Also move the slider along the bar, to the corresponding position.
    OnValueChangedCallback(value);
    ImageButton::Position.x = End1.Position.x + (Bar.Size.x) * value;
}
void Slider::OnMouseDown(int button, int mx, int my) {
	// TODO 4 (4/6): Set 'Down' to true if mouse is in the slider.
    if (mx >= ImageButton::Position.x - ImageButton::Size.x / 2 && mx <= ImageButton::Position.x + ImageButton::Size.x / 2
        && my >= ImageButton::Position.y - ImageButton::Size.y / 2 && my <= ImageButton::Position.y + ImageButton::Size.y / 2) {
        Down = true;
        ImageButton::bmp = ImageButton::imgIn;
    }
}
void Slider::OnMouseUp(int button, int mx, int my) {
	// TODO 4 (5/6): Set 'Down' to false.
    Down = false;
    ImageButton::bmp = ImageButton::imgOut;
}
void Slider::OnMouseMove(int mx, int my) {
	// TODO 4 (6/6): Clamp the coordinates and calculate the value. Call 'SetValue' when you're done.
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
