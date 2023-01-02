#include <iostream>
#include <mingw.mutex.h>
#include <mingw.thread.h>
#include <mingw.condition_variable.h>
#include <vector>
using namespace std;

const int READ = 0;
const int WRITE = 1;

struct LockNode
{
    int lockType;            // 锁类型，1是写锁，0是读锁
    int semaphore;           // 信号量
    thread::id threadID;     // 所属线程
    string filename;         // 文件名
    condition_variable cond; // 条件变量
    LockNode *next;
    LockNode(string filename, int lockType)
    {
        this->next = NULL;
        this->filename = filename;
        this->lockType = lockType;
        this->threadID = this_thread::get_id();
        this->semaphore = 1;
    }
};

class LockList
{
private:
    LockNode *head;
    mutex mx;
    LockNode *find(LockNode *start, string filename, int lockType);
    LockNode *findLast(LockNode *start, string filename, int lockType);
    LockNode *getTail();
    void remove(LockNode *remove);

public:
    LockList();
    void Lock(string filename, int lockType);
    void UnLock(string filename, int lockType);
};

LockList::LockList()
{
    head = new LockNode("head", -1);
}

// 找到最近一个锁，也就是正在活跃的锁
LockNode *LockList::find(LockNode *start, string filename, int lockType)
{
    LockNode *curr = start;
    while (curr != NULL)
    {
        if (curr->filename == filename && curr->lockType == lockType)
        {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

// 找到最后一个锁，也就是最新加上的锁
LockNode *LockList::findLast(LockNode *start, string filename, int lockType)
{
    LockNode *curr = start;
    LockNode *result = NULL;
    while (curr != NULL)
    {
        if (curr->filename == filename && curr->lockType == lockType)
        {
            result = curr;
        }
        curr = curr->next;
    }
    return result;
}

// 找到尾节点，从尾节点插入锁
LockNode *LockList::getTail()
{
    LockNode *curr = head;
    while (curr->next != NULL)
    {
        curr = curr->next;
    }
    return curr;
}

// 删除节点
void LockList::remove(LockNode *remove)
{
    LockNode *curr = head;
    while (curr->next != NULL)
    {
        if (curr->next == remove)
        {
            curr->next = curr->next->next;
            return;
        }
        curr = curr->next;
    }
    return;
}

// 加锁，lockType=1为加写锁，lockType=0为加读锁
void LockList::Lock(string filename, int lockType)
{
    unique_lock<mutex> lk(mx);
    LockNode *lock = new LockNode(filename, lockType);
    LockNode *tail = this->getTail();
    LockNode *wlock, *rlock, *lastwlock, *lastrlock;

    switch (lockType)
    {
    // 加写锁
    case WRITE:
        wlock = this->find(this->head, filename, WRITE);
        rlock = this->find(this->head, filename, READ);
        if (wlock != NULL || rlock != NULL)
        {
            tail->next = lock;
            cout << "threadid: " << this_thread::get_id() << " 阻塞等待其他线程释放" << filename << "锁" << endl;
            lock->cond.wait(lk);
            return;
        }
        break;
    // 加读锁
    case READ:
        lastwlock = this->findLast(this->head, filename, WRITE);
        lastrlock = this->findLast(this->head, filename, READ);
        if (lastwlock != NULL)
        {
            // 判断写锁后是否有阻塞的读
            LockNode *raw = this->find(wlock, filename, READ);
            if (raw != NULL)
            {
                raw->semaphore++;
                cout << "threadid: " << this_thread::get_id() << " 阻塞等待其他线程释放" << filename << "锁" << endl;
                raw->cond.wait(lk);
                return;
            }
            tail->next = lock;
            cout << "threadid: " << this_thread::get_id() << " 阻塞等待其他线程释放" << filename << "锁" << endl;
            lock->cond.wait(lk);
            return;
        }
        if (lastrlock != NULL)
        {
            lastrlock++;
            return;
        }
        tail->next = lock;
        return;
        break;
    default:
        break;
    }
}

// 解锁，lockType=1为解写锁，lockType=0为解读锁
void LockList::UnLock(string filename, int lockType)
{
    unique_lock<mutex> lk(mx);
    LockNode *wlock, *rlock;

    switch (lockType)
    {
    // 解写锁
    case WRITE:
        wlock = this->find(this->head, filename, WRITE);
        this->remove(wlock);
        if (wlock->next != NULL)
        {
            wlock->next->cond.notify_all();
        }
        delete wlock;
        break;
    // 解读锁
    case READ:
        rlock = this->find(this->head, filename, READ);
        if (rlock == NULL)
            return;
        if (--rlock->semaphore == 0)
        {
            this->remove(rlock);
            if (rlock->next != NULL)
            {
                rlock->next->cond.notify_all();
            }
            delete rlock;
        }
        break;
    default:
        break;
    }
}

void reader(void *arg)
{
    LockList *lockList = static_cast<LockList *>(arg);
    lockList->Lock("a.txt", 0);
    cout << "reader" << endl;
    this_thread::sleep_for(std::chrono::seconds(3));
    lockList->UnLock("a.txt", 0);
}

void writer(void *arg)
{
    LockList *lockList = static_cast<LockList *>(arg);
    lockList->Lock("a.txt", 1);
    cout << "writer" << endl;
    this_thread::sleep_for(std::chrono::seconds(3));
    lockList->UnLock("a.txt", 1);
}

int main()
{
    LockList lockList;
    vector<thread> vec;
    for (int i = 0; i < 5; ++i)
    {
        vec.push_back(thread(reader, (void *)&lockList));
        vec.push_back(thread(writer, (void *)&lockList));
    }
    for (int i = 0; i < vec.size(); ++i)
    {
        if (vec[i].joinable())
            vec[i].join();
    }
}