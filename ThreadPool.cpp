#include <mingw.thread.h>
#include <mingw.mutex.h>
#include <queue>
#include <vector>
#include <mingw.condition_variable.h>
#include <iostream>

using namespace std;
using namespace literals::chrono_literals;
using callback = void (*)(void *);

const int NUMBER = 2;

// 任务队列类
class Task
{
public:
    callback function; // 任务执行函数
    void *arg;         // 任务执行函数的参数

public:
    Task(callback f, void *arg)
    {
        function = f;
        this->arg = arg;
    }
};

// 线程池类
class ThreadPool
{
public:
    ThreadPool(int min, int max);
    // 添加任务
    void Add(callback f, void *arg);
    void Add(callback f);
    void Add(Task task);
    // 忙线程个数
    int Busynum();
    // 存活线程个数
    int Alivenum();
    ~ThreadPool();

private:
    queue<Task> taskQ;        // 任务队列
    thread managerID;         // 管理者线程ID
    vector<thread> threadIDs; // 线程批
    int minNum;               // 最小线程数
    int maxNum;               // 最大线程数
    int busyNum;              // 忙的线程数
    int liveNum;              // 存活的线程数
    int exitNum;              // 要销毁的线程数

    mutex mutexPool;                // 整个线程池的锁
    condition_variable cond;        // 任务队列是否为空,阻塞工作者线程
    bool shutdown;                  // 是否销毁线程池，销毁为1，不销毁为0
    static void manager(void *arg); // 管理者线程
    static void worker(void *arg);  // 工作线程
};

ThreadPool::ThreadPool(int min, int max)
{
    do
    {
        minNum = min;
        maxNum = max;
        busyNum = 0;
        liveNum = min;
        exitNum = 0;

        shutdown = false;
        // this:传递给线程入口函数的参数，即线程池
        managerID = thread(manager, this);

        threadIDs.resize(max);
        for (int i = 0; i < min; ++i)
        {
            threadIDs[i] = thread(worker, this);
        }
        return;
    } while (0);
}

ThreadPool::~ThreadPool()
{
    shutdown = true;
    // 阻塞回收管理者线程
    if (managerID.joinable())
        managerID.join();
    // 唤醒阻塞的消费者线程
    cond.notify_all();
    for (int i = 0; i < maxNum; ++i)
    {
        if (threadIDs[i].joinable())
            threadIDs[i].join();
    }
}

void ThreadPool::Add(Task t)
{
    unique_lock<mutex> lk(mutexPool);
    if (shutdown)
    {
        return;
    }
    // 添加任务
    taskQ.push(t);
    cond.notify_all();
}

void ThreadPool::Add(callback f, void *a)
{
    unique_lock<mutex> lk(mutexPool);
    if (shutdown)
    {
        return;
    }
    // 添加任务
    taskQ.push(Task(f, a));
    cond.notify_all();
}

void ThreadPool::Add(callback f)
{
    unique_lock<mutex> lk(mutexPool);
    if (shutdown)
    {
        return;
    }
    // 添加任务
    taskQ.push(Task(f, NULL));
    cond.notify_all();
}

int ThreadPool::Busynum()
{
    mutexPool.lock();
    int busy = busyNum;
    mutexPool.unlock();
    return busy;
}

int ThreadPool::Alivenum()
{
    mutexPool.lock();
    int alive = liveNum;
    mutexPool.unlock();
    return alive;
}

