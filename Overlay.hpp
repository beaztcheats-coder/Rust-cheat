#include "Render/render.hpp"

class Overlay {
public:
	void DrawOverlay()
	{
		auto result = Renderer->Setup();
		if (result != RENDER_INFORMATION::RENDER_SETUP_SUCCESSFUL) {
			LOG("Overlay Setup FAILED — result code: %d", (int)result);
			LOG("Overlay: FindWindowA may have failed — check if Rust window exists with class 'UnityWndClass' title 'Rust'");
		} else {
			LOG("Overlay Setup SUCCESS — starting render loop");
		}
		Renderer->Render();
		LOG("Overlay Render() returned — overlay loop ended");
	}
};