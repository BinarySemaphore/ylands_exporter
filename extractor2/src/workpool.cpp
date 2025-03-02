#include "workpool.hpp"

#include <iostream>
#include <chrono>

const int WORKER_THREAD_POLL_TIMEMS = 10;

Workitem::Workitem(std::function<void()> call, std::function<void()> callback) {
	this->alive = false;
	this->call = call;
	this->callback = callback;
}

void Workpool::WaitJoin() {
	if (this->debug) std::cout << "[Workpool] Waiting for workers to stop..." << std::endl;
	for (int i = 0; i < this->workers.size(); i++) {
		if (!this->workers[i].joinable()) continue;
		if (this->debug) std::cout << "[Workpool] Waiting on worker " << i + 1 << " to stop..." << std::endl;
		this->workers[i].join();
		if (this->debug) std::cout << "[Workpool] Worker " << i + 1 << " stopped" << std::endl;
	}
	if (this->debug) std::cout << "[Workpool] All workers stopped" << std::endl;
}

void Workpool::Thread_Runner(int id) {
	bool all_idle;
	Workitem* task = nullptr;

	while (true) {
		// Pick up a task
		if (task == nullptr) {
			this->quetex.lock();
			if (!this->running) {
				this->quetex.unlock();
				break;
			}
			if (!this->work_queue.empty()) {
				task = this->work_queue.front();
				this->work_queue.pop();
				if (this->debug) std::cout << "[Workpool-Runner] Picked up new task from queue (" << this->work_queue.size() << ")" << std::endl;
			}
			this->quetex.unlock();

		// Call task
		} else {
			this->protex.lock();
			this->in_progress += 1;
			this->protex.unlock();
			task->call();
			if (task->callback != nullptr) {
				task->callback();
			}
			if (this->debug) std::cout << "[Workpool-Runner] Finished task" << std::endl;

			// Check if all workers are idle (notify wait)
			this->quetex.lock();
			this->protex.lock();
			this->in_progress -= 1;
			all_idle = this->in_progress == 0 && this->work_queue.empty();
			this->quetex.unlock();
			this->protex.unlock();
			if (all_idle) {
				if (this->debug) std::cout << "[Workpool-Runner] Notifying all tasks finished and queue is empty" << std::endl;
				this->queue_cv_mutex.lock();
				this->queue_cv.notify_all();
				this->queue_cv_mutex.unlock();
			}

			// Cleanup
			delete task;
			task = nullptr;
		}

		// Delay next if no task
		if (task == nullptr) {
			std::this_thread::sleep_for(
				std::chrono::milliseconds(WORKER_THREAD_POLL_TIMEMS)
			);
		}
	}
	if (this->debug) std::cout << "[Workpool-Runner] Exited" << std::endl;
}

Workpool::Workpool(int max_workers) {
	this->debug = false;
	this->running = false;
	this->max_workers = max_workers;
	if (this->debug) std::cout << "[Workpool] Created" << std::endl;
}

Workpool::Workpool(int max_workers, bool debug) {
	this->debug = debug;
	this->running = false;
	this->max_workers = max_workers;
	if (this->debug) std::cout << "[Workpool] Created" << std::endl;
}

void Workpool::Start() {
	if (this->running) return;

	if (this->debug) std::cout << "[Workpool] Starting..." << std::endl;
	this->in_progress = 0;
	this->running = true;
	for (int i = 0; i < this->max_workers; i++) {
		if (this->debug) std::cout << "[Workpool] Creating worker thread " << i + 1 << std::endl;
		this->workers.emplace_back(&Workpool::Thread_Runner, this, i);
	}
	if (this->debug) std::cout << "[Workpool] Started" << std::endl;
}

std::queue<Workitem*>* Workpool::Stop() {
	if (!this->running) return nullptr;

	if (this->debug) std::cout << "[Workpool] Stopping..." << std::endl;
	this->quetex.lock();
	this->running = false;
	this->quetex.unlock();
	this->WaitJoin();
	this->workers.clear();
	
	if (this->debug) std::cout << "[Workpool] Stopped" << std::endl;
	return &this->work_queue;
}

void Workpool::Wait() {
	if (this->debug) std::cout << "[Workpool] Waiting for queue to empty and tasks to finish..." << std::endl;
	std::unique_lock<std::mutex> lock(this->queue_cv_mutex);
	this->queue_cv.wait(lock, [this]() {
		this->protex.lock();
		this->quetex.lock();
		bool done = this->work_queue.empty() && this->in_progress == 0;
		this->quetex.unlock();
		this->protex.unlock();
		return done;
	});
	lock.unlock();
	if (this->debug) std::cout << "[Workpool] Done waiting" << std::endl;
 }

void Workpool::AddTask(std::function<void()> call, std::function<void()> callback) {
	if (this->debug) std::cout << "[Workpool] Adding task..." << std::endl;
	this->quetex.lock();
	this->work_queue.push(new Workitem(call, callback));
	if (this->debug) std::cout << "[Workpool] Task added to queue (" << this->work_queue.size() << ")" << std::endl;
	this->quetex.unlock();
}