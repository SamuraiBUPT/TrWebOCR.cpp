#include <stdio.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <vector>
#include <cuda_runtime.h>

// use opencv
#include <opencv2/opencv.hpp>

#include "tr_wrapper.h"
#include "tr_worker.h"
#include "tr_utils.h"

// third party libraries
#include "httplib.h"
#include "json.hpp"
#include "base64.h"

using json = nlohmann::json;
using namespace httplib;

void tr_log(std::string& msg, std::string& level) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time_t));
    std::string time_str = std::string(buffer);

    printf("[%s] [%s] %s\n", time_str.c_str(), level.c_str(), msg.c_str());
}

bool contains_keywords(const std::string& plain_text) {
    return plain_text.find("年") != std::string::npos ||
           plain_text.find("登记") != std::string::npos ||
           plain_text.find("统一") != std::string::npos ||
           plain_text.find("营") != std::string::npos;
}

void clear_memory() {
    cudaError_t err = cudaDeviceReset();
    if (err != cudaSuccess) {
        std::cerr << "Error resetting CUDA device: " << cudaGetErrorString(err) << std::endl;
    }
}

void resume_tr_libs(int ctpn_id, int crnn_id) {
    clear_memory();
    tr_init(0, ctpn_id, (void*)(CTPN_PATH), NULL);
    tr_init(0, crnn_id, (void*)(CRNN_PATH), NULL);
}

int main() {
    printf("Initializing TR binary...\n");
    int ctpn_id = 0;
    int crnn_id = 1;

    resume_tr_libs(ctpn_id, crnn_id);
    
    // test_inference();

    TrThreadPool tr_task_pool(12);

    int port = 6006;
    Server svr;

    svr.Get("/hi", [](const Request& req, Response& res) {
        res.set_content("Hello World!", "text/plain");
    });

    std::vector<int> rotations = {0, 90, 270, 180};

    svr.Post("/api/trocr", [&tr_task_pool, ctpn_id, crnn_id, &rotations](const Request& req, Response& res) {
        try {
            int width, height, channels;
            cv::Mat img;

            if (req.has_file("file")) {
                const auto& file = req.get_file_value("file");
                // 从内存中读取图像
                std::vector<unsigned char> img_data(file.content.begin(), file.content.end());
                img = cv::imdecode(img_data, cv::IMREAD_COLOR);  // 读取为彩色图像
            } else if (req.has_param("img_base64")) {
                std::string img_base64 = req.get_param_value("img_base64");
                std::string decoded_data = base64_decode(img_base64);  // 解码Base64数据

                // 将Base64解码的数据转为vector并加载为彩色图像
                std::vector<unsigned char> img_data(decoded_data.begin(), decoded_data.end());
                img = cv::imdecode(img_data, cv::IMREAD_COLOR);  // 读取为彩色图像
            } else {
                res.set_content("Missing file or img_base64 parameter", "text/plain");
                return;
            }

            if (img.empty()) {
                res.set_content("Failed to load image", "text/plain");
                return;
            }

            // 转为灰度图像
            cv::Mat gray_img;
            cv::cvtColor(img, gray_img, cv::COLOR_BGR2GRAY);

            // 现在开始核心的推理进程，旋转图像来看看到底是不是有用的数据
            auto start = std::chrono::high_resolution_clock::now();
            std::string plain_text;
            bool direction_is_right = false;

            for (int rotation:rotations) {
                // 开始旋转
                cv::Mat rotated_img;
                plain_text = "";
                if (rotation != 0) {
                    // 获取旋转矩阵
                    cv::Point2f center(gray_img.cols / 2.0, gray_img.rows / 2.0);
                    cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, rotation, 1.0);

                    // 进行旋转，expand 参数相当于在这里选择大一点的边界来容纳旋转后的图像
                    cv::warpAffine(gray_img, rotated_img, rotation_matrix, gray_img.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
                } else {
                    rotated_img = gray_img.clone();
                }

                int width = rotated_img.cols;
                int height = rotated_img.rows;
                int channels = rotated_img.channels();
                unsigned char* image_data = rotated_img.data;
                auto future = tr_task_pool.enqueue(image_data, height, width, channels, ctpn_id, crnn_id);
                std::vector<TrResult> results = future.get();   // inference

                // 处理 OCR 结果
                for (const auto& result : results) {
                    std::string txt = std::get<1>(result);
                    plain_text += txt + "|";
                }

                // 检查关键词
                if (contains_keywords(plain_text)) {
                    direction_is_right = true;
                    break;  // 方向正确，停止旋转
                }
            }

            if (!direction_is_right) {
                plain_text += "-----------问题数据-----------";
            }

            res.set_content(plain_text, "text/plain; charset=UTF-8");

            // log
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::string msg = std::string("Latency: ") + std::to_string(duration.count() / 1000.0) + " ms";
            std::string level("INFO");
            tr_log(msg, level);


        } catch (const std::exception& e) {
            res.set_content("Invalid JSON data", "text/plain");
        }
    });


    printf("Server listening on http://localhost:%d\n", port);

    svr.listen("localhost", port);

    return 0;
}