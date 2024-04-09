#include "main.h"

#include <random>
std::random_device randomDevice;
std::mt19937_64 mt(randomDevice());
std::uniform_int_distribution<int> distribution(-1000,1000);
auto rnd = std::bind(distribution,mt);
void sleep_(){
    std::this_thread::sleep_for(std::chrono::milliseconds(2000 + rnd()));
}



void add(int a,int b){
    sleep_();
    int t = a * b;
    std::cout << a << "*" << b << "=" << t << "\t";
}

int main(){
    ABC::thread_pool pool(3);// 创建线程池

    for (int i = 1; i < 10; ++i) {
        for (int j = i; j < 10; ++j) {
            pool.submit(add,i,j).get();
        }
        std::cout << "\n";
    }


    return 0;
}