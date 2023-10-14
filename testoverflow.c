#include "types.h"
#include "user.h"

struct bigint {
    int high;  // Most significant 32 bits
    int low;   // Least significant 32 bits
};

struct bigint add(struct bigint a, struct bigint b) {
    struct bigint result;
    result.low = a.low + b.low;

    // Handle overflow of the `low` part
    result.high = a.high + b.high + (result.low < a.low);

    return result;
}

struct bigint subtract(struct bigint a, struct bigint b) {
    struct bigint result;
    result.low = a.low - b.low;

    // Handle underflow of the `low` part
    result.high = a.high - b.high - (a.low < result.low);

    return result;
}

void print(struct bigint num) {
    // Base case: If high is 0 and low is non-negative, print low directly
    if (num.high == 0 && num.low >= 0) {
        printf(1, "%d", num.low);
        return;
    }

    // Handle negative numbers
    if (num.high < 0 || (num.high == 0 && num.low < 0)) {
        printf(1, "-");
        num.high = -num.high;
        if (num.low != 0) {
            num.low = -(num.low - 1); //revere two's complement to get magnitude
            num.high--;
        } else {
            num.low = 0;
        }
    }

    // Compute the effective large value
    uint effective_low;
    int effective_high = num.high;

    if (num.low < 0) {
        effective_low = (1u << 31) - (-num.low); // Convert negative to positive counter part in 32 bit system 
        effective_high--;
    } else {
        effective_low = num.low;
    }

    // Now, print the effective large value
    if (effective_high > 0) {
        // Here, we use the division method to find the quotient and remainder
        int quotient = effective_low / 1000000000;
        int remainder = effective_low % 1000000000;
        printf(1, "%d%d", effective_high * 2 + quotient, remainder); 
    } else {
        printf(1, "%d", effective_low);
    }
}


int main()
{
    struct bigint num1 = {-5, 131071};
    print(num1);
    printf(1, "\n");

    struct bigint num4 = {5, 131071};
    print(num4);
    printf(1, "\n");



	exit();

}
