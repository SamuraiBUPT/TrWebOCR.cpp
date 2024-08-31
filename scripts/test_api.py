import requests

# 服务器的URL
url = "http://localhost:8008/api/trocr"  # 根据实际情况替换URL

# 需要上传的图像文件路径
file_path = "./img.png"  # 替换为你的图像文件路径

# 打开文件并发送请求
with open(file_path, 'rb') as f:
    files = {'file': f}
    response = requests.post(url, files=files)

# 打印服务器响应
print(f"Status code: {response.status_code}")
print(f"Response body: {response.text}")