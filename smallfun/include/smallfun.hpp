#ifndef SMALLFUNCTION_SMALLFUNCTION_HPP
#define SMALLFUNCTION_SMALLFUNCTION_HPP

#include <type_traits>
#include <utility>
#include <cstddef>

namespace smallfun {


template<typename ReturnType, typename... Args>
struct SFConcept {
    virtual ReturnType operator()(Args...)const = 0;
    virtual ReturnType operator()(Args...)      = 0;
    virtual void copy(void*)                    = 0;
    virtual void move(void*)                    = 0;

    virtual ~SFConcept() {};
};

template<typename F, typename ReturnType, typename... Args>
struct SFModel final : SFConcept<ReturnType, Args...> {
    F f;

    SFModel(F &f)
        : f(f)
    {}

    SFModel(F &&f)
        : f(std::move(f))
    {}

    void copy(void* memory)                   override final {
        new (memory) SFModel(f);
    }
    void move(void* memory)                   override final {
        new (memory) SFModel(std::move(f));
    }
    ReturnType operator()(Args... args) const override final {
        return f(args...);
    }
    ReturnType operator()(Args... args)       override final {
        return f(args...);
    }

};

/*
template<typename ReturnType, typename... Args>
struct SFModelPlain final : SFConcept<ReturnType, Args...> {
  using F = ReturnType(*const)(Args...);
  F f;

  SFModelPlain(F f)
    : f(f)
  {}

  void copy(void* memory)             override final {
    // *reinterpret_cast<decltype(&f)*>(memory) = &f; // callable with (*reinterpret_cast<decltype(&f)*>(memory))(args...)
    new (memory) SFModelPlain(f);
  }
  void move(void* memory)             override final {
    new (memory) SFModelPlain(std::move(f));
  }

  ReturnType operator()(Args...args)const override final {
    return f(args...);
  }

  ReturnType operator()(Args...args)      override final {
    return f(args...);
  }

};
*/



template<typename Signature, std::size_t Size=60, std::size_t Align = 0>
struct SmallFun;

template<typename ReturnType, typename... Args, std::size_t Size, std::size_t Align>
class SmallFun<ReturnType(Args...), Size, Align> {
    using Concept = SFConcept<ReturnType, Args...>;
    static_assert(Align <= alignof(std::max_align_t), "please check your compiler to see if you can really align to greater than std::max_align_t");

public:
    static constexpr std::size_t alignment = std::max(alignof(Concept), Align);
private:
    alignas(alignment) char memory[Size];

    bool allocated = false;
public:

    SmallFun(){}

    // functor and lambda

    template<typename F>
    explicit SmallFun(F &f)
        : allocated(true) {
        using Model = SFModel<F, ReturnType, Args...>;
        static_assert(sizeof( Model) <= Size,      "increase Size template param");
        static_assert(alignof(Model) <= alignment, "increase Align template param");
        new (memory) Model(f);
    }

    template<typename F>
    explicit SmallFun(F &&f)
        : allocated(true) {
        using Model = SFModel<F, ReturnType, Args...>;
        static_assert(sizeof( Model) <= Size,      "increase Size template param");
        static_assert(alignof(Model) <= alignment, "increase Align template param");
        new (memory) Model(std::move(f));
    }

    template<typename F>
    SmallFun& operator=(F &f) {
        using Model = SFModel<F, ReturnType, Args...>;
        static_assert(sizeof( Model) <= Size,      "increase Size template param");
        static_assert(alignof(Model) <= alignment, "increase Align template param");
        clean();
        allocated = true;
        new (memory) Model(f);
        return *this;
    }

    template<typename F>
    SmallFun& operator=(F &&f) {
        using Model = SFModel<F, ReturnType, Args...>;
        static_assert(sizeof( Model) <= Size,      "increase Size template param");
        static_assert(alignof(Model) <= alignment, "increase Align template param");
        clean();
        allocated = true;
        new (memory) Model(std::move(f));
        return *this;
    }



    // plain function pointer
    /*
    explicit SmallFun(ReturnType(*const f)(Args...))
        : allocated(false) { // optimization (function pointer has no destructor)
        using Model = SFModelPlain<ReturnType, Args...>;
        static_assert(sizeof( Model) <= Size,      "increase Size template param");
        static_assert(alignof(Model) <= alignment, "increase Align template param");
        new (memory) Model(f);
    }

    SmallFun& operator=(ReturnType(*const f)(Args...)) {
        using Model = SFModelPlain<ReturnType, Args...>;
        static_assert(sizeof( Model) <= Size,      "increase Size template param");
        static_assert(alignof(Model) <= alignment, "increase Align template param");
        clean();
        allocated = false;  // optimization (function pointer has no destructor)
        new (memory) Model(f);
        return *this;
    }
    */


    // copy constructor

    template<std::size_t s>
    SmallFun(SmallFun<ReturnType(Args...), s> const& other)
        : allocated(other.allocated) {
        static_assert(s <= Size,                    "target's Size too small");
        static_assert(other.alignment <= alignment, "target's alignment is too small");
        other.copy(memory);
    }


    // copy assign

    template<std::size_t s>
    SmallFun& operator=(SmallFun<ReturnType(Args...), s> const& other) {
        static_assert(s <= Size,                    "target's Size too small");
        static_assert(other.alignment <= alignment, "target's alignment is too small");
        clean();
        allocated = other.allocated;
        other.copy(memory);
        return *this;
    }


    // move constructor

    template<std::size_t s>
    SmallFun(SmallFun<ReturnType(Args...), s> && other)
        : allocated(other.allocated) {
        static_assert(s <= Size,                    "target's Size too small");
        static_assert(other.alignment <= alignment, "target's alignment is too small");
        other.move(memory);
    }


    // move assign

    template<std::size_t s>
    SmallFun& operator=(SmallFun<ReturnType(Args...), s> && other) {
        static_assert(s <= Size,                    "target's Size too small");
        static_assert(other.alignment <= alignment, "target's alignment is too small");
        clean();
        allocated = other.allocated;
        other.move(memory);
        return *this;
    }



    void clean() {
        if (allocated) {
            Concept *cpt = reinterpret_cast<Concept*>(memory);
            cpt->~Concept();
            allocated = false;
        }
    }

    ~SmallFun() {
        if (allocated) {
            Concept *cpt = reinterpret_cast<Concept*>(memory);
            cpt->~Concept();
        }
    }

    //template<typename... Args>
    ReturnType operator()(Args&&... args) {
        Concept *cpt = reinterpret_cast<Concept*>(memory);
        return (*cpt)(std::forward<Args>(args)...);
    }

    //template<typename... Args>
    ReturnType operator()(Args&&... args)const {
        const Concept *cpt = reinterpret_cast<const Concept*>(memory);
        return (*cpt)(std::forward<Args>(args)...);
    }

    /*
    void copy(void* data)const {
        if (allocated) {
            Concept *cpt = reinterpret_cast<Concept*>(memory);
            cpt->copy(data);
        }
    }
    */
};

}

#endif
