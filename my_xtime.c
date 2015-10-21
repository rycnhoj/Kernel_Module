#ifndef __MY_XTIME_C
#define __MY_XTIME_C

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/time.h>
#include <asm-generic/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Evan Lee");
MODULE_AUTHOR("John Cyr");
MODULE_AUTHOR("Abe Omari");
MODULE_DESCRIPTION("Upon read, outputs Epoch milliseconds to /proc/timed.");

#define ENTRY_NAME "timed"
#define PERMS 0644
#define PARENT NULL
static struct file_operations fops;

static char *message;
static int read_p;
static struct timespec lasttime = {-1, -1};

int my_xtime_open(struct inode *sp_inode, struct file *sp_file){
	read_p = 1;
	message = kmalloc(sizeof(char) * 256, __GFP_WAIT|__GFP_IO|__GFP_FS);
	if (message == NULL) {
		return -ENOMEM;
	}
	return 0;
}

ssize_t my_xtime_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset){
	struct timespec time;
	//char timestr[64];

	time = current_kernel_time();
	sprintf(message, "current time: %lld.%09ld\n", (long long) time.tv_sec, time.tv_nsec);
	if(lasttime.tv_sec != -1){
		char newtime[64];
		long long esec, ensec, msec;
		msec = time.tv_nsec;
		if (lasttime.tv_nsec > time.tv_nsec){
			time.tv_sec = time.tv_sec - 1;
			msec = time.tv_nsec + 1000000000;
		}
		esec = (long long) time.tv_sec - (long long) lasttime.tv_sec;
		ensec = (long long) msec - (long long) lasttime.tv_nsec;
		sprintf(newtime, "elapsed time: %lld.%09lld\n", esec, ensec);
		strcat(message, newtime);
	}
	lasttime.tv_sec = time.tv_sec;
	lasttime.tv_nsec = time.tv_nsec;
	int len;
	len = strlen(message);
	read_p = !read_p;
	if (read_p){
		return 0;
	}
	copy_to_user(buf, message, len);

	return len;
}

int my_xtime_release(struct inode *sp_inode, struct file *sp_file){
	kfree(message);
	return 0;
}

static int my_xtime_init(void){
	fops.open = my_xtime_open;
	fops.read = my_xtime_read;
	fops.release = my_xtime_release;

	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)){
		printk("ERROR: proc creation\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}
	return 0;
}

static void my_xtime_exit(void){
	remove_proc_entry(ENTRY_NAME, NULL);
}

module_init(my_xtime_init);
module_exit(my_xtime_exit);

#endif
