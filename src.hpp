#ifndef SRC_HPP
#define SRC_HPP

#include <stdexcept>
#include <initializer_list>
#include <typeinfo>

namespace sjtu {

class any_ptr {
 private:
  // Base class for type erasure
  struct holder_base {
    size_t ref_count;

    holder_base() : ref_count(1) {}
    virtual ~holder_base() {}
    virtual const std::type_info& type() const = 0;
  };

  // Templated holder for actual data
  template <typename T>
  struct holder : holder_base {
    T* data;

    holder(T* ptr) : data(ptr) {}

    ~holder() override {
      delete data;
    }

    const std::type_info& type() const override {
      return typeid(T);
    }
  };

  holder_base* ptr_;

  void increment_ref() {
    if (ptr_) {
      ptr_->ref_count++;
    }
  }

  void decrement_ref() {
    if (ptr_ && --ptr_->ref_count == 0) {
      delete ptr_;
      ptr_ = nullptr;
    }
  }

 public:
  /**
   * @brief 默认构造函数，行为应与创建空指针类似
   */
  any_ptr() : ptr_(nullptr) {}

  /**
   * @brief 拷贝构造函数，要求拷贝的层次为浅拷贝，即该对象与被拷贝对象的内容指向同一块内存
   * @note 若将指针作为参数传入，则指针的生命周期由该对象管理
   * @example
   *  any_ptr a = make_any_ptr(1);
   *  any_ptr b = a;
   *  a.unwrap<int> = 2;
   *  std::cout << b << std::endl; // 2
   * @param other
   */
  any_ptr(const any_ptr &other) : ptr_(other.ptr_) {
    increment_ref();
  }

  template <class T>
  any_ptr(T *ptr) : ptr_(new holder<T>(ptr)) {}

  /**
   * @brief 析构函数，注意若一块内存被多个对象共享，那么只有最后一个析构的对象需要释放内存
   * @example
   *  any_ptr a = make_any_ptr(1);
   *  {
   *    any_ptr b = a;
   *  }
   *  std::cout << a << std::endl; // valid
   * @example
   *  int *p = new int(1);
   *  {
   *    any_ptr a = p;
   *  }
   *  std::cout << *p << std::endl; // invalid
   */
  ~any_ptr() {
    decrement_ref();
  }

  /**
   * @brief 拷贝赋值运算符，要求拷贝的层次为浅拷贝，即该对象与被拷贝对象的内容指向同一块内存
   * @note 应与指针拷贝赋值运算符的语义相近
   * @param other
   * @return any_ptr&
   */
  any_ptr &operator=(const any_ptr &other) {
    if (this != &other) {
      decrement_ref();
      ptr_ = other.ptr_;
      increment_ref();
    }
    return *this;
  }

  template <class T>
  any_ptr &operator=(T *ptr) {
    decrement_ref();
    ptr_ = new holder<T>(ptr);
    return *this;
  }

  /**
   * @brief 获取该对象指向的值的引用
   * @note 若该对象指向的值不是 T 类型，则抛出异常 std::bad_cast
   * @example
   *  any_ptr a = make_any_ptr(1);
   *  std::cout << a.unwrap<int>() << std::endl; // 1
   * @tparam T
   * @return T&
   */
  template <class T>
  T &unwrap() {
    if (!ptr_) {
      throw std::bad_cast();
    }
    if (ptr_->type() != typeid(T)) {
      throw std::bad_cast();
    }
    return *(static_cast<holder<T>*>(ptr_)->data);
  }

  template <class T>
  const T &unwrap() const {
    if (!ptr_) {
      throw std::bad_cast();
    }
    if (ptr_->type() != typeid(T)) {
      throw std::bad_cast();
    }
    return *(static_cast<holder<T>*>(ptr_)->data);
  }
};

/**
 * @brief 由指定类型的值构造一个 any_ptr 对象
 * @example
 *  any_ptr a = make_any_ptr(1);
 *  any_ptr v = make_any_ptr<std::vector<int>>(1, 2, 3);
 *  any_ptr m = make_any_ptr<std::map<int, int>>({{1, 2}, {3, 4}});
 * @tparam T
 * @param t
 * @return any_ptr
 */
template <class T>
any_ptr make_any_ptr(const T &t) {
  return any_ptr(new T(t));
}

// Support for variadic arguments with initializer_list
template <class T, typename... Args>
any_ptr make_any_ptr(Args&&... args) {
  return any_ptr(new T{std::forward<Args>(args)...});
}

}  // namespace sjtu

#endif
