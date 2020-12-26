/* util.c */

#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/** Delimiters between fields */
const char * FieldDelimiters = " \t";

/** Delimiters between lines */
const char * LineDelimiters = "\n\r";

void manageError(char * msg)
{
    fprintf( stderr, "\n%s ERROR: %s\n", getAppName(), msg );
    exit( EXIT_FAILURE );
}

void manageWarning(char * msg)
{
    fprintf( stderr, "\n%s WARNING: %s\n", getAppName(), msg );
}


void *my_realloc(void *buf, size_t size)
{
    buf = realloc( buf, size );

    if ( buf == NULL ) {
        manageError( "my_realloc(): not enough memory");
    }

    return buf;
}

void *my_malloc(size_t size)
{
    void * buf = malloc( size );

    if ( buf == NULL ) {
        manageError( "my_malloc(): not enough memory" );
    }

    memset( buf, 0, size );

    return buf;
}

void *my_strdup(const char * str)
{
    char * toret = strdup( str );

    if ( toret == NULL ) {
        manageError( "my_strdup(): not enough memory" );
    }

    return toret;
}

char *getShortFileName(const char * fileName)
{
    const unsigned int len = strlen( fileName );
    const char * ptr = fileName + len - 1;
    char * toret = NULL;
    const char * dotPos = NULL;
    const char * slashPos = NULL;
    int neededLen = 0;

    /* Look for directory mark & extension mark */
    while( ptr >= fileName ) {
        if ( *ptr == '/'
          || *ptr == '\\' )
        {
            slashPos = ptr;
            break;
        }
        else
        if ( *ptr == '.' ) {
            dotPos = ptr;
        }

        --ptr;
    }

    /* Adapt marks if missing */
    if ( dotPos <= slashPos ) {
        dotPos = NULL;
    }

    if ( slashPos == NULL ) {
        slashPos = fileName - 1;
    }

    if ( dotPos == NULL ) {
        dotPos = fileName + len;
    }

    /* Create new string (if needed) */
    neededLen = dotPos - slashPos - 1;
    toret = (char *) malloc( neededLen + 1 );

    memset( toret, 0, neededLen + 1 );
    memcpy( toret, slashPos + 1, neededLen );

    return toret;
}

char *changeFileNameExt(char * fileName, const char * ext)
{
    const unsigned int fileNameLen = strlen( fileName );
    const unsigned int extLen = strlen( ext );
    const unsigned int maxLen = fileNameLen + extLen;
    char * toret = (char *) my_malloc( maxLen );
    char * ptr = fileName + fileNameLen - 1;
    char * dotPos = NULL;

    /* Look for dot */
    while( ptr >= fileName ) {
        if ( *ptr == '.' ) {
            dotPos = ptr;
            break;
        }
        --ptr;
    }

    if ( dotPos == NULL ) {
        dotPos = fileName + fileNameLen;
        memcpy( toret, fileName, dotPos - fileName + 1 );
        strcat( toret, "." );
    } else {
        /* Copy until dot */
        memcpy( toret, fileName, dotPos - fileName + 1 );
        *( toret + ( dotPos - fileName ) + 1 ) = 0;
    }

    if ( *ext == '.' ) {
            ++ext;
    }

    /* Append extension */
    strcat( toret, ext );

    return toret;
}

char * getFileNameExt(const char * fileName)
{
    const unsigned int fileNameLen = strlen( fileName );
    unsigned int extLen;
    char * toret = (char *) my_malloc( fileNameLen );
    const char * ptr = fileName + fileNameLen - 1;
    const char * dotPos = NULL;

    /* Look for dot */
    while( ptr >= fileName ) {
        if ( *ptr == '.' ) {
            dotPos = ptr;
            break;
        }
        --ptr;
    }

    if ( dotPos == NULL ) {
        *toret = 0;
    } else {
        /* Copy after dot */
        extLen = dotPos - fileName + 2;
        memcpy( toret, dotPos + 1, extLen );
        *( toret + extLen ) = 0;
        strTrim( toret, FieldDelimiters );
    }

    return toret;
}

