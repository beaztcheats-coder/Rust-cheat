#pragma once
#include "../../sdk.hpp"
#include "../../skcrypt.hpp"
#include <memory>

struct EspCacheData;

class Visuals {
public:
	void do_Visuals(Rust::BaseEntity* Player, Vector3 lpPos, const EspCacheData& cached, const Matrix4x4& frameMatrix);
	void do_NPC_Visuals(Rust::BaseEntity* Entity, Vector3 lpPos, const EspCacheData& cached, const Matrix4x4& frameMatrix);
	void do_Animal_Visuals(Rust::BaseEntity* Entity, Vector3 lpPos, const EspCacheData& cached, const Matrix4x4& frameMatrix);
	void do_Ghost_Visuals();
}; inline Visuals* esp = nullptr;