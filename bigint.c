#include "bigint.h"

struct bigint add(struct bigint a, struct bigint b) {
    struct bigint result;

    // Add the low parts
    result.low = a.low + b.low;
    
    // Handle overflow of the `low` part
    result.high = a.high + b.high;
    if (result.low >= 10000000) {
        result.low -= 10000000; // Keep only the least significant 10 digits
        result.high++;             // Add the carry to the high part
    }

    return result;
}

struct bigint subtract(struct bigint a, struct bigint b) {
    struct bigint result;

    // Subtract the low parts
    result.low = a.low - b.low;
    
    // Handle underflow of the `low` part
    result.high = a.high - b.high;
    if (result.low < 0) {
        result.low += 10000000; // Compensate for the underflow
        result.high--;           // Remove the borrow from the high part
    }

    return result;
}

int compare(struct bigint A, struct bigint B) {
    // Compare the high parts
    if (A.high > B.high) {
        return 0; // A is bigger than B
    } else if (A.high < B.high) {
        return 1; // A is smaller than B
    } 
    // If the high parts are the same, compare the low parts
    else {
        if (A.low > B.low) {
            return 0; // A is bigger than B
        } else if (A.low < B.low) {
            return 1; // A is smaller than B
        } else {
            return 2; // A and B are the same
        }
    }
}

