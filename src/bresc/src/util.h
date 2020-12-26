/* util.h */

#include <stdio.h>
#include <stdbool.h>

/** Delimiters between fields */
extern const char * FieldDelimiters;

/** Delimiters between lines */
extern const char * LineDelimiters;

extern const char * getAppName();

/**
    my_realloc() - reallocates memory. calls manageError if there is not enough memory
    @see manageError
    @param buf Original memory to reallocate
    @param size New size of memory
    @return the new memory
*/

void *my_realloc(void *buf, size_t size);

/**
    my_malloc() - allocates memory. calls manageError if there is not enough memory
    @see manageError
    @param size New size of memory
    @return the new memory
*/

void *my_malloc(size_t size);

/**
    my_strdup() - allocates memory for a given string. calls manageError if there is not enough memory
    @see manageError
    @param str string to copy
    @return the new memory
*/

void *my_strdup(const char * str);

/**
    manageError() - shows a message on error stream and exits program
    @param msg The message to show on the error stream
    @brief prints the message and exits program with -1 code.
*/
void manageError(char * msg);

/**
    manageWarning() - shows a warning message on error stream
    @param msg The message to show on the error stream
    @brief prints the message and exits program with -1 code.
*/
void manageWarning(char * msg);


/**
  getShortFileName() - (strips directory and extension from file name)
  @return simple file name (must be freed)
  @param fileName the file name as string
*/

char *getShortFileName(const char * fileName);

/**
  changeFileNameExt() - (changes extension from file name).
  Returns a new string which should be free'd.
  @return a new file name (must be freed)
  @param fileName the file name as string
  @param ext a string with the new extension
*/

char *changeFileNameExt(char *fileName, const char *ext);

/**
  getFileNameExt() - (gets extension from file name).
  Returns a new string which should be free'd.
  @return a new extension, of the file name (must be freed)
  @param fileName the file name as string
*/

char *getFileNameExt(const char *fileName);

/**
  skipDelimiters() - (skips '\n', ' ', and '\t').
  @param f File name handle
*/

void skipDelimiters(FILE * f);

/**
  makeCompletePath() - returns a new string with the path and the file name concat.
  @param fileName The file name as string
  @param path The path as string
  @return A new string with the path and the filename concatenated.
          Should be free'd.
*/

char * makeCompletePath(const char * path, const char * fileName);

/**
  getPathFromFileName() - returns a new string with the path from that file name.
  @param fileName The file name as string
  @return A new string with the path extracted. Should be free'd.
          A final slash is always present, unless there is no path.
*/

char * getPathFromFileName(const char * fileName);


/**
    freadline() - reads the rest of the input line of the file f
    @param f input file
    @param buffer A pointer to a buffer in memory. It is updated as needed.
                  It can be NULL, buit buflen should be Zero in that case
    @param buflen The length of the buffer. It is updated as needed.
    @param delimiters The characters that will make the function to stop reading
    @return the buffer parameter is returned

*/
char * freadLine(FILE * f, char ** buffer, unsigned int * buflen, const char * delimiters);

/**
    freadBlock() - reads a binary block from file f
    @param f input file
    @param offset The offset ot the file to read from
    @param length Number of bytes to read
    @return memoeyr buffer (to be free'd) with the contents of the file

*/
char * freadBlock(FILE * f, size_t offset, size_t length);

/**
 * strtoupper() converts a string to uppercase
 * @param s The string to convert. Must be writable.
 * @return A pointer to s.
 */
char * strtoupper(char *s);

/**
 * strtolower() converts a string to lowercase
 * @param s The string to convert. Must be writable.
 * @return A pointer to s.
 */
char * strtolower(char *s);

/**
 * strTrim() converts a string, removing leading and trailing spaces
 * @param s The string to convert. Must be writable.
 * @param delimiters The delimiters (tabs, spaces...) to look for.
 *                   If it is NULL, then FieldDelimiters is used.
 * @return A pointer to s.
 */
char * strTrim(char *s, const char * delimiters);

/**
 * isRelativePath() Decides whether a path is relative or not.
 * @param fileName The file name of which decide its path is relative or not
 * @return True if the path is relative; false otherwise
 */
bool isRelativePath(char * fileName);
