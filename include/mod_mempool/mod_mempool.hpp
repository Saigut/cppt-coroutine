#ifndef CPP_TOOLKIT_MOD_MEMPOOL_HPP
#define CPP_TOOLKIT_MOD_MEMPOOL_HPP

#include <stdio.h>
#include <stddef.h>
#include <queue>
#include <mod_atomic_queue/atomic_queue.hpp>



class mempool {
public:
    mempool() = default;
    explicit mempool(size_t ele_num, size_t ele_size)
    : m_ele_num(ele_num), m_ele_size(ele_size) {}
    virtual int init();
    virtual void deinit();
    virtual void* alloc();
    virtual void free(void* buf);
    virtual void print_me() {
        printf("mempool: %p\n", this);
    }
private:
    size_t m_ele_num = 0;
    size_t m_ele_size = 0;
    void* m_buf = nullptr;
    using mempool_aq = atomic_queue::AtomicQueue2<void*, 65535>;
    mempool_aq m_q;
};

#endif //CPP_TOOLKIT_MOD_MEMPOOL_HPP
