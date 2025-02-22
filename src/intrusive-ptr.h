#pragma once

#include <atomic>
#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>

template <typename T>
struct intrusive_ptr {
public:
  using element_type = T;
  using pointer = T*;
  using reference = T&;

  template <typename Y>
  friend struct intrusive_ptr;

public:
  intrusive_ptr() noexcept = default;

  explicit intrusive_ptr(pointer p, bool add_ref = true)
      : _ptr(p) {
    if (p != nullptr && add_ref) {
      intrusive_ptr_add_ref(p);
    }
  }

  intrusive_ptr(const intrusive_ptr& r)
      : intrusive_ptr(r._ptr, true) {}

  template <class Y>
  intrusive_ptr(const intrusive_ptr<Y>& r)
    requires (std::is_convertible_v<Y*, T*>)
      : intrusive_ptr(r._ptr, true) {}

  intrusive_ptr(intrusive_ptr&& r) noexcept
      : _ptr(std::exchange(r._ptr, nullptr)) {}

  template <class Y>
  explicit intrusive_ptr(intrusive_ptr<Y>&& r)
    requires (std::is_convertible_v<Y*, T*>)
  {
    _ptr = std::exchange(r._ptr, nullptr);
  }

  ~intrusive_ptr() {
    if (get() != nullptr) {
      intrusive_ptr_release(get());
    }
  }

  intrusive_ptr& operator=(const intrusive_ptr& r) {
    if (this != &r) {
      intrusive_ptr(r).swap(*this);
    }
    return *this;
  }

  template <class Y>
  intrusive_ptr& operator=(const intrusive_ptr<Y>& r)
    requires (std::is_convertible_v<Y*, T*>)
  {
    intrusive_ptr(r).swap(*this);
    return *this;
  }

  intrusive_ptr& operator=(pointer r) {
    intrusive_ptr(r).swap(*this);
    return *this;
  }

  intrusive_ptr& operator=(intrusive_ptr&& r) noexcept {
    if (this != &r) {
      intrusive_ptr(std::move(r)).swap(*this);
    }
    return *this;
  }

  template <class Y>
  intrusive_ptr& operator=(intrusive_ptr<Y>&& r)
    requires (std::is_convertible_v<Y*, T*>)
  {
    intrusive_ptr(std::move(r)).swap(*this);
    return *this;
  }

  void reset() noexcept {
    intrusive_ptr().swap(*this);
  }

  void reset(pointer r) noexcept {
    intrusive_ptr(r).swap(*this);
  }

  void reset(pointer r, bool add_ref) noexcept {
    intrusive_ptr(r, add_ref).swap(*this);
  }

  reference operator*() const noexcept {
    assert(_ptr != nullptr);
    return *operator->();
  }

  pointer operator->() const noexcept {
    assert(_ptr != nullptr);
    return _ptr;
  }

  pointer get() const noexcept {
    return _ptr;
  }

  pointer detach() noexcept {
    return std::exchange(_ptr, nullptr);
  }

  explicit operator bool() const noexcept {
    return (_ptr != nullptr);
  }

  void swap(intrusive_ptr& b) noexcept {
    std::swap(b._ptr, _ptr);
  }

private:
  pointer _ptr{nullptr};
};

template <class T, class U>
bool operator==(const intrusive_ptr<T>& a, const intrusive_ptr<U>& b) noexcept {
  return (a.get() == b.get());
}

template <class T, class U>
bool operator!=(const intrusive_ptr<T>& a, const intrusive_ptr<U>& b) noexcept {
  return !(a.get() == b.get());
}

template <class T, class U>
bool operator==(const intrusive_ptr<T>& a, U* b) noexcept {
  return (a.get() == b);
}

template <class T, class U>
bool operator!=(const intrusive_ptr<T>& a, U* b) noexcept {
  return !(a.get() == b);
}

template <class T, class U>
bool operator==(T* a, const intrusive_ptr<U>& b) noexcept {
  return (a == b.get());
}

template <class T, class U>
bool operator!=(T* a, const intrusive_ptr<U>& b) noexcept {
  return !(a == b.get());
}

template <class T>
bool operator<(const intrusive_ptr<T>& a, const intrusive_ptr<T>& b) noexcept {
  return std::less<T*>()(a.get(), b.get());
}

template <class T>
void swap(intrusive_ptr<T>& a, intrusive_ptr<T>& b) noexcept {
  a.swap(b);
}

template <typename T>
struct intrusive_ref_counter {
public:
  using element_type = T;
  using pointer = T*;
  using reference = T&;

public:
  intrusive_ref_counter() noexcept = default;

  intrusive_ref_counter(const intrusive_ref_counter&) noexcept {}

  intrusive_ref_counter& operator=(const intrusive_ref_counter&) noexcept {
    return *this;
  }

  unsigned int use_count() const noexcept {
    return _counter.load(std::memory_order_acquire);
  }

  void add_ref() const noexcept {
    _counter.fetch_add(1, std::memory_order_relaxed);
  }

  void release() const noexcept {
    _counter.fetch_sub(1, std::memory_order_acq_rel);
  }

protected:
  ~intrusive_ref_counter() = default;

private:
  mutable std::atomic<unsigned int> _counter{0};
};

template <class Derived>
void intrusive_ptr_add_ref(const intrusive_ref_counter<Derived>* p) noexcept {
  if (p != nullptr) {
    p->add_ref();
  }
}

template <class Derived>
void intrusive_ptr_release(const intrusive_ref_counter<Derived>* p) noexcept {
  if (p != nullptr) {
    p->release();
    if (p->use_count() == 0) {
      delete static_cast<const Derived*>(p);
    }
  }
}
