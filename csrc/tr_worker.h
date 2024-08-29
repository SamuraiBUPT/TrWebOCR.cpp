#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <tuple>

// 任务类型, 这是一个函数指针
using TrTask = std::function<int(float*, int*, float*)>;
using Result = std::tuple<std::vector<float>, std::string, float>;

std::pair<std::string, float> parse(const int* unicode_arr, const float* prob_arr, int num);
std::vector<Result> process_results(int line_num, float* rect_arr, int* unicode_arr, float* prob_arr);

class TrThreadPool {
    public:
        TrThreadPool(size_t numThreads);
        ~TrThreadPool();
        void enqueue(TrTask task, const char* image_path, int ctpn_id, int crnn_id);
    
    private:
        std::vector<std::thread> threads;
        std::condition_variable eventVar;
        std::mutex eventMutex;

        std::queue<TrTask> tasks;

        std::atomic_bool stopFlag{false};

        void start(size_t numThreads);
        void stop();
};