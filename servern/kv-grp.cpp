#include "kv.hpp"
#include <stdio.h>
#include <string>
#include <cstring>
#include <lmdb.h>
#include <assert.h>
#include <stdexcept>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "Stream.hpp"
#include "log.hpp"

using std::string;
using std::unique_ptr;

typedef std::chrono::duration<long,std::ratio<1,64>> Ticks;

Ticks now()
{
	return Ticks(1);
}

struct GroupKVCtrl
{
	std::mutex cs;
	std::condition_variable cv;
	unsigned nopen;
	Ticks open_since;
	const Ticks group_delay = Ticks(std::chrono::seconds(3));

	void Open()
	{
		std::lock_guard<std::mutex> lock (cs);
		if(!open_since.count()) {
			open_since = now();
		}
		nopen++;
	}

	void Close()
	{
		std::unique_lock<std::mutex> lock (cs);
		if(nopen==1) {
			while((now()-open_since)<group_delay) {
				lock.unlock();
				std::this_thread::sleep_for(std::chrono::seconds(1));
				lock.lock();
			}
			ActuallyCommit();
			cv.notify_all();
		} else {
			nopen--;
			while(nopen) {
				cv.wait(lock);
			}
		}
	}
	void ActuallyCommit()
	{
		//todo
	}
};
