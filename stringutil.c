#include "stringutil.h"

char* itoa(int value, char* buffer, int base) {
    if (base < 2 || base > 36) {
        *buffer = '\0';  // Invalid base
        return buffer;
    }
	
    char* ptr = buffer, *ptr1 = buffer, tmp_char;
    int tmp_value;
	
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
    } while ( value );
	
    // Sign for negative numbers
    if (tmp_value < 0)
        *ptr++ = '-';
    *ptr-- = '\0';

    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return buffer;
}


// Pads the source string on the right with spaces to ensure it's at least `width` characters long.
void padstring(char *dest, const char *src, int width) {
    int i, j;
    for(i = 0; src[i] && i < width; i++) {
        dest[i] = src[i];
    }
    for(j = i; j < width; j++) {
        dest[j] = ' ';
    }
    dest[width] = '\0';  // Null terminate the string
}
