#include "pmr_queue.h"

int main() {
    DynamicMemoryResource dynamic_mr;
    
    PmrQueue<int> int_queue(&dynamic_mr);
    
    for (int i = 1; i <= 5; ++i) {
        int_queue.push(i);
    }
    
    while (!int_queue.empty()) {
        int_queue.pop();
    }
    
    PmrQueue<ComplexType> complex_queue(&dynamic_mr);
    
    complex_queue.push(ComplexType(1, 3.14, "Первый"));
    complex_queue.push(ComplexType(2, 2.71, "Второй"));
    complex_queue.push(ComplexType(3, 1.41, "Третий"));
    
    PmrQueue<ComplexType> moved_queue = std::move(complex_queue);
    
    return 0;
}
