#include <thread>
#include <iostream>
#include <chrono>



//void ThreadTest() {
//	// std::this_thread::get_id() 获取当前线程ID
//	std::cout << "Main thread id: " << std::this_thread::get_id() << std::endl;
//	// 线程创建启动
//	std::thread t1(ThreadInstance1);
//	// 此处 主线程阻塞等待子线程退出
//	t1.join();
//
//	return;
//}

class ThreadTestClass {
    ThreadTestClass();
public:
    static void ThreadInstance1();
};


void ThreadTestClass::ThreadInstance1() {
	std::cout << "Begin ThreadInstance_1" << std::endl;
	// sleep释放CPU控制权 而不是一直循环 占用CPU
	for (int i = 0; i < 10; i++) {
		std::this_thread::sleep_for(std::chrono::seconds(1)); // 
		std::cout << std::this_thread::get_id() << "-> Time: " << i << std::endl;
	}
}