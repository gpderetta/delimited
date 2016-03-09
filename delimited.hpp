#ifndef GPD_DELIMITED_HPP
#define GPD_DELIMITED_HPP
#include <exception>
#include "test_utils.hpp"
namespace gpd
{


struct cont_impl {  void **ip, **sp; };

/**
   An implementation of delimited continuations in C++.

   This implementation heavily relies on GCC extensions plus a few
   evil hacks and should not be considered production ready as it
   *will* break even with minor compiler changes, options or even
   surrounding code.

   It should only be considered a demonstration of how compiler help
   can get a library based coro implementation close as possible to
   ideal.
**/

template<class R = void, class T = void>
struct continuation {
    continuation(const continuation& rhs) = delete;
    continuation(cont_impl data = {}) : data(data) {}
    continuation(continuation&& rhs) : data(rhs.pilfer()) { }
    continuation&operator=(continuation rhs) 
    { assert(data.ip == 0); data = rhs.data; rhs.data.ip = 0; return *this; }
    
    ~continuation()  {  assert(data.ip == 0);  }
    explicit operator bool() const { return data.ip; }

    R * operator()(T* param = nullptr);

    void exit();
    cont_impl pilfer() { return std::exchange(data, cont_impl{0, 0});  }

    cont_impl data;
};

/// Create a continuation object. This is similar to the shift operator
template<class R, class T, class F>
continuation<R, T> make(F&& f);

template<class R2, class T2, class R, class T, class F>
continuation<R2, T2> splice_ex(continuation<R, T> cont, F&& f);

template<class R, class T, class F>
continuation<R, T> splice(continuation<R, T> cont, F&& f) {
    return splice_ex<R, T>(std::move(cont), std::forward<F>(f));
}

template<class R, class T, class F>
continuation<R, T> yield_with(continuation<R, T> cont, F&& f, T* param = nullptr);

template<class R2, class T2, class R, class T, class F>
continuation<R2, T2> yield_with_ex(continuation<R, T> c1,
                                   F&& f, T2* param = nullptr);
struct exit_exception : std::exception {
    exit_exception(cont_impl x) : cont(x) {}
    cont_impl cont;
    std::exception_ptr except;
};

template<class R, class T>
void exit_to(continuation<R, T> cont)
{
    throw exit_exception(cont.pilfer());
}

//// Implementation

namespace details {
template<class T>
void destruct(T*x) { x->~T(); }
template<class F> using trampoline_t = cont_impl (F&, cont_impl);

/// Trampoline function that calls an actual spliced F. It performs
/// correct typecasting of parameters, builds the continuation form a
/// cont_impl and destroys F during normal return..
template<class F, class R, class T>
cont_impl __attribute__((flatten)) trampoline(F& f, cont_impl p) {
    auto result = f(continuation<R, T>{p});
    f.~F();
    return result.pilfer();
}

/// Trampoline_thunk calls the trampoline. It performs calling
/// convention adjustments and pops the storage for F from the stack
/// frame. Finally it is responsible of destroying F on exceptional
/// unwind (even if the trampoline was not called at all).
///
/// This function is a giant hack: we need to control the body of the
/// function completely, including defining .cfi unwind info and the
/// exception table, but we still need to wrap it inside a C++
/// function as we need the template machinery to compute the size of
/// F and the actuall address of the trampoline and destruct functions.
///
/// On entry the stack contains the orig_ip address and the function
/// object state.  If the function object has size N, esp is its
/// address, and N(esp) is the address of the return slot.
///
/// On entry {rax, rdx} is the source continuation, rbx is the
/// parameter.  The trampolined functor is called with rdi containing
/// 'this', rax is moved to rsi and rdx is passed through. The
/// functor will preserve rbx.
///
/// On functor return, {rax, rdx} will contain the (potentially
/// modified) return continuation; rbx is preserved. 
template<class F, trampoline_t<F> * trampoline>
__attribute__((optimize("-fomit-frame-pointer"))) // we really want 'naked' here
void trampoline_thunk() {
    asm volatile (
        ".start%=:                                     \n\t"
        "    .cfi_personality 0x3,__gxx_personality_v0 \n\t"
        "    .cfi_lsda 0x3, .LSDA%=                    \n\t"
        "    .cfi_def_cfa_offset 8+%a0                 \n\t"
        "    nop                                       \n\t" // one byte nop
        ".real_start%=:                                \n\t"
        "    movq  %%rsp,  %%rdi                       \n\t"
        "    movq  %%rax,  %%rsi                       \n\t"
        ".call%=:                                      \n\t"
        "    call %c1                                  \n\t"
        ".call_end%=:                                  \n\t"
        "    movq  %c0(%%rsp), %%rcx                   \n\t"
        "    addq $(8+%c0), %%rsp                      \n\t"
        "    .cfi_def_cfa_offset 0                     \n\t"
        "    jmp *%%rcx                                \n\t"
        ".cleanup%=:                                   \n\t"
        "    mov %%rsp,  %%rdi                         \n\t"
        "    movq	%%rax, %%rbx                       \n\t"
        "    call %c2                                  \n\t"
        "    addq $%c0, %%rsp                          \n\t"
        "    .cfi_def_cfa_offset 8                     \n\t"
        "    movq	%%rbx, %%rdi                       \n\t"
        ".unwind_resume%=:                             \n\t"
        "	 call	_Unwind_Resume                     \n\t"
        ".unwind_resume_end%=:                         \n\t"
        ".end%=:                                       \n\t"
        "    .cfi_endproc                              \n\t"
        "    .globl	__gxx_personality_v0               \n\t"
        "    .pushsection	.gcc_except_table,\"a\",@progbits\n\t"
        ".LSDA%=:                                      \n\t"
        "    .byte	0xff                               \n\t"
        "    .byte	0xff                               \n\t"
        "    .byte	0x1                                \n\t"
        "    .uleb128 .table_end%= - .table_begin%=    \n\t"
        ".table_begin%=:                               \n\t"
        "    .uleb128 .start%= - .start%=              \n\t"
        "    .uleb128 .real_start%= - .start%=         \n\t"
        "    .uleb128 .cleanup%= - .start%=            \n\t"
        "    .uleb128 0                                \n\t"
        "    .uleb128 .call%= - .start%=               \n\t"
        "    .uleb128 .call_end%= - .call%=            \n\t"
        "    .uleb128 .cleanup%= - .start%=            \n\t"
        "    .uleb128 0                                \n\t"
        "    .uleb128 .unwind_resume%= - .start%=      \n\t"
        "    .uleb128 .unwind_resume_end%= - .unwind_resume%= \n\t"
        "    .uleb128 0                                \n\t"
        "    .uleb128 0                                \n\t"
        ".table_end%=:                                 \n\t"
        ".popsection                                   \n\t"
        ".cfi_startproc                                \n\t"
        : : "i"((sizeof(F) + sizeof(void*) -1) & -sizeof(void*) )
          , "i"(trampoline)
          , "i"(&destruct<F>)
        );
    __builtin_unreachable();
}
}

template<class R2, class T2, class R, class T, class F>
continuation<R2, T2> splice_ex(continuation<R, T> cont, F&& f) {
    using F2 = std::decay_t<F>;

    // Trampoline thunk contains a one byte nop which is required to
    // correctly match the .cfi pseudo instructions with the code. We
    // must skip over it.
    auto nop_size = 1; 
    auto target = (char*)&details::trampoline_thunk
        <F2, &details::trampoline<F2, T2, R2> > + nop_size;
    void ** orig_ip = std::exchange(cont.data.ip, (void**)target);
    *(--cont.data.sp) = orig_ip; // return address slot
    cont.data.sp -= (sizeof(F2) + sizeof(void*) -1) / sizeof(void*);
    new (cont.data.sp) F2(std::forward<F>(f));
    return continuation<R2, T2>{cont.pilfer()};
}

template<class R, class T, class F>
continuation<R, T> make(F&& f) {
    const std::size_t len = 1024*10;
    char* stack = new char[len];

    using result_t = continuation<R, T>;
    using rresult_t = continuation<T, R>;
    result_t result { { 0, (void**)(stack + len) } };

    auto f_except = [f = std::forward<F>(f)] (auto cont){
        cont_impl ret;
        try {
            ret = f(std::move(cont)).pilfer();
        } catch(exit_exception& e)
        {
            if (e.except)
                ret = splice(continuation<>(e.cont),
                             [e = e.except] (auto c){
                                 std::rethrow_exception(e);
                                 return c;
                             }).pilfer();
            else
                ret = e.cont;
        }
        return continuation<>(ret);
    };

    
    auto cleanup = [stack] (rresult_t c) {
        (void)c.pilfer();
        delete [] stack; return rresult_t{};
    };

    auto exit_to_cleanup = [cleanup] (rresult_t c) -> rresult_t {
        if (!c) std::abort();
        splice(std::move(c), cleanup)(0);
        abort();
    };
    
    result = splice(std::move(result), exit_to_cleanup);
    result = splice(std::move(result), std::move(f_except));
    return result;
}


#ifndef __OPTIMIZE__
#define GPD_CALL_ATTRIBUTES \
    __attribute__((optimize("-O3,-fomit-frame-pointer,-fnon-call-exceptions"), noinline, flatten))
#else
#define GPD_CALL_ATTRIBUTES \
    __attribute__((always_inline, flatten))
#endif

#define GPD_CLOBBERS                                                    \
    "memory", "rbp",                                              \
        "r8", "r9", "r10", "r11", "r13", "r14", "r15",                  \
        "xmm0", "xmm1", "xmm2" , "xmm3", "xmm4" , "xmm5" , "xmm6" , "xmm7", \
        "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15", \
        "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)" \
        /**/
    
template<class R, class T>
R * GPD_CALL_ATTRIBUTES continuation<R, T>::operator()(T* param) {
    void * p =  param;
    auto ip = data.ip;
    auto sp = data.sp;
    try {
        /// HACK HACK
        /// GCC assumes that asm statements do not throw. So we
        /// use a dummy store plus async-unwind-tables to force
        /// gcc to generate unwind info. We jump over the store
        /// within the asm, which is explicitly prohibited by GCC,
        /// but it seems to work good enough for us.
        asm volatile (
            "    xchg  %%rsp, %%rdx      \n\t"
            "    leaq  1f(%%rip), %%rax  \n\t"
            "    jmp   *%%rsi            \n\t"
            : "=a"(ip), "+d"(sp), "+b"(p)
            : "S"(ip) 
            : "rdi", "r12", "rcx", GPD_CLOBBERS);
        *(volatile int*)0x0 = 0; // force unwind table
        asm volatile (".p2align 4\n\t1:": "+a"(ip), "+d"(sp), "+b"(p)
                      :: "rsi", "rdi", "r12", "rcx", GPD_CLOBBERS);
    
        data.ip = ip;
        data.sp = sp;
        return (R*)p;
    } catch(...) { data.sp = 0; throw; }

}

template<class R, class T, class F>
continuation<R, T> yield_with(continuation<R, T> cont, F&& f, T* param) {
    cont = yield_with_ex<R, T>(std::move(cont), std::forward<F>(f), param);
    return cont;
}

template<class F, class R, class T>
cont_impl yield_trampoline(F& f, cont_impl c) {
    return f(continuation<R, T>{c}).pilfer();
}

template<class R2, class T2, class R, class T, class F>
continuation<R2, T2> yield_with_ex(continuation<R, T> c1,
                                   F&& f, T2* param){
    auto data = c1.pilfer();
    register auto ip asm("%r12")  = data.ip ;
    void ** ipout;
    auto sp = data.sp;
    void * p = param;
    try {
        asm volatile (
            "    xchg  %%rsp, %%rdx      \n\t"
            "    leaq  1f(%%rip), %%rsi   \n\t"
            "2:  call  %c3               \n\t"
            "    jmp   *%%r12            \n\t"
            : "=&a"(ipout), "+d"(sp), "+b"(p)
            : "i"(&yield_trampoline<F, T2, R2>), "r"(ip), "D"(&f)
            : "rcx", "rsi", GPD_CLOBBERS);
        *(volatile int*)0x0 = 0; // force unwind table
        asm volatile (".p2align 4\n\t1:": "+a"(ipout), "+d"(sp), "+b"(p)
                      :: "rsi", "rdi", "r12", "rcx", GPD_CLOBBERS);
    
        data.ip = ipout;
        data.sp = sp;
        (void)p;
        return { data };
    } catch(...) { data.sp = 0; throw; }
}


/// x86_64 SysV calling conventions:
// callee-save: RBP, RBX, R12, R13, R14, R15
// param:       RDI, RSI, RDX, RCX, R8,  R9,
// float param: XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7
// return:      RAX, RDX
// float return: XMM0
// caller save: R10, R11

template<class R, class T>
void  continuation<R, T>::exit() {
    if (!!this)
    {
        yield_with(std::move(*this), [](auto c) { exit_to(std::move(c)); return c; });
    }
}

}
#endif
