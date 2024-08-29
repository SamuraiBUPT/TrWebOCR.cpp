import requests
import time

# 发送 GET 请求到 http://localhost:8008/hi
start = time.time()
response = requests.get("http://localhost:8008/hi")
end = time.time()

# 输出服务器的响应
print("Status Code:", response.status_code)
print("Response Body:", response.text)