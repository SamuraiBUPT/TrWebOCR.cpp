import asyncio
import aiohttp
import time

async def send_request(session, url, data, semaphore):
    async with semaphore:  # 使用信号量来控制并发数量
        async with session.post(url, json=data) as response:
            status = response.status
            response_text = await response.text()
            # print(f"Status Code: {status}, Response Body: {response_text[:100]}")  # 打印前100字符的响应
            return status, response_text

async def main():
    url = "http://localhost:8008/api/trocr"
    data = {
        "img_path": "/mnt/d/Code_Space/HPC/img.png"
    }

    semaphore = asyncio.Semaphore(20)  # 信号量设置为 20，控制并发数量
    tasks = []

    # 创建一个 session
    async with aiohttp.ClientSession() as session:
        for i in range(100):  # 发送 1000 个请求
            task = asyncio.create_task(send_request(session, url, data, semaphore))
            tasks.append(task)

        # 等待所有任务完成
        await asyncio.gather(*tasks)

if __name__ == "__main__":
    start = time.time()
    asyncio.run(main())
    end = time.time()
    print("Total Time taken:", end - start)
