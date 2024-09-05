#include <stdio.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <vector>

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

std::string dump_headers(const Headers &headers) {
    std::string s;
    char buf[BUFSIZ];

    for (const auto &x : headers) {
        snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
        s += buf;
    }

    return s;
}

std::string dump_multipart_files(const MultipartFormDataMap &files) {
    std::string s;
    char buf[BUFSIZ];

    s += "--------------------------------\n";

    for (const auto &x : files) {
        const auto &name = x.first;
        const auto &file = x.second;

        snprintf(buf, sizeof(buf), "name: %s\n", name.c_str());
        s += buf;

        snprintf(buf, sizeof(buf), "filename: %s\n", file.filename.c_str());
        s += buf;

        snprintf(buf, sizeof(buf), "content type: %s\n", file.content_type.c_str());
        s += buf;

        snprintf(buf, sizeof(buf), "text length: %zu\n", file.content.size());
        s += buf;

        s += "----------------\n";
    }

    return s;
}

std::string log(const Request &req, const Response &res) {
    std::string s;
    char buf[BUFSIZ];

    s += "================================\n";

    snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
            req.version.c_str(), req.path.c_str());
    s += buf;

    std::string query;
    for (auto it = req.params.begin(); it != req.params.end(); ++it) {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%c%s=%s",
                (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
                x.second.c_str());
        query += buf;
    }
    snprintf(buf, sizeof(buf), "%s\n", query.c_str());
    s += buf;

    s += dump_headers(req.headers);
    s += dump_multipart_files(req.files);

    s += "--------------------------------\n";

    snprintf(buf, sizeof(buf), "%d\n", res.status);
    s += buf;
    s += dump_headers(res.headers);

    return s;
}

void tr_log(std::string& msg, std::string& level) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time_t));
    std::string time_str = std::string(buffer);

    printf("[%s] [%s] %s\n", time_str.c_str(), level.c_str(), msg.c_str());
}

int main() {
    printf("Initializing TR binary...\n");
    int ctpn_id = 0;
    int crnn_id = 1;

    tr_init(0, ctpn_id, (void*)(CTPN_PATH), NULL);
    tr_init(0, crnn_id, (void*)(CRNN_PATH), NULL);

    // test_inference();

    TrThreadPool tr_task_pool(6);

    int port = 6006;
    Server svr;

    svr.Get("/hi", [](const Request& req, Response& res) {
        res.set_content("Hello World!", "text/plain");
    });

    svr.Post("/api/trocr", [&tr_task_pool, ctpn_id, crnn_id](const Request& req, Response& res) {
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

            width = gray_img.cols;
            height = gray_img.rows;
            channels = gray_img.channels();

            unsigned char* image_data = gray_img.data;  // 获取图像数据指针


            // 现在开始处理图像数据
            // 调用任务池中的任务接口（假设任务接口能够处理 image_data 指针）
            auto start = std::chrono::high_resolution_clock::now();
            auto future = tr_task_pool.enqueue(image_data, height, width, channels, ctpn_id, crnn_id);
            std::vector<TrResult> results = future.get();   // inference
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            // log
            std::string msg = std::string("Latency: ") + std::to_string(duration.count() / 1000.0) + " ms";
            std::string level("INFO");
            tr_log(msg, level);

            // 处理结果并返回响应
            std::string result_str;
            for (const auto& result : results) {
                std::vector<float> rect = std::get<0>(result);
                std::string txt = std::get<1>(result);
                // std::cout << txt << std::endl;
                float confidence = std::get<2>(result);
                result_str += txt + "\n";
            }
            res.set_content(result_str, "text/plain; charset=UTF-8");

        } catch (const std::exception& e) {
            res.set_content("Invalid JSON data", "text/plain");
        }
    });

    // svr.set_logger(
    //   [](const Request &req, const Response &res) { std::cout << log(req, res); });

    printf("Server listening on http://localhost:%d\n", port);

    svr.listen("localhost", port);

    return 0;
}