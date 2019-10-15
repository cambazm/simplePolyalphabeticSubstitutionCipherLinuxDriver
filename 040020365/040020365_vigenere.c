/************************************
 * SYSTEMS PROGRAMMING ASSIGNMENT 2 *
 * STUDENT: MEHMET CAMBAZ	    *
 * NUMBER:  040020365		    *
 ************************************/

/* this code is based on "scull" code that is shown at class */

#ifndef __KERNEL__
	#define __KERNEL__
#endif
#ifndef MODULE
	#define MODULE
#endif

#include <linux/config.h>
#include <linux/module.h>

#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */


#include <asm/system.h>     /* cli(), *_flags */
#include <asm/uaccess.h>    /* cli(), *_flags */

#include "040020365_vigenere.h"

#ifndef VIGENERE_MAJOR_NUMBER
	#define VIGENERE_MAJOR_NUMBER 0
#endif

#define DEFAULT_BUFFER_SIZE 4  // default buffer size is 4KB
#define DEFAULT_KEY "A"
#define DECRYPTING_MODE 0
#define SIMPLE_MODE 1
#define NO 0
#define YES 1



/* global variables default values */
int vigenere_major = VIGENERE_MAJOR_NUMBER;	//default major number is dynamically given so 0
int kbuffersize = DEFAULT_BUFFER_SIZE;		//default kernel buffer size is 4 KB
char *key = DEFAULT_KEY;			//default key is "A"
int device_opened = NO;				//at the beginning device is not opened


/* sample usage: (if want to give parameters, it is possible to give parameters, also not mandatory) 
   insmod vigenere.o vigenere_major=254 kbuffersize=4 key="LINUX" 
*/
/* module parameters */
MODULE_PARM(vigenere_major,"i");		//major number
MODULE_PARM(kbuffersize,"i");			//kernel buffer size
MODULE_PARM(key,"s");				//cipher key

/* module specifications */
MODULE_AUTHOR("MEHMET CAMBAZ");
MODULE_LICENSE("GPL");


/* Function prototypes */
int vigenere_init(void);
void vigenere_cleanup(void);
int vigenere_open(struct inode *, struct file *);
int vigenere_release(struct inode *, struct file *);
ssize_t vigenere_read(struct file *, char *, size_t, loff_t *);
ssize_t vigenere_write(struct file *, char *, size_t, loff_t *);
int vigenere_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
loff_t vigenere_llseek(struct file *, loff_t, int);

/* file operations available */
struct file_operations vigenere_fops = {
	open:    vigenere_open,
	release: vigenere_release,
	read:    vigenere_read,
	write:   vigenere_write,
	ioctl:   vigenere_ioctl,
	llseek:  vigenere_llseek,
};


/* Device Structure */
typedef struct Vigenere_Dev{
	int vigenere_major;			//device major number
	char *kbuffer;				//kernel buffer
	unsigned long kbuffersize;		//kernel buffer size
	char *letters;				//uppercase english letters
	char *key;				//key string for encryption
	int mode;				//mode of the device (simple mode or decrypting mode)
	int plain_text;				//does the kernel buffer consist of plain text (decrypted or encrypted text)
	struct semaphore sem;
} Vigenere_Dev;


Vigenere_Dev *vd;	/* device structure variable */



