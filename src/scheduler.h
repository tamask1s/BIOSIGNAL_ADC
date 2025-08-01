#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

class i_scheduler
{
public:
    static int const INFINIT = -1;
    using Task = std::function<void()>;
    virtual uint32_t schedule(Task const& task, int interval, int runCount = INFINIT) = 0;
    virtual void cancel(uint32_t task_id) = 0;
    virtual void run() = 0;
    virtual void poll() = 0;
    virtual void stop() = 0;
    virtual ~i_scheduler() {}
};

struct scheduler_factory
{
    enum e_type
    {
        clock = 0,
        clock_gettime,
        gettimeofday,
        chrono,
        epoll,
        timerfd,
        asio
    };
    static i_scheduler* new_instance(e_type type);
    static void delete_instance(i_scheduler* instance);
};

struct clock_scheduler_factory
{
    static i_scheduler* new_instance();
    static void delete_instance(i_scheduler* instance);
};

struct clock_gettime_scheduler_factory
{
    static i_scheduler* new_instance();
    static void delete_instance(i_scheduler* instance);
};

struct gettimeofday_scheduler_factory
{
    static i_scheduler* new_instance();
    static void delete_instance(i_scheduler* instance);
};

struct chrono_scheduler_factory
{
    static i_scheduler* new_instance();
    static void delete_instance(i_scheduler* instance);
};

struct epoll_scheduler_factory
{
    static i_scheduler* new_instance();
    static void delete_instance(i_scheduler* instance);
};

struct timerfd_scheduler_factory
{
    static i_scheduler* new_instance();
    static void delete_instance(i_scheduler* instance);
};

struct asio_scheduler_factory
{
    static i_scheduler* new_instance();
    static void delete_instance(i_scheduler* instance);
};

#endif ///_SCHEDULER_H_
