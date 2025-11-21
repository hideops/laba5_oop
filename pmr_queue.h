#pragma once

#include <memory_resource>
#include <list>
#include <iterator>
#include <iostream>
#include <cstddef>
#include <algorithm>
#include <utility>

class DynamicMemoryResource final : public std::pmr::memory_resource 
{
    struct Block 
    {
        void* address;
        std::size_t bytes;
        Block(void* addr, std::size_t size) : address(addr), bytes(size) {}
    };
    
    std::list<Block> active_blocks;
    std::pmr::memory_resource* parent_resource;

public:
    explicit DynamicMemoryResource(std::pmr::memory_resource* parent = std::pmr::get_default_resource()) 
        : parent_resource(parent) 
    {
    }
    
    DynamicMemoryResource(const DynamicMemoryResource&) = delete;
    DynamicMemoryResource& operator=(const DynamicMemoryResource&) = delete;
    
    ~DynamicMemoryResource() 
    {
        for (const Block& block : active_blocks) 
        {
            parent_resource->deallocate(block.address, block.bytes);
        }
    }

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override 
    {
        void* new_block = parent_resource->allocate(bytes, alignment);
        
        try 
        {
            active_blocks.emplace_back(new_block, bytes);
        } 
        catch (...) 
        {
            parent_resource->deallocate(new_block, bytes, alignment);
            throw;
        }
        
        return new_block;
    }
    
    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override 
    {
        auto found = std::find_if(active_blocks.begin(), active_blocks.end(),
            [ptr](const Block& block) { return block.address == ptr; });
        
        if (found != active_blocks.end()) 
        {
            parent_resource->deallocate(ptr, bytes, alignment);
            active_blocks.erase(found);
        }
    }
    
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override 
    {
        return this == &other;
    }
};

template<typename T>
struct QueueNode 
{
    T data;
    QueueNode* next_node;
    
    template<typename... Args>
    QueueNode(Args&&... args) 
        : data(std::forward<Args>(args)...), next_node(nullptr) 
    {
    }
};

template<typename T>
class QueueIterator 
{
    QueueNode<T>* current_node;

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    explicit QueueIterator(QueueNode<T>* node = nullptr) : current_node(node) {}

    QueueIterator& operator++() 
    {
        if (current_node) 
        {
            current_node = current_node->next_node;
        }
        return *this;
    }

    QueueIterator operator++(int) 
    {
        QueueIterator old = *this;
        ++(*this);
        return old;
    }

    reference operator*() const 
    { 
        return current_node->data; 
    }
    
    pointer operator->() const 
    { 
        return &current_node->data; 
    }

    bool operator==(const QueueIterator& other) const 
    {
        return current_node == other.current_node;
    }
    
    bool operator!=(const QueueIterator& other) const 
    {
        return current_node != other.current_node;
    }
};

template<typename T>
class PmrQueue 
{
    using NodeAllocator = std::pmr::polymorphic_allocator<QueueNode<T>>;
    
    QueueNode<T>* head_node;
    QueueNode<T>* tail_node;
    NodeAllocator node_allocator;
    std::size_t element_count;

public:
    using iterator = QueueIterator<T>;
    using const_iterator = QueueIterator<const T>;

    explicit PmrQueue(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) 
        : head_node(nullptr), tail_node(nullptr), node_allocator(resource), element_count(0) 
    {
    }
    
    ~PmrQueue() 
    {
        clear_elements();
    }
    
    PmrQueue(const PmrQueue&) = delete;
    PmrQueue& operator=(const PmrQueue&) = delete;
    
    PmrQueue(PmrQueue&& other) noexcept 
        : head_node(other.head_node), 
          tail_node(other.tail_node), 
          node_allocator(other.node_allocator), 
          element_count(other.element_count) 
    {
        other.head_node = nullptr;
        other.tail_node = nullptr;
        other.element_count = 0;
    }
    
    PmrQueue& operator=(PmrQueue&& other) noexcept 
    {
        if (this != &other) 
        {
            clear_elements();
            
            head_node = other.head_node;
            tail_node = other.tail_node;
            node_allocator = other.node_allocator;
            element_count = other.element_count;
            
            other.head_node = nullptr;
            other.tail_node = nullptr;
            other.element_count = 0;
        }
        return *this;
    }
    
    template<typename U>
    void push(U&& element) 
    {
        QueueNode<T>* new_node = node_allocator.allocate(1);
        try 
        {
            node_allocator.construct(new_node, std::forward<U>(element));
        } 
        catch (...) 
        {
            node_allocator.deallocate(new_node, 1);
            throw;
        }
        
        if (tail_node) 
        {
            tail_node->next_node = new_node;
        } 
        else 
        {
            head_node = new_node;
        }
        tail_node = new_node;
        ++element_count;
    }
    
    void pop() 
    {
        if (!head_node) return;
        
        QueueNode<T>* old_head = head_node;
        head_node = head_node->next_node;
        
        if (!head_node) 
        {
            tail_node = nullptr;
        }
        
        node_allocator.destroy(old_head);
        node_allocator.deallocate(old_head, 1);
        --element_count;
    }
    
    T& front() 
    { 
        return head_node->data; 
    }
    
    const T& front() const 
    { 
        return head_node->data; 
    }
    
    bool empty() const 
    { 
        return head_node == nullptr; 
    }
    
    std::size_t size() const 
    { 
        return element_count; 
    }
    
    void clear() 
    {
        clear_elements();
    }
    
    iterator begin() 
    { 
        return iterator(head_node); 
    }
    
    iterator end() 
    { 
        return iterator(nullptr); 
    }
    
    const_iterator begin() const 
    { 
        return const_iterator(head_node); 
    }
    
    const_iterator end() const 
    { 
        return const_iterator(nullptr); 
    }
    
    const_iterator cbegin() const 
    { 
        return const_iterator(head_node); 
    }
    
    const_iterator cend() const 
    { 
        return const_iterator(nullptr); 
    }

private:
    void clear_elements() 
    {
        while (!empty()) 
        {
            pop();
        }
    }
};

struct ComplexType 
{
    int identifier;
    double data_value;
    std::string text_name;
    
    ComplexType(int id, double value, const std::string& name) 
        : identifier(id), data_value(value), text_name(name) 
    {
    }
    
    friend std::ostream& operator<<(std::ostream& output, const ComplexType& obj) 
    {
        output << "ComplexType{id=" << obj.identifier << ", value=" << obj.data_value 
               << ", name=\"" << obj.text_name << "\"}";
        return output;
    }
    
    bool operator==(const ComplexType& other) const 
    {
        return identifier == other.identifier && 
               data_value == other.data_value && 
               text_name == other.text_name;
    }
};
