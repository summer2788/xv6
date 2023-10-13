#ifndef STRINGUTIL_H
#define STRINGUTIL_H

// Convert integer to string. Supports base 2 to 36.
char* itoa(int value, char* buffer, int base);

// Pads the source string on the right with spaces to ensure it's at least `width` characters long.
void padstring(char *dest, const char *src, int width);

#endif
