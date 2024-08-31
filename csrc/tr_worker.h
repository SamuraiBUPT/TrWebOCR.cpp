#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <tuple>

#include <future>

// 任务结果：Tuple类型
using TrResult = std::tuple<std::vector<float>, std::string, float>;
// 任务类型, 这是一个函数指针
using TrTask = std::function<std::vector<TrResult>(float*, int*, float*)>;
using PromiseResult = std::shared_ptr<std::promise<std::vector<TrResult>>>;



std::pair<std::string, float> parse(const int* unicode_arr, const float* prob_arr, int num);
std::vector<TrResult> process_results(int line_num, float* rect_arr, int* unicode_arr, float* prob_arr);

class TrThreadPool {
    public:
        TrThreadPool(size_t numThreads);
        ~TrThreadPool();

        // the task entrance
        std::future<std::vector<TrResult>> enqueue(const char* image_path, int ctpn_id, int crnn_id);
        // uint8_t类型的图像数据，实际上，就是unsigned char*，因为底层都是一样的
        std::future<std::vector<TrResult>> enqueue(unsigned char* image_data, int width, int height, int channels, int ctpn_id, int crnn_id);   
    
    private:
        std::vector<std::thread> threads;
        std::condition_variable eventVar;
        std::mutex eventMutex;

        std::queue<std::pair<TrTask, PromiseResult>> task_queue;

        std::atomic_bool stopFlag{false};

        void start(size_t numThreads);
        void stop();
};