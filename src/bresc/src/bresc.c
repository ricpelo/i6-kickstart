/* bresc.c
 * Based on blc v0.5b from L. Ross Raszewski
 * Permission from the original author was granted.
 */

/* Original copyright message follows
 * BLC:  The Blorb Packager .5b by L. Ross Raszewski
 * Copyright 2000 by L. Ross Raszewski, but freely distributable.
 */

#include "util.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

/** Max buffer size for all operations */
#define BufferSize 8192

/** Maximum number of chunks we'll allow in a blob file */
#define MaxChunksBlorb 1024

/** Short string size */
#define ShortStringSize 512

/** Size of Blorb ID Chunks */
#define BlorbIdLen 4

/* Options */
const char * OptNoBli    = "nobli";
const char * OptVersion  = "version";
const char * OptBliOnly  = "blionly";
const char * OptVerbose  = "verbose";
const char * OptShortExt = "shortext";
const char * OptHelp     = "help";

/** Allowed symbols in ID's, apart from letters and digits */
const char * allowedSymbolsInIds = "_-";

/** The program's version message */
const char * Version = "v0.32 Serial 20091218";

/** The program's name */
const char * AppName = "bresc";

/** The default output file's extension */
const char * BlorbExt      = "blb";
const char * BlorbZCodeExt = "zblorb";
const char * BlorbGlulxExt = "gblorb";

/** The default input file's extension */
const char * DefaultInExt = "res";

/** The default extension for bli files */
const char * DefaultBliExt = "bli";

typedef enum _PictureTypes {
    PNG, JPG, PictureTypeError
} PictureTypes;

const char * PictureChunkTypes[] = {
    "PNG",
    "JPEG",
    ""
};

typedef enum _SoundTypes {
    OGG, AIFF, MOD, SoundTypeError
} SoundTypes;

const char * SoundChunkTypes[] = {
    "OGGV",     /* see http://www.vorbis.com/ for specification */
    "AIFF",
    "MOD",      /* ProTracker 2.0 format: 31 note samples, up to 128 note patterns */
    ""
};

typedef enum _ExecutableTypes {
    ZCOD, GLUL, TAD2, TAD3, HUGO, ALAN, ADRI, LEVE,
    AGT, MAGS, ADVS,
    ExecutableTypeError
} ExecutableTypes;

const char * ExecutableChunkTypes[] = {
    "ZCOD",     /* Z virtual machine */
    "GLUL",     /* Glulx virtual machine */
    "TAD2",     /* TADS 2 virtual machine */
    "TAD3",     /* TADS 3 virtual machine */
    "HUGO",     /* HUGO virtual machine */
    "ALAN",     /* Alan virtual machine */
    "ADRI",     /* Adrift virtual machine */
    "LEVE",     /* Level 9 virtual machine */
    "AGT",      /* AGT virtual machine */
    "MAGS",     /* Magnetic Scrolls virtual machine */
    "ADVS",     /* AdvSys virtual machine */
    "EXEC",     /* Native executable */
    ""
};

const char * ChunkUsages[] = {
    "Exec",
    "Pict",
    "Snd",
    "IFmd",
    "Fspc",
    "ERR",
    ""
};

const char * ChunkUsagesAlternateIds[] = {
    " EXEC EXE CODE  ",
    " PICT PIC PICTURE ",
    " SND MSC MUSIC SOUND ",
    " META MTA BIBLIO BIBLIOGRAPHIC BIB IFMD ",
    " POSTER POST COV COVER FRONT FSPC ",
    ""
};

const char * VblePrefixes[] = {
    "exe",
    "pic",
    "snd",
    ""
};

typedef enum _Usages {
        Exec, Pict, Snd, IFmd, Fspc, UsageError
} Usages;

/** The blorb chunk type.
 * In addition to the chunk data, it holds the index information if needed
 */
typedef struct _Chunk {
    char Type[ BlorbIdLen + 1 ];
    char Use[ BlorbIdLen + 1 ];
    unsigned int Res;
    unsigned long Length;
    char *Data;
} Chunk;

