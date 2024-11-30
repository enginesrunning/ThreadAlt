#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>

class ResourceManager {
private:
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cv;

    // Current color accessing the resource
    enum Color { NONE, WHITE, BLACK } current_color = NONE;

    // Synchronization variables
    std::atomic<int> active_threads{0};
    std::atomic<int> next_thread_id{0};

public:
    ResourceManager() {
        InitializeCriticalSection(&cs);
        InitializeConditionVariable(&cv);
    }

    ~ResourceManager() {
        DeleteCriticalSection(&cs);
    }

    void acquire(bool is_white, int thread_id) {
        EnterCriticalSection(&cs);

        // Wait until it's this thread's turn
        while (next_thread_id != thread_id) {
            SleepConditionVariableCS(&cv, &cs, INFINITE);
        }

        // Wait for resource access conditions
        while (!(current_color == NONE ||
                 current_color == (is_white ? WHITE : BLACK))) {
            SleepConditionVariableCS(&cv, &cs, INFINITE);
        }

        // Update current color and activate thread
        if (current_color == NONE) {
            current_color = is_white ? WHITE : BLACK;
        }
        active_threads++;

        LeaveCriticalSection(&cs);
    }

    void release(bool is_white, int thread_id) {
        EnterCriticalSection(&cs);

        active_threads--;

        // Reset color if no active threads
        if (active_threads == 0) {
            current_color = NONE;
        }

        // Move to next thread
        next_thread_id = (thread_id + 1) % 10;

        // Wake up waiting threads
        WakeAllConditionVariable(&cv);

        LeaveCriticalSection(&cs);
    }

    void use_resource(bool is_white) {
        std::cout << (is_white ? "White" : "Black")
                  << " thread is using the resource." << std::endl;

        // Simulate resource usage
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};

void thread_function(ResourceManager& rm, bool is_white, int id) {
    std::cout << (is_white ? "White" : "Black")
              << " thread " << id << " wants to access resource." << std::endl;

    rm.acquire(is_white, id);
    rm.use_resource(is_white);
    rm.release(is_white, id);

    std::cout << (is_white ? "White" : "Black")
              << " thread " << id << " released resource." << std::endl;
}

int main() {
    ResourceManager resource_manager;
    const int NUM_THREADS = 10;
    std::vector<std::thread> threads;

    // Create mixed white and black threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(thread_function, std::ref(resource_manager),
                              i % 2 == 0, i);
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
