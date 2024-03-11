#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>
#include <stdlib.h> 

static bool isAlpha(wchar_t c) {
  return (c >= 0x1200 && c <= 0x137F);
}

int main(int argc, const char *argv[]) {
    setlocale(LC_ALL, "");  // set the locale to the user's default locale

    if (argc == 1) {
        printf("No argument provided\n");
        return 1;
    }

    printf("File: %s\n", argv[1]);
    
    wchar_t ena[100];
    mbstowcs(ena, argv[1], 100);  // convert the input string to wide characters

    if (isAlpha(ena[0])) {
        printf("Unicode representation: %#x\n", ena[0]);
        printf("Amharic character\n");
    } else {
        printf("Not an Amharic character\n");
    }
    
    return 0;
}