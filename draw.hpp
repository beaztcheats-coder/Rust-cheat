#include "imgui/imgui.h"


inline void DrawBox(float X, float Y, float W, float H, const ImU32 color, float size)
{
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X, Y), ImVec2(X + W, Y), ImGui::GetColorU32(color), size);
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), size);
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X + W, Y + H), ImVec2(X, Y + H), ImGui::GetColorU32(color), size);
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X, Y + H), ImVec2(X, Y), ImGui::GetColorU32(color), size);
}

inline void draw_text_outline_font(float x, float y, ImColor color, const char* string) {
	char buf[512];
	ImVec2 len = ImGui::CalcTextSize(string);
	ImGui::GetBackgroundDrawList()->AddText(ImVec2(x - 1, y), ImColor(0.0f, 0.0f, 0.0f, 255.f), string);
	ImGui::GetBackgroundDrawList()->AddText(ImVec2(x + 1, y), ImColor(0.0f, 0.0f, 0.0f, 255.f), string);
	ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y - 1), ImColor(0.0f, 0.0f, 0.0f, 255.f), string);
	ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y + 1), ImColor(0.0f, 0.0f, 0.0f, 255.f), string);
	ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y), color, string);
}
inline void DrawCorneredBox(int X, int Y, int W, int H, const ImU32& color, int thickness) {
	// Calculate the length of the corner lines (1/4 of the width and height)
	float cornerLengthW = W / 4.0f;
	float cornerLengthH = H / 4.0f;

	// Draw top-left corner
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X, Y), ImVec2(X + cornerLengthW, Y), color, thickness); // Top horizontal
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X, Y), ImVec2(X, Y + cornerLengthH), color, thickness); // Left vertical

	// Draw top-right corner
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X + W - cornerLengthW, Y), ImVec2(X + W, Y), color, thickness); // Top horizontal
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + cornerLengthH), color, thickness); // Right vertical

	// Draw bottom-left corner
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X, Y + H - cornerLengthH), ImVec2(X, Y + H), color, thickness); // Left vertical
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X, Y + H), ImVec2(X + cornerLengthW, Y + H), color, thickness); // Bottom horizontal

	// Draw bottom-right corner
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X + W - cornerLengthW, Y + H), ImVec2(X + W, Y + H), color, thickness); // Bottom horizontal
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(X + W, Y + H - cornerLengthH), ImVec2(X + W, Y + H), color, thickness); // Right vertical
}
inline void DrawFilledRect(float x, float y, float w, float h, ImColor color)
{
	ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color, NULL, NULL);
}

inline void arc(float x, float y, float radius, float min_angle, float max_angle, ImColor col, float thickness) {
	ImGui::GetBackgroundDrawList()->PathArcTo(ImVec2(x, y), radius, DEG2RAD(min_angle), DEG2RAD(max_angle), 32);
	ImGui::GetBackgroundDrawList()->PathStroke(col, false, thickness);
}

class bounds
{
public:
	float left, right, top, bottom;
	bounds(float left, float right, float top, float bottom) : left(left), right(right), top(top), bottom(bottom) {}
	bounds() : left(FLT_MAX), right(FLT_MIN), top(FLT_MAX), bottom(FLT_MIN) {}
};
template <typename t = float>
auto bound(t current, t min, t max) -> t
{
	if (current < min)
	{
		return min;
	}
	else if (current > max)
	{
		return max;
	}

	return current;
}


static void DrawRect(int x, int y, int w, int h, ImColor color, int thickness, int round = 0)
{
	ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w + 1, y + h + 1), color, round, 0, thickness);
}




inline float GRD_TO_BOG(float GRD) {
	return (M_PI / 180) * GRD;
}

inline float BOG_TO_GRD(float BOG) {
	return (180 / M_PI) * BOG;
}