/* init module: do necessary allocations, make module ready for usage */
int vigenere_init(void)
{
	int result, i;
	
	/* set module */
	SET_MODULE_OWNER(&vigenere_fops);
	
	/* register the character device */
	result = register_chrdev(vigenere_major, "vigenere", &vigenere_fops);
    
    	/* if major number cannot be given, error */
    	if (result < 0) 
	{
        	printk("<1>vigenere: can not get major number %d\n",vigenere_major);
        	return result;
    	}
	
	/* allocate the memory for device usage */
	vd = kmalloc(sizeof(Vigenere_Dev), GFP_KERNEL);
	if(vd == NULL)
		return -ENOMEM;
	
	/* clear the memory for device usage */
	memset(vd, 0, sizeof(Vigenere_Dev));
    
    	/* give major number the returned value from the function if dynamic */
    	if (vigenere_major == 0)
        	vd->vigenere_major = result;
	else
		vd->vigenere_major = vigenere_major;

	vd->kbuffersize = kbuffersize*1024;	//kb
	/* allocate memory */
	vd->kbuffer = kmalloc(vd->kbuffersize, GFP_KERNEL);
	vd->letters = kmalloc(sizeof("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), GFP_KERNEL);
	strcpy(vd->letters, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	vd->key = kmalloc(sizeof(key), GFP_KERNEL);
	strcpy(vd->key, key);
	vd->plain_text = NO;
	
	printk("<1>vigenere: module start: [ OK ]\n");

   	return 0;
}


/* cleanup module: do necessary stops, free the memory allocated */
void vigenere_cleanup(void)
{	
	if(unregister_chrdev(vd->vigenere_major, "vigenere") < 0)
		printk("<1>vigenere: cleanup failed\n");
	
	kfree(vd->kbuffer);
	kfree(vd->letters);
	kfree(vd->key);
	kfree(vd);
	
	printk("<1>vigenere: module remove: [ OK ]\n");
}


/* open module: to do's when opening the device for usage */
int vigenere_open(struct inode *inode, struct file *filp)
{
	/* only one process can access this device at a time,
	   it is a single open device, if already opened, say that device is busy */
	if(device_opened == YES)
		return -EBUSY;
		 
	MOD_INC_USE_COUNT;
	
	vd->mode = SIMPLE_MODE;		//default mode is simple mode
	
	device_opened = YES;		//single-open device, device is now opened
	
	return 0;
}



/* release module: to do's when releasing the device from usage */
int vigenere_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	
	device_opened = NO;		//single-open device, device usage is stopped, device is closed now
	
	return 0;
}



/* write: to do's when write request comes to the device */
ssize_t vigenere_write(struct file *filp, char *ubuffer, size_t ubuffer_len, loff_t *f_pos)
{
	int i;
	
	/* if the user buffer's size is larger than kernel buffer,
	   take as much as kernel buffer size can take */
	if( ubuffer_len >= vd->kbuffersize )
		copy_from_user(vd->kbuffer, ubuffer, vd->kbuffersize);
	else
		copy_from_user(vd->kbuffer, ubuffer, ubuffer_len);
		
	//printk("<1>vigenere: user sent this buffer: %s\n" , vd->kbuffer);
	
	/* encrypt taken buffer with vigenere cipher algorithm */
	for(i = 0; vd->kbuffer[i]!='\0'; i++) 
		vd->kbuffer[i] = vd->letters[( ( vd->kbuffer[i] % 65 ) + ( vd->key[i % strlen(vd->key)] % 65 ) ) % 26 ];
	
	vd->plain_text = NO;		//kernel buffer consist of encrypted text now
	
	//printk("<1>vigenere: encrypted buffer is: %s\n" , vd->kbuffer);

	*f_pos += ubuffer_len;		//file position incremented
	
	return ubuffer_len;
}


/* read: to do's when read request comes to the device */
ssize_t vigenere_read(struct file *filp, char *ubuffer, size_t ubuffer_len, loff_t *f_pos)
{
	int i;

	/* to decrypt, the mode must be DECRYPTING_MODE and kernel buffer must be consisted of non-plain (encrypted) text */
	if(vd->mode == DECRYPTING_MODE && vd->plain_text == NO)	
	{
		/* decrypt the string at kernel buffer */
		for(i = 0; vd->kbuffer[i]!='\0'; i++) 
			vd->kbuffer[i] = vd->letters[(( vd->kbuffer[i] % 65 - vd->key[i % strlen(vd->key)] % 65 ) + 26) % 26];
			
		//printk("<1>vigenere: decrypted : %s\n" , vd->kbuffer);
		vd->plain_text = YES;			//kernel buffer consists of decrypted (plain) text now
	}
	/* at SIMPLE_MODE there is nothing to do but copy kernel buffer to user buffer */
	
	/* copy kernel buffer to user buffer */
	copy_to_user(ubuffer, vd->kbuffer, ubuffer_len);

	*f_pos += ubuffer_len;		//file position incremented
	
	return strlen(vd->kbuffer);
}


/* ioctl: to get control commands with parameters and do necessary operations */
int vigenere_ioctl(struct inode* inode , struct file* filp , unsigned int command , unsigned long parameter)
{
	char *given_key;
	int ret = 0;
	
	given_key = (char*)parameter;
	
	switch(command)
	{	
		/* decrypting mode selected */
		case VIGENERE_MODE_DECRYPT:
			/* if given key is equal to present key */
			if(strcmp(given_key , vd->key) == 0)
			{
				vd->mode = DECRYPTING_MODE;
				printk("<1>vigenere: now at DECRYPTING_MODE\n");
			}
			else 
			{
				printk("<1>vigenere: given key is invalid\n");
				ret = -1;	/*error returning value*/
			}
			break;
			
		/* simple mode selected */
		case VIGENERE_MODE_SIMPLE:
		
			/* if given key is equal to present key */
			if(strcmp(given_key , vd->key) == 0)
			{
				vd->mode = SIMPLE_MODE;
				printk("<1>vigenere: now at SIMPLE_MODE\n");	
			}
			else
			{
				printk("<1>vigenere: given key is invalid\n");
				ret = -1;	/*error returning value*/
			}
			break;
			
		default :
			printk("<1>Unknown command\n");
			return -EINVAL;
			
	}
	return ret;
}


/* llseek: seek to the place given */
loff_t vigenere_llseek(struct file *filp, loff_t off, int whence)
{
	loff_t newpos;

	switch(whence) {
	      case 0: /* SEEK_SET */
        	newpos = off;
        	break;

	      case 1: /* SEEK_CUR */
        	newpos = filp->f_pos+off;
        	break;

	      case 2: /* SEEK_END */
        	newpos = vd->kbuffersize+off;
	        break;

	      default: /* can't happen */
        	return -EINVAL;
    	}

	if (newpos < 0)
        	return -EINVAL;
	
	filp->f_pos = newpos;
	
	return newpos;
}

module_init(vigenere_init);
module_exit(vigenere_cleanup);
