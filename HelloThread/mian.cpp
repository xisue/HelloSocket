// HelloThread.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include<thread>
using namespace std;
void workFun()
{
    std::cout << "Hello ,other thread!\n";
}
int main()
{
    thread t(workFun);
   // t.join();

    std::cout << "Hello ,main thread!\n";
}
