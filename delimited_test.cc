#include <utility>
#include <cstdlib>
#include <memory>
#include <cassert>
#include <iostream>
#include "delimited.hpp"
#include "test_utils.hpp"
using namespace gpd;

void test()
{
    {
        LOG("Basic call test");
        unwind_guard guard;
        continuation<void, void> cont = make<void, void>(
            [canary = unwind_test(guard)](auto c)
            {
                LOG("in continuation");
                return c;
            });
        cont();
        LOG("Test end");
    }
    {
        LOG("Basic yield test");
        unwind_guard guard;
        continuation<void, void> cont = make<void, void>(
            [canary = unwind_test(guard)](auto c)
            {
                LOG("in continuation I");
                c();
                LOG("in continuation II");
                return c;
            });
        cont();
        LOG("in main");
        cont();
        LOG("Test end");
    }
    {
        LOG("Basic throw test");
        unwind_guard guard;
        continuation<void, void> cont = make<void, void>(
            [canary = unwind_test(guard)](auto c)
            {
                LOG("in continuation");
                exit_to(std::move(c));
                return c;
            });
        cont();
         
        LOG("Test end");
    }
    {
        LOG("Basic splice test");
        unwind_guard guard {"main"};
        continuation<void, void> cont = make<void, void>(
            [canary = unwind_test(guard)](auto c)
            {
                unwind_guard guard {"cont"};
                LOG("in continuation I");
                c = splice(std::move(c), [canary = unwind_test(guard)]
                       (auto c) {
                           LOG("cont->main");
                           return c;
                       });
                c();
                LOG("in continuation II");
                return c;
            });
        LOG("calling : " << &cont);
        cont();
        LOG("in main");
        cont();
        LOG("Test end");
    }


    #if 0
    {
        LOG("Yield-predict performance test");
        continuation<void, void> x = make<void, void>(
            [](auto c)
            {
                void * x = 0;
                while(true) {
                    x = c(x); x = c(x); x = c(x); x = c(x); x = c(x);
                    x = c(x); x = c(x); x = c(x); x = c(x); x = c(x);

                }
                return c;
            });

        measure([&] { x(0); }, "yield-predict");
        x.exit();
    }
    
    {
       
        LOG("Yield-mispredict performance test");
        continuation<void, void> x = make<void, void>(
            [](auto c)
            {

                while(true) {
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);

                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);

                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);

                }
                return c;
            });

        measure([&] { x(0); }, "yield-mispredict");
        x.exit();
    }
    #endif
}

int main()
{
    test();
    #if 0
    volatile int i = 666;
    volatile int j = 42;
    continuation<void, void> y = make<void, void>(
        [](auto c)
        {
            unwind_test _ {"y: stack"};
            LOG ("y I");
            return std::move(c);
        });
    LOG ("m I");
    y(0);
    LOG ("m II");
    
    continuation<void, void> x = make<void, void>(
        [i, &j](auto c)
        {
            LOG ("cont I");
            c(0);
            LOG ("cont II");
            c(0);
            LOG ("cont III");
            c = splice(std::move(c),
                       [](auto c) {
                           LOG ("cont III -> main IV: throw ");
                           throw std::move(c);
                           return c;
                       });
            c(0);
            LOG ("cont ");

            c = splice(std::move(c),
                       [canary = unwind_test{"cont IV: closure"}]
                       (auto c) {
                           unwind_test _ {"cont IV: stack"};
                           LOG ("cont IV -> main V: return ");
                           return c;
                       });
            c(0);
            LOG ("cont V: pass thru + exceptions ");
            c = splice(std::move(c),
                       [canary = unwind_test{"cont V: closure except"}]
                       (auto c) {
                           unwind_test _ {"cont V: except"};
                           LOG ("cont V -> cont VI: yield");
                           c(0);
                           assert(false && "cont VI -> main VI: unreachable");
                           return c;
                       });
            c(0);
            LOG ("cont VI");
            c = splice(std::move(c),
                       [](auto c) {
                           LOG ("cont VI -> main VI: throw");
                           throw std::move(c);
                           return c;
                       });
            c(0);
            LOG ("cont VII");
            c = splice(std::move(c),
                       [canary = unwind_test{"cont VII: closure ret I"}]
                       (auto c) {
                           LOG("cont VII -> main IIX: I");
                           return c;
                       });
            c = splice(std::move(c),
                       [canary = unwind_test{"cont VII: closure ret II"}]
                       (auto c) {
                           LOG("cont VII -> main IIX: II");
                           return c;
                       });
            c(0);

            LOG ("cont IIX");
            c = splice(std::move(c),
                       [canary = unwind_test{"cont IIX: closure except I"}]
                       (auto c) {
                           assert(false && "cont IIX -> main IX: unreachable");
                           return c;
                       });
            c = splice(std::move(c),
                       [canary = unwind_test{"cont IIX: closure except II"}]
                       (auto c) {
                           LOG ("cont IIX -> main IX: throw");
                           throw std::move(c);
                           return c;
                       });
            c(0);
            LOG ("cont IX");
            return c;
        });

    LOG ("main I");
    x(0);
    LOG ("main II");
    x(0);
    LOG ("main III");
    try {
        x(0);
        assert(false && "main III: unreachable");
    } catch(continuation<void, void>& c) {
        LOG ("main IV: exception caught!");
        x = std::move(c);
        x(0);
    }
    try {
        LOG ("main V: exception caught!");
        x(0);
    } catch(continuation<void, void>& c)  {
        LOG ("main VI: exception caught!");
        x = std::move(c);
    }
    LOG ("main VII");
    x(0);
    try {
        LOG ("main IIX");
        x(0);
    } catch(continuation<void, void>& c) {
        LOG ("main IX");
        x = std::move(c);
    }
    x(0);
    LOG ("main X");
    {
        measure([&] { asm volatile ("":::"memory"); }, "null");
    }
    {
        continuation<void, void> x = make<void, void>(
            [](auto c)
            {
                void * x = 0;
                while(true) {
                    x = c(x);
                }
                return c;
            });

        measure([&] { x(0); }, "yield");
    }
    {
        continuation<void, void> x = make<void, void>(
            [](auto c)
            {
                void * x = 0;
                while(true) {
                    x = c(x); x = c(x); x = c(x); x = c(x); x = c(x);
                    x = c(x); x = c(x); x = c(x); x = c(x); x = c(x);

                }
                return c;
            });

        measure([&] { x(0); }, "yield-predict");
    }
    {
        continuation<void, void> x = make<void, void>(
            [](auto c)
            {

                while(true) {
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);

                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);

                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);
                    c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0); c(0);

                }
                return c;
            });

        measure([&] { x(0); }, "yield-mispredict");
    }
    #endif
}