typedef struct _status {
    /* Chunk count / use */
    /** Ids for chunk types = Pict */
    unsigned int nextChunkForPicts;
    /** Ids for chunk types = Snd */
    unsigned int nextChunkForSnds;
    /** Ids for chunk types = Exec */
    unsigned int nextChunkForExecs;
    /** Ids for chunk types = IFmd, Fspc... */
    unsigned int nextChunkForMeta;
    /** verbose mode */
    bool verbose;
    /** do not generate bli file */
    bool noBli;
    /** only generate bli file */
    bool onlyBli;
    /* Cover information */
    /** Cover ? */
    bool thereIsCover;
    /** Cover chunk number */
    unsigned coverChunk;
    /** Cover res number */
    unsigned int coverId;
    /** Using short extension (blb) */
    bool isShortExtension;
    /** Bibliographic info ? */
    bool thereIsBib;
    /** The current line in the res control file */
    unsigned int lineNumber;
    /** Array of all chunks */
    Chunk *BlorbChunks[ MaxChunksBlorb ];
    /** Number of chunks in this file */
    int numberOfChunks;
    /** Number of chunks we need to index */
    int numberOfIndexEntries;
    /** Offsets of index entries */
    unsigned long int *indexOffsets;
    /** Program name */
    char * myName;
    /** File path */
    char * path;
    /** Report contents */
    char *report;
    /** Default extension */
    const char * outFileExt;
    /** Is a Glulx story file or not */
    bool isGlulx;
    /** String for message errors */
    char msg[ShortStringSize];
    char * inName;
    char * outName;
    char * bliName;
    FILE * bli;
    FILE * in;
    FILE * out;
} Status;


const char * BrescApp          = "bresc";
const char * BresApp           = "bres";
const char * BlcApp            = "blc";
const char * PngFilesExt       = "png";
const char * JpgFilesExt       = "jpg";
const char * OggFilesExt       = "ogg";
const char * AifFilesExt       = "aif";
const char * ModFilesExt       = "mod";
const char * Z5FilesExt        = "z5";
const char * Z8FilesExt        = "z8";
const char * GlulxFilesExt     = "ulx";
const char * IfictionFilesExt  = "ifiction";
const char * CommentCharacters = ";.!#%&/:\\$->";


/** The program information message string is formatted
 * with the assumption  that it will be printed with the program name,
 * and version string
 * @see Version
 */

const char * InfoMsg = "%s %s\n"
                       "Blorb resource compiler (%s is based on blc .5b by L. Ross Raszewski)\n"
;

void initStatus(char * argv[], Status *stats)
{
    stats->nextChunkForPicts = 3;
    stats->nextChunkForSnds = 3;
    stats->nextChunkForExecs = 0;
    stats->nextChunkForMeta = 0;
    stats->verbose = false;
    stats->noBli = false;
    stats->onlyBli = false;
    stats->isShortExtension = false;
    stats->thereIsCover = stats->thereIsBib = false;
    stats->coverChunk = 0;
    stats->coverId = 0;
    stats->lineNumber = 0;
    stats->numberOfChunks = 0;
    stats->numberOfIndexEntries = 0;
    stats->path = NULL;
    stats->indexOffsets = NULL;
    stats->inName = stats->outName = stats->bliName = NULL;
    stats->bli = stats->in = stats->out = NULL;
    stats->report = NULL;
    stats->outFileExt = BlorbExt;

    stats->myName = getShortFileName( argv[ 0 ] );
    strtolower( stats->myName );
}

/**
 * getVectorPos() - Returns the position of a string inside a vector of strings
 * The vector must be finished with a null string
 * @param s the string containing the possible use
 * @param v the vector of strings containing the possible use
 * @return the corresponding vector position, if found.
 * The position of null string otherwise
 */

int getVectorPos(const char *v[], const char *s)
{
    unsigned int i = 0;

    for(; *v[ i ] != 0; ++i) {
        if ( !strcmp( v[ i ], s ) ) {
            break;
        }
    }

    return i;

}

/**
 * Converts a string to its usage enumrated value
 * @see ChunkUsages
 * @param s the string containing the possible use
 * @return the corresponding usage value, if found. UsageError otherwise
 */
Usages cnvtToUsages(const char *s)
{
    Usages toret = UsageError;

    if ( s != NULL
      && *s != 0 )
    {
        /* Convert the string to spcSTRINGspc, so it can be found */
        const unsigned int idLen = strlen( s );
        const unsigned int neededLen = idLen + 3;
        char * aux = my_malloc( neededLen );

        memset( aux, 0, neededLen );
        *aux = ' ';
        memcpy( aux + 1, s, idLen );
        *( aux + neededLen -2 ) = ' ';
        strtoupper( aux );

        for(toret = (Usages) 0; toret < UsageError; ++toret) {
            if ( strstr( ChunkUsagesAlternateIds[ toret ], aux ) != NULL ) {
                break;
            }
        }

        free( aux );
    }

    return toret;
}

/**
 * Checks that the usage is valid
 * @see ChunkUsages
 * @param s the string containing the possible use
 * @return true if is found. False otherwise
 */
bool chkUse(const char *s, Status * stat)
{
    Usages toret = cnvtToUsages( s );
    *( stat->msg ) = 0;

    if ( toret == UsageError ) {
        sprintf( stat->msg, "%d: Illegal use '%s'", stat->lineNumber, s );
    }

    return ( toret != UsageError );
}

