/* error.h
 *
 * Error message codes
 *
 * 1995	K.W.E. de Lange
 */
#ifndef _ERROR_
#define _ERROR_

/* Error numbers are used as program return code, so error numbering starts
 * at 1 because convention is that program return code 0 indicates no error
 */
#define NameError 1				/* name already in use or unknown */
#define TypeError 2				/* unsupported operation for type */
#define SyntaxError 3			/* programmers not abiding the languages syntax */
#define ValueError 4			/* invalid value or conversion impossible */
#define SystemError 5			/* low level problem like file open or read */
#define IndexError 6			/* sequence index out of range */
#define OutOfMemoryError 7		/* malloc or calloc failed */
#define ModNotAllowedError 8	/* using mod on anything other then an integer */
#define DivisionByZeroError 9	/* something / 0 */
#define DesignError 10			/* error in the design of EXIN, my bad ;) */

extern void raise(const int number, ...);

#endif
