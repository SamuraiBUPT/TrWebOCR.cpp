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

# 计算均值
mean_time1 = sum(times) / len(times)
mean_time2 = sum(times2) / len(times2)

# 绘制柱状图
labels = ['TrWebOCR Time (ms)', 'C++ Time (μs)']
mean_values = [mean_time1, mean_time2]

plt.figure(figsize=(8, 6))
plt.bar(labels, mean_values, color=['blue', 'green'])

# 添加标题和标签
plt.title('Average Time Comparison')
plt.ylabel('Average Time')

plt.savefig('average_time_comparison.png')