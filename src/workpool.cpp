#include "workpool.hpp"

#include <iostream>
#include <chrono>
#include <system_error>

const int WORKER_THREAD_POLL_TIMEMS = 1;
std::mutex Workpool::shutex[10];

Workitem::Workitem(std::function<void()> call, std::function<void()> callback, std::function<void(std::exception& e)> errback) {
	this->alive = false;
	this->call = call;
	this->callback = callback;
	this->errback = errback;
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
			try {
			if (task->errback != nullptr) {
				try {
					task->call();
				} catch (std::exception& e) {
					task->errback(e);
					// Shudown all threads, clear queue, no accept new tasks, stop any waiting.
					this->quetex.lock();
					this->running = false;
					while (!this->work_queue.empty()) this->work_queue.pop();
					this->quetex.unlock();
					std::unique_lock<std::mutex> lock(this->queue_cv_mutex);
					if(this->wait_called) {
						if (this->debug) std::cout << "[Workpool-Runner] Notifying: tasks finished and queue is empty: caught exception" << std::endl;
						this->queue_cv.notify_one();
					}
					lock.unlock();
					return;
				}
			} else {
				task->call();
			}
			if (task->callback != nullptr) {
				task->callback();
			}
			if (this->debug) std::cout << "[Workpool-Runner] Finished task" << std::endl;

			// Check if all workers are idle (notify wait)
			this->protex.lock();
			this->quetex.lock();
			this->in_progress -= 1;
			all_idle = this->in_progress == 0 && this->work_queue.empty();
			this->quetex.unlock();
			this->protex.unlock();
			if (all_idle) {
				std::unique_lock<std::mutex> lock(this->queue_cv_mutex);
				if(this->wait_called) {
					if (this->debug) std::cout << "[Workpool-Runner] Notifying: tasks finished and queue is empty" << std::endl;
					this->queue_cv.notify_one();
				}
				lock.unlock();
			}
			} catch (std::exception& e) {
				std::cerr << "[Workpool-Runner] Unexpected exception: " << e.what() << std::endl;
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
	this->no_threads = false;
	this->running = false;
	this->wait_called = false;
	this->max_workers = max_workers;
	if (this->debug) std::cout << "[Workpool] Created" << std::endl;
}

Workpool::Workpool(int max_workers, bool debug, bool no_threads) {
	this->debug = debug;
	this->no_threads = no_threads;
	this->running = false;
	this->max_workers = max_workers;
	if (this->debug) std::cout << "[Workpool] Created" << std::endl;
}

void Workpool::start() {
	int i = 0;
	if (this->running) return;

	if (this->debug) std::cout << "[Workpool] Starting (" << this->max_workers << ")..." << std::endl;
	this->in_progress = 0;
	this->running = true;
	if (!this->no_threads) {
		for (i = 0; i < this->max_workers; i++) {
			if (this->debug) std::cout << "[Workpool] Creating worker thread " << i + 1 << std::endl;
			try {
				this->workers.emplace_back(&Workpool::Thread_Runner, this, i);
			} catch (std::system_error) {
				if (this->debug) std::cout << "[Workpool] Failed to create more threads" << std::endl;
				break;
			}
		}
	} else i = this->max_workers;
	if (i != this->max_workers) {
		std::cerr << "[Workpool] Thread pool limited at " << i << std::endl;
		this->stop();
		// Limit theards to 50% of limit reached (to nearest 100)
		this->max_workers = (int)(i * 0.005f) * 100;
		// Auto disable threading
		if (this->max_workers == 0) {
			std::cout << "Unable to use threads, processing will take much longer" << std::endl;
			this->no_threads = true;
		}
		return this->start();
	}
	if (this->debug) std::cout << "[Workpool] Started" << std::endl;
}

std::queue<Workitem*>* Workpool::stop() {
	if (this->debug) std::cout << "[Workpool] Stopping..." << std::endl;
	if (!this->no_threads) {
		this->quetex.lock();
		this->running = false;
		this->quetex.unlock();
		this->WaitJoin();
		this->workers.clear();
	} else {
		this->running = false;
	}
	if (this->debug) std::cout << "[Workpool] Stopped" << std::endl;
	return &this->work_queue;
}

void Workpool::wait() {
	if (this->no_threads || !this->running) return;
	if (this->debug) std::cout << "[Workpool] Waiting for queue to empty and tasks to finish..." << std::endl;

	// Check if wait is necessary
	this->protex.lock();
	this->quetex.lock();
	bool all_idle = this->work_queue.size() == 0 && this->in_progress == 0;
	this->quetex.unlock();
	this->protex.unlock();
	if (all_idle) return;

	std::unique_lock<std::mutex> lock(this->queue_cv_mutex);
	this->wait_called = true;
	this->queue_cv.wait(lock);
	this->wait_called = false;
	lock.unlock();
	if (this->debug) std::cout << "[Workpool] Done waiting" << std::endl;
 }

void Workpool::addTask(std::function<void()> call, std::function<void()> callback, std::function<void(std::exception& e)> errback) {
	if (!this->running) return;
	if (this->no_threads) {
		if (errback != nullptr) {
			try {
				call();
			} catch (std::exception& e) {
				errback(e);
			}
		} else {
			call();
		}
		if (callback != nullptr) {
			callback();
		}
		return;
	}
	if (this->debug) std::cout << "[Workpool] Adding task..." << std::endl;
	std::lock_guard<std::mutex> lock(this->quetex);
	this->work_queue.push(new Workitem(call, callback, errback));
	if (this->debug) std::cout << "[Workpool] Task added to queue (" << this->work_queue.size() << ")" << std::endl;
}