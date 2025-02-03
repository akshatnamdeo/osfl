#ifndef SOURCE_LOCATION_H
#define SOURCE_LOCATION_H

/* A tagged definition for SourceLocation so that all modules share the same type. */
typedef struct SourceLocation {
    unsigned int line;
    unsigned int column;
    const char* file;
} SourceLocation;

#endif // SOURCE_LOCATION_H