/**
 * Checks that the type is valid
 * @see ChunkTypes
 * @param s the string containing the possible type
 * @param use The use for this type
 * @return true if is found. False otherwise
 */
bool chkType(Usages use, const char *s, Status * stat)
{
    unsigned toret;
    const char ** v = NULL;
    *( stat->msg ) = 0;

    if ( use == Exec ) {
        v = ExecutableChunkTypes;
    }
    else
    if ( use == Pict ) {
        v = PictureChunkTypes;
    }
    else
    if ( use == Snd ) {
        v = SoundChunkTypes;
    }

    if ( v != NULL ) {
        toret = getVectorPos( v, s );

        if ( *v[ toret ] == 0 ) {
            sprintf( stat->msg, "%d: Illegal type '%s' for use '%s'",
                stat->lineNumber,
                ChunkUsages[ use ],
                s
            );
        }
    } else {
        v = ChunkUsages;
        toret = use;
        sprintf( stat->msg, "%d: Illegal use '%s' with type '%s'", stat->lineNumber, v[ use ], s );
    }

    return ( *v[ toret ] != 0 );
}

/**
    isExecUse()
    @param s String containing the possible use
    @return true if use us Exec, false otherwise
*/
inline
bool isExecUse(const char *s)
{
    return ( cnvtToUsages( s ) == Exec );
}

/**
    isPictUse()
    @param s String containing the possible use
    @return true if use us Pict, false otherwise
*/
inline
bool isPictUse(const char *s)
{
    return ( cnvtToUsages( s ) == Pict );
}

/**
    isSndUse()
    @param s String containing the possible use
    @return true if use us Snd, false otherwise
*/
inline
bool isSndUse(const char *s)
{
    return ( cnvtToUsages( s ) == Snd );
}

/** writeInt writes an integer to a file in blorb format
 * @param f file handle
 * @param v number to write
 */
inline
void writeInt(FILE *f, unsigned int v)
{
    unsigned char v1 = v&0xFF;
    unsigned char v2 = (v>>8)&0xFF;
    unsigned char v3 = (v>>16)&0xFF;
    unsigned char v4 = (v>>24)&0xFF;

    fwrite( &v4, 1, 1, f );
    fwrite( &v3, 1, 1, f );
    fwrite( &v2, 1 ,1, f );
    fwrite( &v1, 1 ,1 ,f );
}

/** strLong writes a long to a string, in a format suitable to later
 * using with write_id
 * @param f The string to write the number to
 * @param v The number to write
 */
inline
void strLong(char *f, unsigned int v)
{
    unsigned char v1 = v&0xFF;
    unsigned char v2 = (v>>8)&0xFF;
    unsigned char v3 = (v>>16)&0xFF;
    unsigned char v4 = (v>>24)&0xFF;

    f[ 0 ] = v4;
    f[ 1 ] = v3;
    f[ 2 ] = v2;
    f[ 3 ] = v1;
}

/** strShort writes a short to a string, in a format suitable to later
 * using with write_id
 * @param f The string to write the number to
 * @param v The number to write
 */
inline
void strShort(char *f, unsigned int v)
{
    unsigned char v1 = v&0xFF;
    unsigned char v2 = (v>>8)&0xFF;

    f[ 0 ] = v2;
    f[ 1 ] = v1;
}

/** write_id writes a string to a file as a blorb ID string (4 bytes, space padded)
 * @param f The file handle
 * @param s The string
 */
void writeId(FILE *f, char *s)
{
    int i;
    static const char sp = ' ';

    for (i = 0; i < strlen( s ); i++) {
        fwrite( &s[ i ], 1, 1, f );
    }

    for (; i < BlorbIdLen; i++) {
        fwrite( &sp, 1, 1, f );
    }
}

/**
 * strId writes a blorb identifier to a string
 * @param f The string
 * @param s The blorb identifier
*/
void strId(char *f, char *s)
{
    unsigned int i;
    static const char sp = ' ';
    unsigned int lenId = strlen( s );

    if ( lenId > BlorbIdLen ) {
        lenId = BlorbIdLen;
    }

    for ( i=0; i < lenId; i++) {
        f[ i ] = s[ i ];
    }
    for (; i < BlorbIdLen; i++) {
        f[ i ] = sp;
    }
}

/**
 * copyId copies a blorb identifier (only 4 chars) to a string
 * @param dest The destination string
 * @param s The blorb identifier to copy
*/
void copyId(char *dest, const char *org)
{
    unsigned int i;
    unsigned int lenId = strlen( org );

    if ( lenId > BlorbIdLen ) {
        lenId = BlorbIdLen;
    }

    for (i = 0; i < lenId; i++) {
        dest[ i ] = org[ i ];
    }
    dest[ i ] = 0;
}

