

#include <stdio.h>
#include <stdlib.h>

#include "loader.h"

char *load_file(Allocator allocator, const char *path) {
    FILE *file = fopen("input.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 0;
    }

    // Find the file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    if (size < 0) {
        perror("Error determining file size");
        fclose(file);
        return NULL;
    }

    char *content = allocator_alloc(allocator, size + 1);
    if (content == NULL) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(content, 1, (size_t)size, file);
    content[bytes_read] = '\0';

    fclose(file);

    return content;
}
