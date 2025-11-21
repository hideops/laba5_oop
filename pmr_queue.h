#pragma once
#include <memory_resource>
#include <list>
#include <iterator>
#include <iostream>
#include <cstddef>

class DynamicMemoryResource : public std::pmr::memory_resource {
private:
    struct BlockInfo {
        void* ptr;
        std::size_t size;
        BlockInfo(void* p, std::size_t s) : ptr(p), size(s) {}
    };
    
    std::list<BlockInfo> allocated_blocks_;
    std::pmr::memory_resource* upstream_;

public:
    explicit DynamicMemoryResource(std::pmr::memory_resource* upstream = 
                                  std::pmr::get_default_resource()) 
        : upstream_(upstream) {}
    
    DynamicMemoryResource(const DynamicMemoryResource&) = delete;
    DynamicMemoryResource& operator=(const DynamicMemoryResource&) = delete;
    
    ~DynamicMemoryResource() override {
        for (const auto& block : allocated_blocks_) {
            upstream_->deallocate(block.ptr, block.size);
        }
    }

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* ptr = upstream_->allocate(bytes, alignment);
        try {
            allocated_blocks_.emplace_back(ptr, bytes);
        } catch (...) {
            upstream_->deallocate(ptr, bytes, alignment);
            throw;
        }
        return ptr;
    }
    
    void do_deallocate(void* ptr, std::size_t bytes, 
                      std::size_t alignment) override {
        auto it = std::find_if(allocated_blocks_.begin(), allocated_blocks_.end(),
                              [ptr](const BlockInfo& block) {
                                  return block.ptr == ptr;
                              });
        
        if (it != allocated_blocks_.end()) {
            upstream_->deallocate(ptr, bytes, alignment);
            allocated_blocks_.erase(it);
        }
    }
    
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

template<typename T>
struct QueueNode {
    T value;
    QueueNode* next;
    
    template<typename... Args>
    QueueNode(Args&&... args) 
        : value(std::forward<Args>(args)...), next(nullptr) {}
};

template<typename T>
class QueueIterator {
private:
    QueueNode<T>* current_;

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    explicit QueueIterator(QueueNode<T>* node = nullptr) : current_(node) {}

    QueueIterator& operator++() {
        if (current_) {
            current_ = current_->next;
        }
        return *this;
    }

    QueueIterator operator++(int) {
        QueueIterator temp = *this;
        ++(*this);
        return temp;
    }

    reference operator*() const { 
        return current_->value; 
    }
    
    pointer operator->() const { 
        return &current_->value; 
    }

    bool operator==(const QueueIterator& other) const {
        return current_ == other.current_;
    }
    
    bool operator!=(const QueueIterator& other) const {
        return !(*this == other);
    }
};

template<typename T>
class PmrQueue {
private:
    using allocator_type = std::pmr::polymorphic_allocator<QueueNode<T>>;
    
    QueueNode<T>* head_;
    QueueNode<T>* tail_;
    allocator_type allocator_;
    std::size_t size_;

public:
    using iterator = QueueIterator<T>;
    using const_iterator = QueueIterator<const T>;

    explicit PmrQueue(std::pmr::memory_resource* mr = 
                     std::pmr::get_default_resource()) 
        : head_(nullptr), tail_(nullptr), allocator_(mr), size_(0) {}
    
    ~PmrQueue() {
        clear();
    }
    
    PmrQueue(const PmrQueue&) = delete;
    PmrQueue& operator=(const PmrQueue&) = delete;
    
    PmrQueue(PmrQueue&& other) noexcept 
        : head_(other.head_), tail_(other.tail_), 
          allocator_(other.allocator_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }
    
    PmrQueue& operator=(PmrQueue&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = other.head_;
            tail_ = other.tail_;
            allocator_ = other.allocator_;
            size_ = other.size_;
            
            other.head_ = nullptr;
            other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }
    
    template<typename U>
    void push(U&& value) {
        QueueNode<T>* new_node = allocator_.allocate(1);
        try {
            allocator_.construct(new_node, std::forward<U>(value));
        } catch (...) {
            allocator_.deallocate(new_node, 1);
            throw;
        }
        
        if (tail_) {
            tail_->next = new_node;
        } else {
            head_ = new_node;
        }
        tail_ = new_node;
        ++size_;
    }
    
    void pop() {
        if (!head_) return;
        
        QueueNode<T>* old_head = head_;
        head_ = head_->next;
        
        if (!head_) {
            tail_ = nullptr;
        }
        
        allocator_.destroy(old_head);
        allocator_.deallocate(old_head, 1);
        --size_;
    }
    
    T& front() { 
        return head_->value; 
    }
    
    const T& front() const { 
        return head_->value; 
    }
    
    bool empty() const { 
        return head_ == nullptr; 
    }
    
    std::size_t size() const { 
        return size_; 
    }
    
    void clear() {
        while (!empty()) {
            pop();
        }
    }
    
    iterator begin() { 
        return iterator(head_); 
    }
    
    iterator end() { 
        return iterator(nullptr); 
    }
    
    const_iterator begin() const { 
        return const_iterator(head_); 
    }
    
    const_iterator end() const { 
        return const_iterator(nullptr); 
    }
    
    const_iterator cbegin() const { 
        return const_iterator(head_); 
    }
    
    const_iterator cend() const { 
        return const_iterator(nullptr); 
    }
};

struct ComplexType {
    int id;
    double value;
    std::string name;
    
    ComplexType(int i, double v, const std::string& n) 
        : id(i), value(v), name(n) {}
    
    friend std::ostream& operator<<(std::ostream& os, const ComplexType& ct) {
        os << "ComplexType{id=" << ct.id << ", value=" << ct.value 
           << ", name=\"" << ct.name << "\"}";
        return os;
    }
    
    bool operator==(const ComplexType& other) const {
        return id == other.id && value == other.value && name == other.name;
    }
};
