#include <thread>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <memory>

/*----------------------------------------------------------------------*/
// 这个类传给一个线程做参数时(代码中p2变量); 
/*
* 这里创建一次 拷贝两次 清理两次的原因: 少一次销毁是因为这个p2变量还没有被释放(在主程序中)
* 但是拷贝两次 第一次是因为这个类要传给 std::thread 对象一次, 给线程启动时使用
* 第二次拷贝是 std::thread 对象把数据拷贝给线程启动时进行使用
ParamTest Create!
ParamTest Copied!
ParamTest Destroyed!
ParamTest Copied!
ThreadInstance2 -> ID: 1, Str: aaa
Begin ThreadInstance_1
========================================
OpenGL Vendor:   NVIDIA Corporation
OpenGL Renderer: NVIDIA GeForce RTX 4070 Ti SUPER/PCIe/SSE2
OpenGL Version:  3.3.0 NVIDIA 560.94
GLSL Version:    3.30 NVIDIA via Cg compiler
========================================
ParamTest Destroyed!
ParamTest Destroyed!
*/

class ParamTest {
public:
	ParamTest() { std::cout << "ParamTest Create!"<<std::endl; }
	// 在 C++ 里，“以自身类型的 const& 为唯一参数的构造函数”，语义上就是拷贝构造函数。-> 定义成这样就是自己写了一个拷贝改造函数
	// 创建了自己的拷贝构造函数之后, 默认的拷贝构造函数就会被覆盖
	ParamTest(const ParamTest& p) { std::cout << "ParamTest Copied!" << std::endl; }
	~ParamTest() { std::cout << "ParamTest Destroyed!" << std::endl; }

	std::string name;
};
/*----------------------------------------------------------------------*/

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

/*----------------------------------------------------------------------*/
class ThreadTestClass {
public:
    ThreadTestClass();
	~ThreadTestClass();

	std::vector<std::shared_ptr<std::thread>> threads;

    static void ThreadInstance1();
	void ThreadInstance2_ParamTransform(int id, std::string str, ParamTest p);
};

/*
* 整个功能测试的构造函数, 将所有要创建的线程填入 vector 中, 析构时 join() 同步
*/

ThreadTestClass::ThreadTestClass() {
	// 创建即启动
	// 使用 make_shared 创建 shared_ptr<thread>
	// 成员函数需要传递 &ClassName::FunctionName 和 this 指针
	int t2_intParam = 1;
	//ParamTest *p1 = new ParamTest();		// 需要传递 *p1
	ParamTest p2;		// 直接传递p2
	this->threads.push_back(
		std::make_shared<std::thread>(
			// thread 源码中使用Template应对多参数 而且所有的参数都是复制; 所以这里的 t2_intParam 就算被释放也不会让子线程出错
			//&ThreadTestClass::ThreadInstance2_ParamTransform, this, t2_intParam, "aaa", *p1
			&ThreadTestClass::ThreadInstance2_ParamTransform, this, t2_intParam, "aaa", p2
		)
	);
}

ThreadTestClass::~ThreadTestClass() {
	for (auto& i : threads) {
		if (i->joinable()) {
			i->join();
		}
	}
}


void ThreadTestClass::ThreadInstance1() {
	std::cout << "Begin ThreadInstance_1" << std::endl;
	// sleep释放CPU控制权 而不是一直循环 占用CPU
	for (int i = 0; i < 10; i++) {
		std::this_thread::sleep_for(std::chrono::seconds(1)); 
		//std::cout << std::this_thread::get_id() << "-> Time: " << i << std::endl;
	}
}

void ThreadTestClass::ThreadInstance2_ParamTransform(int id, std::string str, ParamTest p) {
	std::cout << "ThreadInstance2 -> ID: " << id << ", Str: " << str << std::endl;
	for (int i = 0; i < 10; i++) {
		std::this_thread::sleep_for(std::chrono::seconds(1)); 
		//std::cout << str << std::this_thread::get_id() << "-> Time: " << i << std::endl;
	}
}