#ifndef GPD_TEST_UTILS_HPP
#define GPD_TEST_UTILS_HPP
#include <chrono>
#include <iostream>
namespace gpd {
namespace cr = std::chrono;
#define LOG(...)                                                        \
    do {                                                                \
        std::cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ \
                  << ": "                                                \
                  << __VA_ARGS__                                        \
                  << "\n";                                              \
    } while (false)                                                     \
/**/


/// Tests start here

static inline unsigned long long rdtsc() {
    unsigned hi, lo;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

template<class F>
void __attribute__((noinline)) do_measure(F f)
{
    f();
}
template<class F>
void measure(F&& f, const char *msg, std::size_t len) {
    //LOG("test begin: " << msg);
    using ns = cr::duration<double, std::nano> ;
    auto tbegin = std::chrono::high_resolution_clock::now();
    auto begin = rdtsc();
    do_measure(std::forward<F>(f));
    auto end = rdtsc();
    auto tend = cr::high_resolution_clock::now();
    
    double delta = end-begin;
    auto   tdelta = cr::duration_cast<ns>(tend-tbegin).count();
    LOG ("test end: " << msg 
                      << "\t clock/iter: " << (delta/len)
                      << "\t ns/iter: " << tdelta/len
        //             << "\t iterations: "<< len
        //<< "\n\t clocks total: " << delta
        //            << "\n\t ns total: " << tdelta
        );
}

struct unwind_guard
{
    std::string name;
    bool destroyed = false;
    void destroy() { destroyed = true; }
    ~unwind_guard() {
        if (!destroyed)
        {
            LOG("object not destroyed: " << name);
        }
        assert(destroyed);
    }
};

struct unwind_test {
    unwind_test(unwind_guard& guard) : guard(&guard) {}
    unwind_guard * guard;
    unwind_test(unwind_test&& rhs)
        : guard(std::exchange(rhs.guard, nullptr))
    {
    }
    
    ~unwind_test() {
        if (guard) {
            LOG ("live unwind test '" << guard->name << "' destroyed" );
            guard->destroy();
        }
    }
};

}
#endif