/**
 * inferType - fills the chunk->Type field. Also sets status->isGlulx
 * @param chunk The chunk data to fill in with the type
 * @param fileName
 * @see Chunk
 */

void inferType(Chunk * chunk, const char * fileName, Status * status)
{
    char * ext = getFileNameExt( fileName );

    strtolower( ext );

    if ( !strcmp( ext, JpgFilesExt ) ) {
        copyId( chunk->Type, PictureChunkTypes[ JPG ] );
    }
    else
    if ( !strcmp( ext, PngFilesExt ) ) {
        copyId( chunk->Type, PictureChunkTypes[ PNG ] );
    }
    else
    if ( !strcmp( ext, OggFilesExt ) ) {
        copyId( chunk->Type, SoundChunkTypes[ OGG ] );
    }
    else
    if ( !strcmp( ext, ModFilesExt ) ) {
        copyId( chunk->Type, SoundChunkTypes[ MOD ] );
    }
    else
    if ( !strcmp( ext, AifFilesExt ) ) {
        copyId( chunk->Type, SoundChunkTypes[ AIFF ] );
    }
    else
    if ( !strcmp( ext, Z5FilesExt ) ) {
        copyId( chunk->Type, ExecutableChunkTypes[ ZCOD ] );
        status->isGlulx = false;
    }
    else
    if ( !strcmp( ext, Z8FilesExt ) ) {
        copyId( chunk->Type, ExecutableChunkTypes[ ZCOD ] );
        status->isGlulx = false;
    }
    else
    if ( !strcmp( ext, GlulxFilesExt ) ) {
        copyId( chunk->Type, ExecutableChunkTypes[ GLUL ] );
        status->isGlulx = true;
    }
    else
    if ( !strcmp( ext, IfictionFilesExt ) ) {
        copyId( chunk->Type, ChunkUsages[ IFmd ]  );
    }
    else {
        sprintf( status->msg, "%d: unrecognized file extension '%s' in '%s'\n",
                 status->lineNumber, ext, fileName
        );
        manageError( status->msg );
    }


    free( ext );
    return;
}

char * createNameFromFile(Usages use, const char * fileName)
{
    const unsigned int lenFileName = strlen( fileName );
    char * toret = (char *) my_malloc( lenFileName * 2 );
    char * shortName = getShortFileName( fileName );

    shortName[ 0 ] = toupper( shortName[ 0 ] );

    strcpy( toret, VblePrefixes[ use ] );
    strcat( toret, shortName );

    free( shortName );
    return toret;
}

void writeBliEntry(FILE * bli, Usages use, unsigned int res, const char * id, const char * fileName)
{
    char * vbleName;
    char * shortFileName;
    char * fileNameExt;

    if ( bli != NULL
      && ( use == Pict
        || use == Snd ) )
    {
        if ( id == NULL
          || *id == 0 )
        {
             vbleName = createNameFromFile( use, fileName );
        }
        else vbleName = my_strdup( id );

        shortFileName = getShortFileName( fileName );
        fileNameExt = getFileNameExt( fileName );

        fprintf( bli, "Constant %s %d;\t! %s: '%s.%s'\n",
                            vbleName, res, ChunkUsages[ use ],
                            shortFileName, fileNameExt
        );

        free( shortFileName );
        free( fileNameExt );
        free( vbleName );
    }

    return;
}

void writeBliHeader(Status * status)
{
    if ( status->bli != NULL ) {
        time_t dateTime  = time( NULL );
        struct tm * date = localtime( &dateTime );
        char strDate[BufferSize];

        sprintf( strDate, "%02d/%02d/%04d %02d:%02d:%02d",
                    date->tm_mday, date->tm_mon + 1, date->tm_year + 1900,
                    date->tm_hour, date->tm_min, date->tm_sec
        );

        /* Write the bli header */
        fprintf( status->bli, "! Resources include file for Inform\n"
                      "! Generated by %s (%s) %s on %s\n\n",
                        status->myName, AppName, Version, strDate
        );

        fprintf( status->bli,
                 "message \"Including resources file by %s, on %s\";\n\n",
                 AppName, strDate
        );
    }
}

unsigned int assignResNumber(Usages use, Status * stat)
{
    unsigned int toret = UsageError;

    if ( use == Exec ) {
        toret = ( stat->nextChunkForExecs )++;
    }
    else
    if ( use == Pict ) {
        toret = ( stat->nextChunkForPicts )++;
    }
    else
    if ( use == Snd ) {
        toret = ( stat->nextChunkForSnds )++;
    }
    else
    if ( use == IFmd ) {
        toret = ( stat->nextChunkForMeta )++;
    }

    return toret;
}

