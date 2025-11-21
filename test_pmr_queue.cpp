#include <gtest/gtest.h>
#include "pmr_queue.h"

TEST(DynamicMemoryResourceTest, BasicAllocationDeallocation) {
    DynamicMemoryResource mr;
    
    void* ptr1 = mr.allocate(47734);
    void* ptr2 = mr.allocate(12845);
    
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    
    mr.deallocate(ptr1, 47734);
    mr.deallocate(ptr2, 12845);
}

TEST(DynamicMemoryResourceTest, CleanupOnDestruction) {
    {
        DynamicMemoryResource mr;
        mr.allocate(58291);
        mr.allocate(37465);
        mr.allocate(92634);
    }
}

TEST(PmrQueueTest, EmptyQueue) {
    PmrQueue<int> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST(PmrQueueTest, PushAndSize) {
    PmrQueue<int> queue;
    
    queue.push(47734);
    queue.push(12845);
    queue.push(29387);
    
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 3);
}

TEST(PmrQueueTest, FrontAndPop) {
    PmrQueue<int> queue;
    
    queue.push(58291);
    queue.push(37465);
    
    EXPECT_EQ(queue.front(), 58291);
    queue.pop();
    EXPECT_EQ(queue.front(), 37465);
    queue.pop();
    EXPECT_TRUE(queue.empty());
}

TEST(PmrQueueTest, Iterator) {
    PmrQueue<int> queue;
    
    queue.push(92634);
    queue.push(65723);
    queue.push(83451);
    
    auto it = queue.begin();
    EXPECT_EQ(*it, 92634);
    ++it;
    EXPECT_EQ(*it, 65723);
    it++;
    EXPECT_EQ(*it, 83451);
    ++it;
    EXPECT_EQ(it, queue.end());
}

TEST(PmrQueueTest, RangeBasedFor) {
    PmrQueue<int> queue;
    
    queue.push(12345);
    queue.push(67890);
    queue.push(54321);
    
    int sum = 0;
    for (const auto& item : queue) {
        sum += item;
    }
    
    EXPECT_EQ(sum, 134556);
}

TEST(PmrQueueTest, MoveSemantics) {
    PmrQueue<int> queue1;
    queue1.push(11111);
    queue1.push(22222);
    
    PmrQueue<int> queue2 = std::move(queue1);
    
    EXPECT_TRUE(queue1.empty());
    EXPECT_FALSE(queue2.empty());
    EXPECT_EQ(queue2.size(), 2);
    EXPECT_EQ(queue2.front(), 11111);
}

TEST(PmrQueueTest, ComplexType) {
    PmrQueue<ComplexType> queue;
    
    queue.push(ComplexType(47734, 58291.58291, "first"));
    queue.push(ComplexType(12845, 37465.37465, "second"));
    
    EXPECT_EQ(queue.size(), 2);
    EXPECT_EQ(queue.front().identifier, 47734);
    EXPECT_EQ(queue.front().text_name, "first");
    
    queue.pop();
    EXPECT_EQ(queue.front().identifier, 12845);
}

TEST(PmrQueueTest, CustomMemoryResource) {
    DynamicMemoryResource custom_mr;
    PmrQueue<int> queue(&custom_mr);
    
    queue.push(99999);
    queue.push(88888);
    queue.push(77777);
    
    EXPECT_EQ(queue.size(), 3);
    
    while (!queue.empty()) {
        queue.pop();
    }
}

TEST(PmrQueueTest, ForwardIteratorRequirements) {
    PmrQueue<int> queue;
    queue.push(13579);
    queue.push(24680);
    
    auto it = queue.begin();
    static_assert(std::is_same_v<typename std::iterator_traits<decltype(it)>::iterator_category, 
                                std::forward_iterator_tag>);
    
    EXPECT_TRUE(std::forward_iterator<decltype(it)>);
}

TEST(PmrQueueTest, Clear) {
    PmrQueue<int> queue;
    
    queue.push(33333);
    queue.push(44444);
    queue.push(55555);
    
    EXPECT_EQ(queue.size(), 3);
    queue.clear();
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST(PmrQueueTest, LargeNumbers) {
    PmrQueue<long long> queue;
    
    queue.push(1234567890);
    queue.push(9876543210);
    queue.push(5555555555);
    
    EXPECT_EQ(queue.size(), 3);
    EXPECT_EQ(queue.front(), 1234567890);
    
    queue.pop();
    EXPECT_EQ(queue.front(), 9876543210);
}

TEST(PmrQueueTest, MixedLargeNumbers) {
    PmrQueue<int> queue;
    
    queue.push(100000);
    queue.push(200000);
    queue.push(300000);
    queue.push(400000);
    queue.push(500000);
    
    EXPECT_EQ(queue.size(), 5);
    
    int expected_values[] = {100000, 200000, 300000, 400000, 500000};
    int index = 0;
    for (const auto& item : queue) {
        EXPECT_EQ(item, expected_values[index++]);
    }
}

TEST(PmrQueueTest, PrimeLikeNumbers) {
    PmrQueue<int> queue;
    
    queue.push(104729);
    queue.push(104743);
    queue.push(104759);
    queue.push(104773);
    
    EXPECT_EQ(queue.size(), 4);
    
    auto it = queue.begin();
    EXPECT_EQ(*it, 104729);
    ++it;
    EXPECT_EQ(*it, 104743);
    ++it;
    EXPECT_EQ(*it, 104759);
    ++it;
    EXPECT_EQ(*it, 104773);
}

TEST(PmrQueueTest, ComplexTypeWithLargeNumbers) {
    PmrQueue<ComplexType> queue;
    
    queue.push(ComplexType(123456, 789012.345, "large_object_1"));
    queue.push(ComplexType(654321, 210987.654, "large_object_2"));
    queue.push(ComplexType(999999, 888888.888, "large_object_3"));
    
    EXPECT_EQ(queue.size(), 3);
    
    auto it = queue.begin();
    EXPECT_EQ(it->identifier, 123456);
    EXPECT_EQ(it->data_value, 789012.345);
    ++it;
    EXPECT_EQ(it->identifier, 654321);
    EXPECT_EQ(it->data_value, 210987.654);
    ++it;
    EXPECT_EQ(it->identifier, 999999);
    EXPECT_EQ(it->data_value, 888888.888);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
