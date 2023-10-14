// bigint.h

#ifndef _BIGINT_H_
#define _BIGINT_H_

struct bigint {
    int high;  // Most significant 7 digits
    int low;   // Least significant 7 digits
};

struct bigint add(struct bigint a, struct bigint b);
struct bigint subtract(struct bigint a, struct bigint b);
int compare(struct bigint A, struct bigint B);

#endif // _BIGINT_H_

