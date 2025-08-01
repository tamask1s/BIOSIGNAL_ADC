#include <chrono>
#include <functional>
#include <thread>
#include <unordered_map>
#include <memory>
#include <cstdint>
using namespace std;

#include "scheduler.h"
#include "likely_unlikely.h"

class chrono_scheduler: public i_scheduler
{
    struct clock_task
    {
        uint32_t task_id_;
        int remaining_runs_;
        i_scheduler::Task task_;
        chrono::milliseconds interval_;
        chrono::steady_clock::time_point run_time_point_;

        clock_task(uint32_t task_id, chrono::milliseconds interval, chrono::steady_clock::time_point tp, int remaining_runs, i_scheduler::Task task)
            :task_id_(task_id),
             remaining_runs_(remaining_runs),
             task_(std::move(task)),
             interval_(std::move(interval)),
             run_time_point_(std::move(tp))
        {}
    };

    unordered_map<uint32_t, shared_ptr<clock_task>> tasks_;
    unordered_map<uint32_t, shared_ptr<clock_task>>::iterator it_;
    uint32_t last_task_id_ = 0;
    bool task_was_deleted_ = 0;
    bool stop_mark_ = false;

public:
    virtual void run()
    {
        while (true)
        {
            for (it_ = tasks_.begin(); it_ != tasks_.end();)
            {
                auto& task = *it_->second;
                task_was_deleted_ = false;

                if (task.run_time_point_ <= chrono::steady_clock::now())
                {
                    uint32_t task_remaining_runs_ = --task.remaining_runs_;
                    uint32_t current_task_id_ = task.task_id_;
                    task.run_time_point_ += task.interval_;
                    task.task_();
                    if (unlikely(task_remaining_runs_ == 0))
                        cancel(current_task_id_);
                }
                if (likely(task_was_deleted_ == false))
                    ++it_;
            }
            if (unlikely(stop_mark_))
                return;
            this_thread::sleep_for(std::chrono::microseconds(1000));
        }
    }

    void poll()
    {
        for (it_ = tasks_.begin(); it_ != tasks_.end();)
        {
            auto& task = *it_->second;
            task_was_deleted_ = false;

            if (task.run_time_point_ <= chrono::steady_clock::now())
            {
                uint32_t task_remaining_runs_ = --task.remaining_runs_;
                uint32_t current_task_id_ = task.task_id_;
                task.run_time_point_ += task.interval_;
                task.task_();
                if (unlikely(task_remaining_runs_ == 0))
                    cancel(current_task_id_);
            }
            if (likely(task_was_deleted_ == false))
                ++it_;
        }
        if (unlikely(stop_mark_))
            return;
    }

    virtual void stop()
    {
        stop_mark_ = true;
    }

    virtual uint32_t schedule(Task const& task, int interval, int remaining_runs = -1)
    {
        uint32_t task_id = ++last_task_id_;
        tasks_[task_id] = make_shared<clock_task>(task_id, chrono::milliseconds(interval), chrono::steady_clock::now() + chrono::milliseconds(interval), remaining_runs, task);
        return task_id;
    }

    virtual void cancel(uint32_t task_id)
    {
        auto task = tasks_.find(task_id);
        if (task != tasks_.end())
        {
            it_ = tasks_.erase(task);
            task_was_deleted_ = true;
        }
    }

    virtual ~chrono_scheduler()
    {}
};

i_scheduler* chrono_scheduler_factory::new_instance()
{
    return new chrono_scheduler();
}

void chrono_scheduler_factory::delete_instance(i_scheduler* instance)
{
    delete((chrono_scheduler*)instance);
}