void ThreadPool::worker(void *arg)
{
    // 将arg转化为ThreadPool
    ThreadPool *pool = static_cast<ThreadPool *>(arg);
    // 工作者线程需要不停的获取线程池任务队列，所以使用while
    while (true)
    {
        // 每一个线程都需要对线程池进任务队列行操作，因此线程池是共享资源，需要加锁
        unique_lock<mutex> lk(pool->mutexPool);
        // 当前任务队列是否为空
        while (pool->taskQ.empty() && !pool->shutdown)
        {
            // 如果任务队列中任务为0，并且线程池没有被关闭，则阻拦当前工作线程，并进入cond中等待
            pool->cond.wait(lk);

            // 判断是否要销毁线程，管理者让该工作者线程自杀
            if (pool->exitNum > 0)
            {
                pool->exitNum--;
                if (pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    cout << "threadid: " << this_thread::get_id() << " exit......" << endl;
                    // 当前线程拥有互斥锁，所以需要解锁，不然会死锁
                    lk.unlock();
                    return;
                }
            }
        }
        // 判断线程池是否关闭了
        if (pool->shutdown)
        {
            cout << "threadid: " << this_thread::get_id() << "exit......" << endl;
            return;
        }

        // 从任务队列中去除一个任务
        Task task = pool->taskQ.front();
        pool->taskQ.pop();
        pool->busyNum++;
        // 当访问完线程池队列时，线程池解锁
        lk.unlock();

        // 取出Task任务后，就可以在当前线程中执行该任务了
        cout << "thread: " << this_thread::get_id() << " start working..." << endl;
        task.function(task.arg);
        free(task.arg);
        task.arg = nullptr;

        // 任务执行完毕，忙线程解锁
        cout << "thread: " << this_thread::get_id() << " end working..." << endl;
        lk.lock();
        pool->busyNum--;
        lk.unlock();
    }
}

// 检测是否需要添加线程还是销毁线程
void ThreadPool::manager(void *arg)
{
    ThreadPool *pool = static_cast<ThreadPool *>(arg);
    // 管理者线程也需要不停的监视线程池队列和工作者线程
    while (!pool->shutdown)
    {
        // 每隔3秒检测一次
        this_thread::sleep_for(chrono::seconds(3));

        // 取出线程池中任务的数量和当前线程的数量，别的线程有可能在写数据，所以需要加锁
        // 目的是添加或者销毁线程
        unique_lock<mutex> lk(pool->mutexPool);
        int queuesize = pool->taskQ.size();
        int livenum = pool->liveNum;
        int busynum = pool->busyNum;
        lk.unlock();

        // 添加线程
        // 任务的个数 > 存活的线程个数 && 存活的线程数 < 最大线程数
        if (queuesize > livenum && livenum < pool->maxNum)
        {
            // 因为在for循环中操作了线程池变量，所以需要加锁
            lk.lock();
            // 用于计数，添加的线程个数
            int count = 0;
            // 添加线程
            for (int i = 0; i < pool->maxNum && count < NUMBER && pool->liveNum < pool->maxNum; ++i)
            {
                // 判断当前线程ID，用来存储创建的线程ID
                if (pool->threadIDs[i].get_id() == thread::id())
                {
                    cout << "Create a new thread..." << endl;
                    pool->threadIDs[i] = thread(worker, pool);
                    // 线程创建完毕
                    count++;
                    pool->liveNum++;
                }
            }
            lk.unlock();
        }
        // 销毁线程：当前存活的线程太多了，工作的线程太少了
        // 忙的线程*2 < 存活的线程数 && 存活的线程数 > 最小的线程数
        if (busynum * 2 < livenum && livenum > pool->minNum)
        {
            // 访问了线程池，需要加锁
            lk.lock();
            // 一次性销毁两个
            pool->exitNum = NUMBER;
            lk.unlock();
            // 让工作的线程自杀，无法做到直接杀死空闲线程，只能通知空闲线程让它自杀
            for (int i = 0; i < NUMBER; ++i)
                pool->cond.notify_all(); // 工作线程阻塞在条件变量cond上
        }
    }
}

void taskFunc(void *arg)
{
    int nNum = *(int *)arg;
    cout << "thread: " << this_thread::get_id() << ", number=" << nNum << endl;
    this_thread::sleep_for(chrono::seconds(1));
}

void Func(void *arg)
{
    ThreadPool *pool = static_cast<ThreadPool *>(arg);
    // 往任务队列中添加10个任务
    for (int i = 0; i < 10; ++i)
    {
        int *pNum = new int(i + 100);
        pool->Add(taskFunc, (void *)pNum);
    }
}
int main()
{
    // 设置线程池最小5个线程，最大10个线程
    ThreadPool pool(5, 10);
    Func(&pool);
    this_thread::sleep_for(chrono::seconds(10));
    return 0;
}