char * prepareFileName(char ** buffer, Status * status)
{
    char * toret;

    strTrim( *buffer, FieldDelimiters );

    if ( isRelativePath( *buffer ) ) {
        toret = makeCompletePath( status->path, *buffer );
        free( *buffer );
        *buffer = NULL;
    }
    else toret = *buffer;

    return toret;
}

bool isId(const char *id)
{
    bool toret = false;
    const char * ptr = id;

    if ( isalpha( *ptr )
     ||  strchr( allowedSymbolsInIds, *ptr ) != NULL )
    {
        for(++ptr; *ptr != 0; ++ptr) {
            if ( !isalpha( *ptr )
              && !isdigit( *ptr )
              && strchr( allowedSymbolsInIds, *ptr ) == NULL )
            {
                break;
            }
        }

        toret = ( *ptr == 0 );
    }

    return toret;
}

/**
    describes a chunk in msg, as string
    @param chunk The chunk to describe
    @return a pointer to msg
*/
char * describeChunk(Chunk *chunk, Status * status, bool complete)
{
    if ( complete ) {
        sprintf( status->msg,
                 "id#%04d: Use '%s'\tType '%s'\tLength: '%lu'",
                 chunk->Res,
                 chunk->Use,
                 chunk->Type,
                 chunk->Length
        );
    } else {
        sprintf( status->msg,
                 "id#%04d: Use '%s'\tType '%s'",
                 chunk->Res,
                 chunk->Use,
                 chunk->Type
        );
    }

    return status->msg;
}

/**
 * readChunk reads one entry from a res control file and loads a chunk from it
 * It does also write each entry in the bli file
 * @see Chunk
 * @param f The file handle
 * @param bli the file handle of the bli file
*/
Chunk * readChunk(Status * status)
{
    unsigned int buflen = ShortStringSize;
    char * fileName     = NULL;
    Usages use          = UsageError;
    char * id           = NULL;
    int c               = EOF;
    FILE * f            = status->in;

    /* Malloc a new chunk */
    Chunk * toret = (Chunk *) my_malloc( sizeof( Chunk ) );
    char * buffer = (char *) my_malloc( buflen );

    /* Read in the res file line */
    /* skip commented lines */
    skipDelimiters( f );
    c = fgetc( f );
    while ( strchr( CommentCharacters, c ) != NULL ) {
        freadLine( f, &buffer, &buflen, LineDelimiters );
        skipDelimiters( f );
        c = fgetc( f );
    }

    /* End of file? */
    if ( c == EOF ) {
        free( buffer );
        free( toret );
        toret = NULL;
        goto End;
    }

    /* Use */
    ungetc( c, f );
    freadLine( f, &buffer, &buflen, FieldDelimiters );
    use = cnvtToUsages( buffer );
    copyId( toret->Use, ChunkUsages[ use ] );

    /* Chk use */
    if ( !chkUse( toret->Use, status ) ) {
        manageError( status->msg );
    }

    /* Convert use, if needed */
    if ( use == IFmd ) {
        if ( !status->thereIsBib ) {
            status->thereIsBib = true;
            strcpy( toret->Type, toret->Use );
            strcpy( toret->Use, "0" );
        } else manageError( "duplicated bibliographic info" );
    }
    else
    if ( use == Fspc ) {
        if ( !status->thereIsCover ) {
            status->thereIsCover = true;
            status->coverChunk = status->numberOfChunks;
            strcpy( toret->Use, ChunkUsages[ Pict ] );
            use = Pict;
        } else manageError( "duplicated cover" );
    }

     /* Assign id */
    toret->Res = assignResNumber( use, status );

    /* Abort if anything is wrong */
    skipDelimiters( f );
    if ( feof( f ) ) {
        manageError( "unexpected end of file" );
    }

    /* Read the rest of the line, if needed */
    freadLine( f, &buffer, &buflen, FieldDelimiters );
    if ( isId( buffer ) ) {
        id = buffer;
        buffer = NULL;
        buflen = 0;
        skipDelimiters( f );
        freadLine( f, &buffer, &buflen, LineDelimiters );
        strTrim( buffer, FieldDelimiters );
    }

    /* get file name */
    fileName = prepareFileName( &buffer, status );

    /* set the type of the chunk */
    inferType( toret, fileName, status );
    chkType( use, toret->Type, status );

    /* Write the .bli entry, provided it is not the cover */
    if ( status->thereIsCover
      && status->numberOfChunks == status->coverChunk )
    {
        status->coverId = toret->Res;
    }
    else {
        writeBliEntry( status->bli, use, toret->Res, id, fileName );
    }

    /* open file name */
    FILE * in = fopen( fileName, "rb" );

    if ( in == NULL ) {
        sprintf( status->msg, "%d: can't open file '%s'\n", status->lineNumber, fileName );
        free( fileName );
        free( toret );
        toret = NULL;
        fileName = NULL;

        // Manage error or warning
        if ( status->onlyBli ) {
            if ( status->verbose ) {
                manageWarning( status->msg );
            }
        } else {
            manageError( status->msg );
        }

        // Jump to the end (next entry)
        goto End;
    }


    /* read in file contents */
    fseek( in, 0, SEEK_END );
    toret->Length = ftell( in );
    toret->Data = freadBlock( in, 0, toret->Length );
    fclose( in );

    End:
    free( fileName );
    return toret;
}

