import requests

# 发送 GET 请求到 http://localhost:8008/hi
response = requests.get("http://localhost:8008/hi")

# 输出服务器的响应
print("Status Code:", response.status_code)
print("Response Body:", response.text)