#include <iostream> 
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

class ResourceManager {
private:
    std::mutex mtx;
    std::condition_variable cv;
    
    // Current color accessing the resource
    enum Color { NONE, WHITE, BLACK } current_color = NONE;
    
    // Synchronization variables
    std::atomic<int> active_threads{0};
    std::atomic<int> next_thread_id{0};

public:
    void acquire(bool is_white, int thread_id) {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Wait until it's this thread's turn
        cv.wait(lock, [&]() { 
            return next_thread_id == thread_id; 
        });
        
        // Wait for resource access conditions
        cv.wait(lock, [&]() {
            return (current_color == NONE || 
                    current_color == (is_white ? WHITE : BLACK));
        });
        
        // Update current color and activate thread
        if (current_color == NONE) {
            current_color = is_white ? WHITE : BLACK;
        }
        active_threads++;
    }
    
    void release(bool is_white, int thread_id) {
        std::unique_lock<std::mutex> lock(mtx);
        
        active_threads--;
        
        // Reset color if no active threads
        if (active_threads == 0) {
            current_color = NONE;
        }
        
        // Move to next thread
        next_thread_id = (thread_id + 1) % 10;
        
        // Notify waiting threads
        cv.notify_all();
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