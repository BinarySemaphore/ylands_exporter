#ifndef WORKPOOL_H
#define WORKPOOL_H

#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <vector>

class Workitem {
public:
	bool alive;
	std::thread thread;
	std::function<void()> call;
	std::function<void()> callback;

	// Constructor
	Workitem(std::function<void()> call, std::function<void()> callback);
};

class Workpool {
private:
	bool debug;
	bool running;
	int max_workers;
	int in_progress;
	std::mutex protex;
	std::mutex quetex;
	std::mutex queue_cv_mutex;
	std::condition_variable queue_cv;
	std::vector<std::thread> workers;
	std::queue<Workitem*> work_queue;

	// Private methods
	void WaitJoin();
	void Thread_Runner(int id);

public:
	static std::mutex shutex;

	// Constructor
	Workpool(int max_workers);
	Workpool(int max_workers, bool debug);

	// Public methods
	void start();
	std::queue<Workitem*>* stop();
	void wait();
	void addTask(std::function<void()> call, std::function<void()> callback);
};

#endif // WORKPOOL_H