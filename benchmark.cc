#include <utility>
#include <cstdlib>
#include <memory>
#include <cassert>
#include <iostream>
#include "delimited.hpp"
#include "test_utils.hpp"
using namespace gpd;

int main()
{
        constexpr std::size_t len = 1000000;
    {
        measure([&] {
                for (int i = 0; i < (int)len; ++i)
                    asm volatile ("":::"memory");
            }, "null", len);
    }

    {
        LOG("Yield performance test");
        continuation<void, void> x = make<void, void>(
            [](auto c)
            {
                while(true) {
                    c();
                }
                return c;
            });


        measure([x = std::move(x)] () mutable
                {
                    auto c = std::move(x);
                    for (int i = 0; i < (int)len; ++i)
                        c();
                }, "yield", len);
    }
    {
        LOG("Yield With performance test");
        continuation<void, void> x = make<void, void>(
            [](auto c)
            {
                //LOG("here");
                while(true) {
                    //LOG("here");
                    asm("#hello\n\t" :::);
                    c = yield_with(std::move(c), [](auto c){
                            asm("#world\n\t" :::);
                            return c;
                            //asm("#world\n\t" :::);
                        });
                }
                return c;
            });


        measure([x = std::move(x)] () mutable
                {
                    //LOG("here");
                    auto c = std::move(x);
                    for (int i = 0; i < (int)len; ++i)
                    {
                        asm("#hi there3\n\t" :::);
                        c();
                    }
                }, "yield_with", len);
    }
}
    
