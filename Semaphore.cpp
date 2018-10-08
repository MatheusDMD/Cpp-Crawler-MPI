#include <condition_variable>
#include <mutex>

class Semaphore
{
private:
    std::condition_variable isGreen;
    std::mutex mtx;
    int count;
public:
    Semaphore(int count);
    ~Semaphore();
    void acquire();
    void release();
};

Semaphore::Semaphore(int count) : count(count){}

Semaphore::~Semaphore(){}

void Semaphore::acquire()
{
    {
        std::unique_lock<std::mutex> lk(mtx);
        isGreen.wait(lk, [this]{return count > 0; });
        count--;
    }
}

void Semaphore::release()
{
    {
        std::unique_lock<std::mutex> lk(mtx);
        count++;
    }
    isGreen.notify_one();
}