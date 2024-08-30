#include <stdio.h>
#include <fstream>
#include <iostream>
#include <chrono>

#include "tr_wrapper.h"
#include "tr_worker.h"
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace httplib;

void test_inference() {
    int CV_TYPE = 0;
    int flag = 2;
    int max_lines = 512;
    int max_width = 512;

    auto allocate_start = std::chrono::high_resolution_clock::now();
    float* rect = new float[max_lines * 6]();
    int* unicode = new int[max_lines * max_width]();
    float* prob = new float[max_lines * max_width]();
    auto allocate_end = std::chrono::high_resolution_clock::now();

    const char* img_path = "../img.png";

    // int line_num = tr_run(0, 1, 
    //     img, h, w, CV_TYPE, 
    //     flag, 
    //     rect, max_lines,
    //     unicode, prob, max_width);

    auto start = std::chrono::high_resolution_clock::now();

    int line_num = tr_run_image_from_local(img_path, 0, 1, rect, unicode, prob);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "耗时：" << duration.count() / 1000.0 << "毫秒" << std::endl;

    std::cout << "检测到 " << line_num << " 行文本。" << std::endl;

    std::vector<TrResult> results = process_results(line_num, rect, unicode, prob);

    for (const auto& result : results) {
        const auto& rect = std::get<0>(result);
        const auto& txt = std::get<1>(result);
        const auto& confidence = std::get<2>(result);

        std::cout << "Rect: ";
        for (float r : rect) {
            std::cout << r << " ";
        }
        std::cout << ", Text: " << txt << ", Confidence: " << confidence << std::endl;
    }


    // 使用完成后释放内存
    auto deallocate_start = std::chrono::high_resolution_clock::now();
    delete[] rect;
    delete[] unicode;
    delete[] prob;
    auto deallocate_end = std::chrono::high_resolution_clock::now();

    auto allocate_duration = std::chrono::duration_cast<std::chrono::microseconds>(allocate_end - allocate_start);
    auto deallocate_duration = std::chrono::duration_cast<std::chrono::microseconds>(deallocate_end - deallocate_start);
    std::cout << "内存分配耗时：" << allocate_duration.count() / 1000.0 << "毫秒" << std::endl;
    std::cout << "内存释放耗时：" << deallocate_duration.count() / 1000.0 << "毫秒" << std::endl;
};

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

int main() {
    printf("Initializing TR binary...\n");
    int ctpn_id = 0;
    int crnn_id = 1;

    tr_init(0, ctpn_id, (void*)(CTPN_PATH), NULL);
    tr_init(0, crnn_id, (void*)(CRNN_PATH), NULL);

    // test_inference();

    TrThreadPool tr_task_pool(6);

    int port = 8008;
    Server svr;

    svr.Get("/hi", [](const Request& req, Response& res) {
        res.set_content("Hello World!", "text/plain");
    });

    svr.Post("/api/trocr", [&tr_task_pool, ctpn_id, crnn_id](const Request& req, Response& res) {
        try {
            // 解析请求体中的 JSON 数据
            auto json_data = json::parse(req.body);

            if (json_data.contains("img_path")) {
                auto img_path = json_data["img_path"].get<std::string>();

                auto start = std::chrono::high_resolution_clock::now();
                auto future = tr_task_pool.enqueue(img_path.c_str(), ctpn_id, crnn_id);

                std::vector<TrResult> results = future.get();   // inference
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                std::cout << "time: " << duration.count() / 1000.0 << " ms" << std::endl;
                
                std::string result_str;
                for (const auto& result : results) {
                    std::vector<float> rect = std::get<0>(result);
                    std::string txt = std::get<1>(result);
                    float confidence = std::get<2>(result);
                    result_str += txt + "\n";
                }
                
                res.set_content(result_str, "text/plain");
            } else {
                res.set_content("Missing img_path parameter", "text/plain");
            }
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