import requests
import time

data = {
    "img_path": "/mnt/d/Code_Space/HPC/TrWebOCR/img.png"
}

start = time.time()
# 发送 GET 请求到 http://localhost:8008/hi
response = requests.post("http://localhost:8008/api/trocr", json=data)
end = time.time()
print("Time taken:", end-start)

# 输出服务器的响应
print("Status Code:", response.status_code)
print("Response Body:", response.text)