#include <iostream>
#include <iomanip>  // 用于格式化输出

#include "tr_worker.h"
#include "tr_wrapper.h"

// 将 Unicode 码点转换为字符
inline char unichr(int unicode) {
    return static_cast<char>(unicode); // 对于宽字符，使用 wchar_t 和 wstring
}

std::string unichr_utf8(int unicode) {
    std::string result;
    if (unicode <= 0x7F) {
        result += static_cast<char>(unicode);
    } else if (unicode <= 0x7FF) {
        result += static_cast<char>(0xC0 | ((unicode >> 6) & 0x1F));
        result += static_cast<char>(0x80 | (unicode & 0x3F));
    } else if (unicode <= 0xFFFF) {
        result += static_cast<char>(0xE0 | ((unicode >> 12) & 0x0F));
        result += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (unicode & 0x3F));
    } else if (unicode <= 0x10FFFF) {
        result += static_cast<char>(0xF0 | ((unicode >> 18) & 0x07));
        result += static_cast<char>(0x80 | ((unicode >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (unicode & 0x3F));
    }
    return result;
}

std::pair<std::string, float> parse(const int* unicode_arr, const float* prob_arr, int num) {
    std::string txt;
    float prob = 0.0;
    int unicode_pre = -1;
    int count = 0;

    for (int pos = 0; pos < num; ++pos) {
        int unicode = unicode_arr[pos];
        if (unicode >= 0) {
            if (unicode != unicode_pre) {
                txt += unichr_utf8(unicode);  // 将 Unicode 转换为 UTF-8 编码
            }
            count += 1;
            prob += prob_arr[pos];
        }
        unicode_pre = unicode;
    }

    float confidence = prob / std::max(count, 1);  // 计算平均置信度

    return {txt, confidence};
}

std::vector<TrResult> process_results(int line_num, float* rect_arr, int* unicode_arr, float* prob_arr) {
    std::vector<TrResult> results;

    for (int i = 0; i < line_num; ++i) {
        int num = static_cast<int>(rect_arr[i * 6 + 5] + 0.5);  // 计算 num

        // TODO: 这个地方512不是固定的，要改
        auto [txt, confidence] = parse(&unicode_arr[i * 512], &prob_arr[i * 512], num);

        // 6是RECT的固定width，相当于一个leading dimension
        std::vector<float> rect(rect_arr + i * 6, rect_arr + i * 6 + 5);  // 提取矩形区域
        results.emplace_back(rect, txt, confidence);
    }

    return results;
}

void TrThreadPool::start(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([=]() {
            // 每个线程持有的内存
            // TODO: 这里需要换成vector，方便做扩容、缩容。
            float* rect = new float[512 * 6](); // 假设最大行数为 512，RECT_SIZE 为 6
            int* unicode = new int[512 * 512](); // 假设最大行数为 512，max_width 为 512
            float* prob = new float[512 * 512](); // 同上

            while (true) {
                TrTask task;
                PromiseResult promise;

                {
                    std::unique_lock<std::mutex> lock(eventMutex);

                    eventVar.wait(lock, [=]() { return stopFlag || !task_queue.empty(); });

                    if (stopFlag && task_queue.empty())
                        break;

                    task = std::move(task_queue.front().first);
                    promise = std::move(task_queue.front().second);
                    task_queue.pop();
                }

                // 清空内存
                std::fill(rect, rect + 512 * 6, 0.0f);
                std::fill(unicode, unicode + 512 * 512, 0);
                std::fill(prob, prob + 512 * 512, 0.0f);

                // 执行任务，传递给 task 函数
                std::vector<TrResult> results = task(rect, unicode, prob);

                if (promise) {
                    promise->set_value(results);  // 将结果设置给 promise
                }
            }

            // 释放内存
            delete[] rect;
            delete[] unicode;
            delete[] prob;
        });
    }
};

void TrThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(eventMutex);
        stopFlag = true;
    }

    eventVar.notify_all();

    for (std::thread &thread : threads)
        thread.join();
}

std::future<std::vector<TrResult>> TrThreadPool::enqueue(const char* image_path, int ctpn_id, int crnn_id) {
    auto promise = std::make_shared<std::promise<std::vector<TrResult>>>();
    auto future = promise->get_future();

    {
        std::unique_lock<std::mutex> lock(eventMutex);
        task_queue.emplace([=](float* rect, int* unicode, float* prob) -> std::vector<TrResult>{
            int line_num = tr_run_image_from_local(image_path, ctpn_id, crnn_id, rect, unicode, prob);
            std::vector<TrResult> results = process_results(line_num, rect, unicode, prob);
            // promise->set_value(results);  // 将结果设置给 promise
            return results;
        }, promise);
    }
    eventVar.notify_one();
    return future;
}

std::future<std::vector<TrResult>> TrThreadPool::enqueue(unsigned char* image_data, int height, int width, int channels, int ctpn_id, int crnn_id) {
    auto promise = std::make_shared<std::promise<std::vector<TrResult>>>();
    auto future = promise->get_future();

    {
        std::unique_lock<std::mutex> lock(eventMutex);
        task_queue.emplace([=](float* rect, int* unicode, float* prob) -> std::vector<TrResult>{
            int CV_TYPE = channels == 1 ? 0 : 16;    // TODO: modify here
            // printf("CTYPE is: %d\n", CV_TYPE);
            int rotate_flag = 2;
            int line_num = tr_run_image_from_ndarray(image_data, ctpn_id, crnn_id, 
                                                        height, width, CV_TYPE, 
                                                        rotate_flag, 
                                                        rect, 512,
                                                        unicode, prob, 512);  
            std::vector<TrResult> results = process_results(line_num, rect, unicode, prob);

            // std::cout << "line_num: " << line_num << std::endl;
            return results;
        }, promise);
    }
    eventVar.notify_one();
    return future;
}

TrThreadPool::TrThreadPool(size_t numThreads) {
    start(numThreads);
}

TrThreadPool::~TrThreadPool() {
    stop();
}
