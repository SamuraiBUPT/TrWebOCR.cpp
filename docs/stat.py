import re
import matplotlib.pyplot as plt

# 读取txt文件内容为字符串
with open('log_trwebocr.txt', 'r', encoding='utf-8') as file:
    content = file.read()

# 定义正则表达式模式，匹配类似336.70ms和520.29ms的时间信息
pattern = r'200 POST /api/tr-run/ \(127\.0\.0\.1\) (\d+\.\d+)ms'

# 使用re.findall()方法提取所有符合条件的时间信息
times = re.findall(pattern, content)

# 输出结果
print(times)
times = [float(time) for time in times]  # 转换为浮点数

with open('log_cpp_0829.txt', 'r', encoding='utf-8') as file:
    content = file.read()

pattern = r'耗时：(\d+\.\d+)微秒'

times2 = re.findall(pattern, content)

print(times2)
times2 = [float(time) for time in times2]  # 转换为浮点数

# 设置横轴为请求数目
x_values = list(range(1, len(times) + 1))
x_values2 = list(range(1, len(times2) + 1))

# 绘制折线图
plt.figure(figsize=(10, 6))
plt.plot(x_values, times, label='TrWebOCR Time (ms)', marker='o')
plt.plot(x_values2, times2, label='C++ Time (ms)', marker='o')

# 添加标题和标签
plt.title('Time Comparison between TrWebOCR and C++')
plt.xlabel('Request Number')
plt.ylabel('Time')
plt.legend()

plt.yticks([])

plt.savefig('time_comparison.png')