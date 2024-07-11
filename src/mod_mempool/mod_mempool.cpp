#include <mod_mempool/mod_mempool.hpp>

#include <stdlib.h>
#include <stdint.h>

#include <mod_common/log.hpp>


int mempool::init()
{
    m_buf = malloc(m_ele_num * m_ele_size);
    if (!m_buf) {
        return -1;
    }
    size_t i;
    for (i = 0; i < m_ele_num; i++) {
        m_q.push((uint8_t*)m_buf + (i * m_ele_size));
    }
    return 0;
}

void mempool::deinit()
{
    void* buf;
    // clear m_q
    while (m_q.try_pop(buf)) {}
    free(m_buf);
    m_buf = nullptr;
}

void* mempool::alloc()
{
    if (m_q.was_empty()) {
        log_error("can't alloc! %p", this);
        print_me();
        return nullptr;
    }
    return m_q.pop();
}

void mempool::free(void* buf)
{
    m_q.push(buf);
}
