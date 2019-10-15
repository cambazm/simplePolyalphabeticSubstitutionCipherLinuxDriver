/************************************
 * SYSTEMS PROGRAMMING ASSIGNMENT 2 *
 * STUDENT: MEHMET CAMBAZ	    *
 * NUMBER:  040020365		    *
 ************************************/

/*  definitions for the char module */

#ifndef VIGENERE_H
#define VIGENERE_H

#include <linux/ioctl.h>

#ifndef VIGENERE_MAJOR_NUMBER
#define VIGENERE_MAJOR_NUMBER 0   /* dynamic major by default */
#endif

/* Use 'M' as magic number */
#define VIGENERE_MAGIC_NUMBER 'M'

#define VIGENERE_MODE_DECRYPT _IOR(VIGENERE_MAGIC_NUMBER, 0 , char*)
#define VIGENERE_MODE_SIMPLE _IOR(VIGENERE_MAGIC_NUMBER, 1 , char*)

#endif
