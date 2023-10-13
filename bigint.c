// bigint.c
#include "types.h"
#include "defs.h"
#include "bigint.h"
#define UINT_MAX 0xFFFFFFFF


struct bigint add(struct bigint a, struct bigint b) {
    uint sum_low = (uint) a.low + b.low;
    int carry = sum_low < (uint) a.low;  // If overflow occurs, carry becomes 1
    uint sum_high = (uint) a.high + b.high + carry;

    struct bigint result;
    result.high = sum_high;
    result.low = sum_low;
    return result;
}

struct bigint subtract(struct bigint a, struct bigint b) {
    int diff_low = a.low - b.low;
    int diff_high = a.high - b.high;

    if (diff_low < 0) {
        diff_low += UINT_MAX;  // Adjust the low part
        diff_high -= 1;       // Borrow from the high part
    }

    struct bigint result;
    result.high = diff_high;
    result.low = diff_low;
    return result;
}


int compare(struct bigint a, struct bigint b) {
    if (a.high < b.high) {
        return -1;
    } else if (a.high > b.high) {
        return 1;
    } else {  // a.high == b.high
        if ((uint)a.low < (uint)b.low) {
            return -1;
        } else if ((uint)a.low > (uint)b.low) {
            return 1;
        } else {
            return 0;
        }
    }
}
