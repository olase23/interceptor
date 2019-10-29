#include <linux/kernel.h>
#include <linux/syscalls.h>

unsigned long **sys_call_table;

asmlinkage long (*old_sys_exit)(int);

asmlinkage long new_sys_exit(int error_code)
{
	printk(KERN_INFO "sys_exit intercepted (error_code %d)", error_code);
	return old_sys_exit(error_code);
}

static unsigned long **aquire_sys_call_table(void)
{
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;

	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;

		if (sct[__NR_close] == (unsigned long *) sys_close)
			return sct;

		offset += sizeof(void *);
	}

	return NULL;
}

static void disable_page_protection(void)
{
	unsigned long reg_cr;
	asm volatile("mov %%cr0, %0" : "=r" (reg_cr));

	if(!(reg_cr & 0x00010000))
		return;

	asm volatile("mov %0, %%cr0" : : "r" (reg_cr & ~0x00010000));
}

static void enable_page_protection(void)
{
	unsigned long reg_cr;
	asm volatile("mov %%cr0, %0" : "=r" (reg_cr));

	if((reg_cr & 0x00010000))
		return;

	asm volatile("mov %0, %%cr0" : : "r" (reg_cr | 0x00010000));
}

static int __init interceptor_start(void)
{
	if(!(sys_call_table = aquire_sys_call_table()))
		return -1;

	disable_page_protection();
	old_sys_exit = (void *)sys_call_table[__NR_exit];
	sys_call_table[__NR_exit] = (unsigned long *)new_sys_exit;
	enable_page_protection();
	printk(KERN_INFO "Interceptor successfully loaded...\n");
	return 0;
}

static void __exit interceptor_end(void)
{

	printk(KERN_INFO "unloading Interceptor...\n");

	if(!sys_call_table)
		return;

	disable_page_protection();
	sys_call_table[__NR_exit] = (unsigned long *)old_sys_exit;
	enable_page_protection();
}

module_init(interceptor_start);
module_exit(interceptor_end);