void initReport(Status * status)
{
    status->report = (char *) my_malloc( ShortStringSize * status->numberOfChunks );
    *( status->report ) = 0;
}

/**
 * buildIndex builds the index chunk for a blorb file, loading all other chunks.
 * It does also write the .bli file
 * @param blorb file handle
 */
void buildIndex(Status * status)
{
    int i;
    int n = 0;
    char *dp;
    Chunk * chunk = NULL;

    /* Start bli file */
    writeBliHeader( status );

    /* Prepare index chunk */
    status->BlorbChunks[ 0 ]= (Chunk *) my_malloc( sizeof( Chunk ) );
    strcpy( status->BlorbChunks[ 0 ]->Type, "RIdx");
    strcpy( status->BlorbChunks[ 0 ]->Use,  "0" );
    status->numberOfChunks = 1;

    /* Load all the chunks */
    skipDelimiters( status->in );

    if ( !feof( status->in ) ) {
        do {
            chunk = readChunk( status );
            if ( chunk != NULL ) {
                status->lineNumber++;

                /* Find out how many resources there are */
                if ( strcmp( chunk->Use, "0" ) ) {
                    n++;
                }

                status->BlorbChunks[ status->numberOfChunks ] = chunk;
                ++( status->numberOfChunks );
                skipDelimiters( status->in );
            }
        } while( !feof( status->in ) );
    }

    /* Is there a cover? Prepare cover chunk */
    if ( status->thereIsCover ) {
        char * buffer = my_malloc( BlorbIdLen );

        /* Create new chunk */
        Chunk * cover = status->BlorbChunks[ status->numberOfChunks ] = my_malloc( sizeof( Chunk ) );

        strcpy( cover->Use, "0" );
        strcpy( cover->Type, ChunkUsages[ Fspc ] );
        cover->Res = ++( status->nextChunkForMeta );

        strLong( buffer, status->coverId );
        cover->Data = buffer;
        cover->Length = 4;
        ++( status->numberOfChunks );
    }

    /* Write the length of the resource index chunk, and allocate its data space */
    status->BlorbChunks[ 0 ]->Length = ( 12 * n ) + 4;
    status->BlorbChunks[ 0 ]->Data=( char *) my_malloc( status->BlorbChunks[ 0 ]->Length );
    status->indexOffsets = (unsigned long int *) my_malloc( n * sizeof( unsigned long int ) );

    /* The first thing in the data chunk is the number of entries */
    strLong( status->BlorbChunks[ 0 ]->Data, n );

    /* Prepare the report */
    if ( status->verbose ) {
        initReport( status );
    }

    /* Now, scroll through the chunks, noting each one in the index chunk */
    dp = status->BlorbChunks[ 0 ]->Data + 4;
    for(i = 1; i < status->numberOfChunks; i++) {
        if ( status->verbose ) {
            strcat( status->report, "\t\t" );
            strcat( status->report, describeChunk( status->BlorbChunks[ i ], status, true ) );
            strcat( status->report, "\n" );
        }

        if ( strcmp( status->BlorbChunks[ i ]->Use, "0" ) != 0 )
        {
            strId( dp, status->BlorbChunks[ i ]->Use );
            dp += 4;
            strLong( dp, status->BlorbChunks[ i ]->Res );
            dp += 4;
            status->indexOffsets[ status->numberOfIndexEntries++ ] =
                    ( dp -( status->BlorbChunks[0]->Data ) ) + 20
            ;
            dp += 4;
        }
    }
}

/**
 * writeChunk writes one chunk to a file
 */
void writeChunk(FILE * out, Chunk * chunk)
/* AIFF files are themselves chunks, so we just write their data, not
 * their other info.
 */
{
    int z = 0;

    if ( strcmp( chunk->Type, "FORM" ) ) {
         writeId ( out, chunk->Type );
         writeInt( out, chunk->Length );
    }

    fwrite( chunk->Data, 1, chunk->Length, out);

    /* Pad chunks of odd length */
    if ( chunk->Length % 2 ) {
        fwrite( &z, 1, 1, out);
    }
}

