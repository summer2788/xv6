// bigint.h
#ifndef BIGINT_H
#define BIGINT_H

struct bigint {
    int high;
    int low;
};

struct bigint add(struct bigint a, struct bigint b);
struct bigint subtract(struct bigint a, struct bigint b);
int compare(struct bigint a, struct bigint b);

#endif // BIGINT_H

