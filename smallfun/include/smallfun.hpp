#ifndef SMALLFUNCTION_SMALLFUNCTION_HPP
#define SMALLFUNCTION_SMALLFUNCTION_HPP

#include <type_traits>
#include <utility>
#include <cstddef>

namespace smallfun {


template<class ReturnType, class...Xs>
struct SFConcept {
  virtual ReturnType operator()(Xs...)const = 0;
  virtual void copy(void*)const = 0;
  virtual ~SFConcept() {};
};

template<class F, class ReturnType, class...Xs>
struct SFModel final
  : SFConcept<ReturnType, Xs...> {
  mutable F f;

  SFModel(F const& f)
    : f(f)
  {}

  void copy(void* memory)       const override final {
    new (memory) SFModel(f);
  }

  ReturnType operator()(Xs...xs)const override final {
    return f(xs...);
  }

};



template<class Signature, std::size_t size=128>
struct SmallFun;

template<class ReturnType, class...Xs, std::size_t size>
class SmallFun<ReturnType(Xs...), size> {
  static constexpr std::size_t alignment = alignof(std::max_align_t);
  alignas(alignment) char memory[size];

  bool allocated = false;
  using Concept = SFConcept<ReturnType, Xs...>;
public:
  SmallFun(){}

  template<class F>
  SmallFun(F const&f)
    : allocated(true) {
      using Model = SFModel<F, ReturnType, Xs...>;
      static_assert(sizeof(Model) <= size, "");
      new (memory) Model(f);
  }

  template<class F>
  SmallFun& operator=(F const&f) {
    using Model = SFModel<F, ReturnType, Xs...>;
    static_assert(sizeof(Model) <= size, "");
    clean();
    allocated = true;
    new (memory) Model(f);
    return *this;
  }


  // copy constructor

  template<std::size_t s>
  SmallFun(SmallFun<ReturnType(Xs...), s> const& sf)
    : allocated(sf.allocated) {
    static_assert(s <= size, "");
    sf.copy(memory);
  }


  // copy assign

  template<std::size_t s>
  SmallFun& operator=(SmallFun<ReturnType(Xs...), s> const& sf) {
    static_assert(s <= size, "");
    clean();
    allocated = sf.allocated;
    sf.copy(memory);
    return *this;
  }

  void clean() {
    if (allocated) {
      Concept *cpt = reinterpret_cast<Concept*>(memory);
      cpt->~Concept();
      allocated = 0;
    }
  }

  ~SmallFun() {
    if (allocated) {
      Concept *cpt = reinterpret_cast<Concept*>(memory);
      cpt->~Concept();
    }
  }

  template<class...Ys>
  ReturnType operator()(Ys&&...ys) {
    Concept *cpt = reinterpret_cast<Concept*>(memory);
    return (*cpt)(std::forward<Ys>(ys)...);
  }

  template<class...Ys>
  ReturnType operator()(Ys&&...ys)const {
    Concept *cpt = reinterpret_cast<Concept*>(memory);
    return (*cpt)(std::forward<Ys>(ys)...);
  }

  void copy(void* data)const {
    if (allocated) {
      Concept *cpt = reinterpret_cast<Concept*>(memory);
      cpt->copy(data);
    }
  }
};

}

#endif