/** generateBlorb generates a blorb from a res file. Requires the index to be already built
 * @see buildIndex
 * @param in res file handle
 * @param out destination file handle
 */
void generateBlorb(Status * status)
{
    long int n = 0;
    int i;

    /* Prepare report, if needed */
    if ( status->verbose
      && status->report == NULL )
    {
        initReport( status );
    }

    /* Write the IFF header */
    writeId( status->out, "FORM" );
    writeId( status->out, "latr" );  /* We'll find this out at the end */
    writeId( status->out, "IFRS" );

    /* The index should be already built */
    for(i = 0; i < status->numberOfChunks; i++) {
        /* Prepare report */
        if ( status->verbose ) {
            char aux[ShortStringSize];
            sprintf( aux, "\t\tChunk %04d(%s)\twritten.\n",
                     i + 1,
                     describeChunk( status->BlorbChunks[ i ], status, false )
            );
            strcat( status->report, aux );
        }

        /* Write the index entries into the index chunk */
        if ( strcmp( status->BlorbChunks[ i ]->Use, "0" ) ) {
            /* Prepare the offsets */
            long int t = ftell( status->out );
            fseek( status->out, status->indexOffsets[ n++ ], SEEK_SET );
            writeInt( status->out, t );
            fseek( status->out, t, SEEK_SET);
        }

        /* Write the chunk to the file */
        writeChunk( status->out, status->BlorbChunks[ i ] );
    }

    /* Size of the data section of the blorb file */
    n = ftell( status->out ) - 8;
    fseek( status->out, 4, SEEK_SET );

    /* Write that to the file */
    writeInt( status->out, n );
}

void changeOutputFileExtension(Status * status)
{
    if ( !status->isShortExtension ) {
        if ( status->thereIsBib ) {
            if ( status->isGlulx )
                    status->outFileExt = BlorbGlulxExt;
            else    status->outFileExt = BlorbZCodeExt;
        }
        else
        if ( !status->isGlulx ) {
            status->outFileExt = BlorbZCodeExt;
        }
    }

    status->outName = changeFileNameExt( status->inName, status->outFileExt );
}

void cleanMemory(Status * status)
{
    int i;

    /* Clean memory */
    free( status->indexOffsets );
    status->indexOffsets = NULL;
    status->myName = NULL;
    status->path = NULL;

    for(i = 0; i < status->numberOfChunks; i++) {
        if ( status->BlorbChunks[ i ] != NULL ) {
            free( status->BlorbChunks[ i ]->Data );
            status->BlorbChunks[ i ]->Data = NULL;
            free( status->BlorbChunks[ i ] );
            status->BlorbChunks[ i ] = NULL;
        }
    }

    status->numberOfChunks = 0;

    free( status->outName );
    free( status->inName );
    free( status->bliName );
    free( status->report );
    status->outName = status->inName = status->report = status->bliName = NULL;

    /* Close files */
    if ( status->in != NULL ) {
        fclose( status->in );
    }

    if ( status->out != NULL ) {
        fclose( status->out );
    }

    if ( status->bli != NULL ) {
        fclose( status->bli );
    }
}

void strUsage(Status * status)
{
    sprintf( status->msg, "Usage is :\n"
                    "\t%s [options] in-file\n\n\tOptions:\n"
                    "\t\t--%s     \tShows this help and ends.\n"
                    "\t\t--%s\tShows version and ends.\n"
                    "\t\t--%s  \tPrevents .bli file of being generated.\n"
                    "\t\t--%s\tIt does only generate the .bli file, no blorb.\n"
                    "\t\t--%s\tIt does only generate files with .blb extension.\n"
                    ,
                    status->myName,
                    OptHelp, OptVersion, OptNoBli, OptBliOnly, OptShortExt
    );
}

unsigned int processOptions(char *argv[], int * argc, Status *status, bool *end)
{
    unsigned int numOp = 1;
    char * op;
    char * ptr;

    for(; numOp < (*argc); ++numOp ) {
        op = my_strdup( argv[ numOp ] );
        strtolower( op );
        ptr = op;

        /* Pass all '-' */
        while ( *ptr == '-' ) {
            ++ptr;
        }

        /* Was any '-' present? */
        if ( ptr == op ) {
            free( op );
            break;
        }

        /* Check options */
        if ( !strcmp( ptr, OptNoBli ) ) {
            status->noBli = true;
            --(*argc);
        }
        else
        if ( !strcmp( ptr, OptVersion ) ) {
            printf( "\n\n" );

            *end = true;
            --(*argc);
        }
        else
        if ( !strcmp( ptr, OptHelp ) ) {
            strUsage( status );
            printf( "\n%s\n", status->msg );

            *end = true;
            --(*argc);
        }
        else
        if ( !strcmp( ptr, OptBliOnly ) ) {
            status->onlyBli = true;
            --(*argc);
        }
        else
        if ( !strcmp( ptr, OptVerbose ) ) {
            status->verbose = true;
            --(*argc);
        }
        else
        if ( !strcmp( ptr, OptShortExt ) ) {
            status->isShortExtension = true;
            --(*argc);
        }
        else {
            sprintf( status->msg, "invalid option: '%s'", ptr );
            manageError( status->msg );
        }


        free( op );
        op  = NULL;
        ptr = NULL;
    }

    return numOp;
}

