/************************************
 * SYSTEMS PROGRAMMING ASSIGNMENT 2 *
 * STUDENT: MEHMET CAMBAZ	    *
 * NUMBER:  040020365		    *
 ************************************/

/* test file for vigenere character device */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "040020365_vigenere.h"

int main(void)
{
	int i;
	int fd = open("/dev/vigenere" , O_RDWR);
	char * buffer = (char*)malloc(19);

	int n = write(fd , "SYSTEMSPROGRAMMING" , 19);
	lseek(fd, -n, SEEK_CUR);  
	read(fd , buffer , 19);
	printf("Written: SYSTEMSPROGRAMMING\n");
	printf("Default read result: %s\n" , buffer);

	for(i=0; i<n; i++)
		buffer[i] = '\0';

	ioctl(fd , VIGENERE_MODE_DECRYPT , "LINUX");
	lseek(fd, -n, SEEK_CUR);
	read(fd , buffer , 19);
	printf("Decrypt mode read result: %s\n" , buffer);	

	for(i=0; i<n; i++)
		buffer[i] = '\0';

	n = write(fd , "MEHMETEFENDIGUCU" , 16);
	lseek(fd, -n, SEEK_CUR);
	read(fd , buffer , 16);
	printf("Written: MEHMETEFENDIGUCU\n");
	printf("Default read (decrypt mode) result: %s\n" , buffer);

	for(i=0; i<n; i++)
		buffer[i] = '\0';
	
	ioctl(fd , VIGENERE_MODE_SIMPLE , "LINUX");
	n = write(fd , "HAYAT" , 5);
	lseek(fd, -n, SEEK_CUR);  
	read(fd , buffer , 5);
	printf("Written: HAYAT\n");
	printf("Default read (simple mode) result: %s\n" , buffer);
	
	close(fd);
	free(buffer);

	return 0;
}
