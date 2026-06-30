#include "../Misc/misc.hpp"

class aimbot {
public:
	std::mutex hotbar_mutex;
	Rust::BaseEntity* lastTarget = nullptr;
	Rust::BaseEntity* Get_BestEntity();
	void do_aimbot();
}; inline aimbot* aim = nullptr;