inline
const char * getAppName()
{
    return AppName;
}

inline
void decideApp(Status * status)
{
    if ( !strcmp( status->myName, BresApp ) ) {
        status->onlyBli = true;
        status->noBli = false;
    }
    else
    if ( !strcmp( status->myName, BlcApp ) ) {
        status->onlyBli = false;
        status->noBli = true;
        status->isShortExtension = true;
    }
    else
    if ( strcmp( status->myName, BrescApp ) ) {
        manageError( "Unsupported functionality" );
    }
}

int main(int argc, char **argv)
{
    bool finish = false;
    unsigned int numOp = 1;
    Status status;

    /* Init vbles */
    initStatus( argv, &status );

    /* Welcome */
    printf( InfoMsg, status.myName, Version, AppName );

    /* Process options.
       "-version" and "-help" mean to finish the program immediately */
    numOp = processOptions( argv, &argc, &status, &finish );

    if ( finish ) {
        goto End;
    }

    decideApp( &status );

    /* Print error usage */
    if ( argc < 2 )
    {
        strUsage( &status );
        manageError( status.msg );
    }
    else
    /* 1 argument: use input res file as reference for output file */
    if ( argc == 2 )
    {
        char * inName2;

        status.inName = my_strdup( argv[ numOp ] );
        inName2 = changeFileNameExt( status.inName, DefaultInExt );
        free( status.inName );
        status.inName = inName2;
    }
    /* two arguments: input: res file output: user-specified file */
    else
    {
        char * name  = my_strdup( argv[ numOp ] );

        status.inName  = changeFileNameExt( name, DefaultInExt );
        status.outName = my_strdup( argv[ numOp + 1 ] );
    }

    /* Show status */
    if ( status.verbose ) {
        printf( "\nCreate .bli file: %s\tGenerate .blorb: %s\tShort ext.: %s\n",
                    ( !status.noBli ) ? "Yes" : "No",
                    ( !status.onlyBli ) ? "Yes" : "No",
                    ( status.isShortExtension ) ? "Yes" : "No"
        );
    }

    /* Open entry file */
    if ( status.verbose ) {
        printf( "\nOpening files..." );
    }

    status.path = getPathFromFileName( status.inName );
    status.bliName = changeFileNameExt( status.inName, DefaultBliExt );
    status.in  = fopen( status.inName,  "rt" );

    if ( status.in == NULL ) {
        sprintf( status.msg,
                 "(before compilation): can't open Blorb Resources Control File:\n'%s'\n",
                 status.inName
        );
        manageError( status.msg );
    }

    if ( !status.noBli ) {
        status.bli = fopen( status.bliName, "wt" );
        if ( status.bli == NULL ) {
            sprintf( status.msg,
                 "(before compilation): can't open Blorb Resources Control File:\n'%s'\n",
                 status.inName
            );
            manageError( status.msg );
        }
    }

    /* Read the .res file and build the index */
    printf( "\nProcessing '%s'...\n", status.inName );
    buildIndex( &status );
    if ( status.verbose ) {
        printf( "\n\tIndex built...\n" );
        printf( "%s\n", status.report );
        *( status.report ) = 0;
    }


    /* Generate blorb */
    if ( !status.onlyBli ) {
        /* Open output blb file */
        changeOutputFileExtension( &status );
        status.out = fopen( status.outName, "wb" );
        if ( status.out == NULL ) {
            sprintf( status.msg, "(before compilation): can't open Blorb Output File:\n'%s'\n",
                    status.outName
            );
            manageError( status.msg );
        }

        /* do it */
        generateBlorb( &status );

        if ( status.verbose ) {
            printf( "\tChunks written...\n" );
            printf( "%s\n", status.report );
        }
    } else {
        free( status.outName );
        status.outName = status.bliName;
        status.bliName = NULL;
    }

    printf( "End ('%s').\n", status.outName );

    End:
    cleanMemory( &status );
    return EXIT_SUCCESS;
}
