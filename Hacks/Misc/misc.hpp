#include "../Cache/cache.hpp"
#include <mutex>

class misc {
public:
	void do_misc();
	static std::mutex s_miscMutex;
}; inline misc* hx = nullptr;