void skipDelimiters(FILE * f)
{
    int c = fgetc( f );

    while ( ( strchr( FieldDelimiters, c ) != NULL
           || strchr( LineDelimiters, c ) != NULL )
         && c != EOF )
    {
        c = fgetc( f );
    }

    if ( c != EOF ) {
        ungetc( c, f );
    }

    return;
}

char * getPathFromFileName(const char * fileName)
{
    /* Look for directory mark & extension mark */
    const unsigned int fileNameLen = strlen( fileName );
    const char * ptr = fileName + fileNameLen;
    char * toret;
    unsigned int resultingLen;

    while( ptr >= fileName ) {
        if ( *ptr == '/'
        || *ptr == '\\' )
        {
            break;
        }

        --ptr;
    }

    resultingLen = ptr - fileName + 1;
    toret = my_malloc( resultingLen + 1 );
    memcpy( toret, fileName, resultingLen );
    *( toret + resultingLen ) = 0;

    return toret;
}

char * makeCompletePath(const char * path, const char * fileName)
{
    const unsigned int neededLen = strlen( path ) +  strlen( fileName ) + 1;
    char * toret = my_malloc( neededLen );

    strcpy( toret, path );
    strcat( toret, fileName );

    return toret;
}

char * freadLine(FILE * f, char ** buffer, unsigned int * buflen, const char * delimiters)
{
    unsigned int current = 0;
    int c = fgetc( f );

    /* Read in the rest of the line to the buffer */
    while ( c != EOF
         && strchr( delimiters, c ) == NULL
         && c != '\n'
         && c != '\r' )
    {
        if ( current == *buflen )
        {
            *buflen = ( ( *buflen ) + 1 ) * 2;
            *buffer = (char *) my_realloc( *buffer, *buflen );
        }

        (*buffer)[ current++ ] = c;
        c = fgetc( f );
    }

    /* Terminate string */
    (*buffer)[ current ] = 0;

    /* Skip end of line, if needed */
    c = fgetc( f );
    while( c == '\r'
        || c == '\n' )
    {
        c = fgetc( f );
    }
    ungetc( c, f );

    return *buffer;
}

char * freadBlock(FILE * f, size_t offset, size_t length)
{
    char * toret = my_malloc( length );
    size_t readLength;

    fseek( f, offset, SEEK_SET );
    readLength = fread( toret, 1, length, f );

    if ( readLength != length ) {
        manageError( "reading file" );
    }

    return toret;
}

char * strtoupper(char *s)
{
    char *ptr;

    for( ptr = s; *ptr != 0; ++ptr) {
        *ptr = toupper( *ptr );
    }

    return s;
}

char * strtolower(char *s)
{
    char *ptr;

    for( ptr = s; *ptr != 0; ++ptr) {
        *ptr = tolower( *ptr );
    }

    return s;
}

bool isRelativePath(char * fileName)
{
    bool toret = true;

    if ( *fileName == '/'
      || *fileName == '\\' )
    {
        toret = false;
    }
    else
    if ( isalpha( *fileName ) ) {
        char * ptr = fileName + 1;

        if ( *ptr == ':'
          && *(++ptr) == '\\' )
        {
            toret = false;
        }
    }

    return toret;
}

char * strTrim(char *s, const char * delimiters)
{
    unsigned int len = strlen( s );
    char * end = s + len - 1;
    char * beg = s;
    char * ptr;

    // Prepare delimiters
    if ( delimiters == NULL ) {
        delimiters = FieldDelimiters;
    }

    // Beware - the order of the next two loops matters

    // Look for the beginning of the string
    while ( strchr( delimiters, *beg ) != NULL
         && beg <= end )
    {
        ++beg;
    }

    // Look for the end of the string
    while ( strchr( delimiters, *end ) != NULL
         && end >= s )
    {
        --end;
    }

    // Mark the end of the string
    *( ++end ) = 0;

    // Relocate the string at the beginning
    if ( beg != s
      && end > beg )
    {
        for( ptr = s; beg < end; ++beg, ++ptr) {
            *ptr = *beg;
        }
        *ptr = 0;
    }

    return s;
}
