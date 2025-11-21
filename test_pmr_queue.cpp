#include <gtest/gtest.h>
#include "pmr_queue.h"

TEST(DynamicMemoryResourceTest, BasicAllocationDeallocation) {
    DynamicMemoryResource mr;
    
    void* ptr1 = mr.allocate(100);
    void* ptr2 = mr.allocate(200);
    
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    
    mr.deallocate(ptr1, 100);
    mr.deallocate(ptr2, 200);
}

TEST(DynamicMemoryResourceTest, CleanupOnDestruction) {
    size_t allocations_before = 0;
    {
        DynamicMemoryResource mr;
        mr.allocate(100);
        mr.allocate(200);
        mr.allocate(300);
    }
}

TEST(PmrQueueTest, EmptyQueue) {
    PmrQueue<int> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST(PmrQueueTest, PushAndSize) {
    PmrQueue<int> queue;
    
    queue.push(1);
    queue.push(2);
    queue.push(3);
    
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 3);
}

TEST(PmrQueueTest, FrontAndPop) {
    PmrQueue<int> queue;
    
    queue.push(10);
    queue.push(20);
    
    EXPECT_EQ(queue.front(), 10);
    queue.pop();
    EXPECT_EQ(queue.front(), 20);
    queue.pop();
    EXPECT_TRUE(queue.empty());
}

TEST(PmrQueueTest, Iterator) {
    PmrQueue<int> queue;
    
    queue.push(1);
    queue.push(2);
    queue.push(3);
    
    auto it = queue.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    it++;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(it, queue.end());
}

TEST(PmrQueueTest, RangeBasedFor) {
    PmrQueue<int> queue;
    
    queue.push(1);
    queue.push(2);
    queue.push(3);
    
    int sum = 0;
    for (const auto& item : queue) {
        sum += item;
    }
    
    EXPECT_EQ(sum, 6);
}

TEST(PmrQueueTest, MoveSemantics) {
    PmrQueue<int> queue1;
    queue1.push(1);
    queue1.push(2);
    
    PmrQueue<int> queue2 = std::move(queue1);
    
    EXPECT_TRUE(queue1.empty());
    EXPECT_FALSE(queue2.empty());
    EXPECT_EQ(queue2.size(), 2);
    EXPECT_EQ(queue2.front(), 1);
}

TEST(PmrQueueTest, ComplexType) {
    PmrQueue<ComplexType> queue;
    
    queue.push(ComplexType(1, 3.14, "first"));
    queue.push(ComplexType(2, 2.71, "second"));
    
    EXPECT_EQ(queue.size(), 2);
    EXPECT_EQ(queue.front().id, 1);
    EXPECT_EQ(queue.front().name, "first");
    
    queue.pop();
    EXPECT_EQ(queue.front().id, 2);
}

TEST(PmrQueueTest, CustomMemoryResource) {
    DynamicMemoryResource custom_mr;
    PmrQueue<int> queue(&custom_mr);
    
    queue.push(100);
    queue.push(200);
    queue.push(300);
    
    EXPECT_EQ(queue.size(), 3);
    
    while (!queue.empty()) {
        queue.pop();
    }
}

TEST(PmrQueueTest, ForwardIteratorRequirements) {
    PmrQueue<int> queue;
    queue.push(1);
    queue.push(2);
    
    auto it = queue.begin();
    static_assert(std::is_same_v<typename std::iterator_traits<decltype(it)>::iterator_category, 
                                std::forward_iterator_tag>);
    
    EXPECT_TRUE(std::forward_iterator<decltype(it)>);
}

TEST(PmrQueueTest, Clear) {
    PmrQueue<int> queue;
    
    queue.push(1);
    queue.push(2);
    queue.push(3);
    
    EXPECT_EQ(queue.size(), 3);
    queue.clear();
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
