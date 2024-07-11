#include <mod_time_wheel/mod_time_wheel.hpp>


time_wheel_task_t::time_wheel_task_t(size_t timeout_interval, timer_callback_t callback)
        : m_timeout_interval(timeout_interval), m_callback(std::move(callback)) {}

time_wheel_task_t::time_wheel_task_t(size_t timeout_interval)
        : m_timeout_interval(timeout_interval) {}

time_wheel_task_t::time_wheel_task_t(size_t timeout_interval, size_t cycles, timer_callback_t callback)
        : m_timeout_interval(timeout_interval), m_remaining_cycles(cycles), m_callback(std::move(callback)) {}

void time_wheel_task_t::set_cb(time_wheel_task_t::timer_callback_t callback)
{
    m_callback = std::move(callback);
}

void time_wheel_task_t::tick() {
    if (!is_canceled) {
        m_callback();
    }
//    if (m_remaining_cycles == 0) {
//        m_callback();
//    } else {
//        m_remaining_cycles--;
//    }
}

size_t time_wheel_task_t::get_timeout_interval() const {
    return m_timeout_interval;
}

void time_wheel_task_t::cancel()
{
    is_canceled = true;
}

time_wheel_t::time_wheel_t(size_t slots) : m_slots(slots), m_current_slot(0), m_tasks(slots) {}

void time_wheel_t::add_tw_task(time_wheel_task_t* timer) {
    auto timeout_interval = timer->get_timeout_interval();
    {
        std::lock_guard lock(m_mutex_tasks);
        size_t slot = (m_current_slot + timeout_interval) % m_slots;
        auto& rst = m_tasks[slot].emplace_back();
        rst.task = timer;
    }
}

void time_wheel_t::add_tw_task(time_wheel_task_t* timer, std::function<void()> release_cb)
{
    auto timeout_interval = timer->get_timeout_interval();
    {
        std::lock_guard lock(m_mutex_tasks);
        size_t slot = (m_current_slot + timeout_interval) % m_slots;
        auto& rst = m_tasks[slot].emplace_back();
        rst.task = timer;
        rst.release_cb = std::move(release_cb);
    }
}

void time_wheel_t::tick() {
    std::vector<time_wheel_info_t> task_infos;
    {
        std::lock_guard lock(m_mutex_tasks);
        task_infos = std::move(m_tasks[m_current_slot]);
        m_current_slot = (m_current_slot + 1) % m_slots;
    }
    for (auto& task_info : task_infos) {
        task_info.task->tick();
        if (task_info.release_cb) {
            task_info.release_cb();
        }
    }
}
