#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

MODULE_LICENSE("GPL");

unsigned long **sys_call_table;

asmlinkage long (*old_sys_exit)(int);
void (*old_cstar_entry)(void);
// static unsigned long long old_cstar_entry;

asmlinkage long new_sys_exit(int error_code) {
  printk(KERN_INFO "sys_exit intercepted (error_code %d)", error_code);
  return old_sys_exit(error_code);
}

asmlinkage void new_cstar_entry(void) { old_cstar_entry(); }

static void change_cstar_target(void) {
  int cpu;

  for_each_present_cpu(cpu) {

    if (boot_cpu_has(X86_FEATURE_SYSCALL32)) {
      printk(KERN_INFO "syscall compatibility mode found");

      old_cstar_entry = (void *)native_read_msr(MSR_CSTAR);
      if (old_cstar_entry != 0) {
        wrmsrl_on_cpu(cpu, MSR_CSTAR, (unsigned long)new_cstar_entry);
        printk(KERN_INFO "set new syscall compatibility mode entry: %lx "
                         "for CPU: %i\n",
               (unsigned long)new_cstar_entry, cpu);
      }
    }
  }
}

static void restore_cstar_entry(void) {
  int cpu;

  for_each_present_cpu(cpu) {
    wrmsrl_on_cpu(cpu, MSR_CSTAR, (unsigned long)old_cstar_entry);
  }
}

static unsigned long **aquire_sys_call_table(void) {
  unsigned long int offset = PAGE_OFFSET;
  unsigned long **sct;

  while (offset < ULLONG_MAX) {
    sct = (unsigned long **)offset;

    if (sct[__NR_close] == (unsigned long *)sys_close)
      return sct;

    offset += sizeof(void *);
  }

  return NULL;
}

static void disable_page_protection(void) {
  unsigned long reg_cr;
  asm volatile("mov %%cr0, %0" : "=r"(reg_cr));

  if (!(reg_cr & 0x00010000))
    return;

  asm volatile("mov %0, %%cr0" : : "r"(reg_cr & ~0x00010000));
}

static void enable_page_protection(void) {
  unsigned long reg_cr;
  asm volatile("mov %%cr0, %0" : "=r"(reg_cr));

  if ((reg_cr & 0x00010000))
    return;

  asm volatile("mov %0, %%cr0" : : "r"(reg_cr | 0x00010000));
}

static int __init interceptor_start(void) {
  if (!(sys_call_table = aquire_sys_call_table())) {
    printk(KERN_INFO "Interceptor abort loading.\n");
    return -1;
  }

  disable_page_protection();
  old_sys_exit = (void *)sys_call_table[__NR_exit];
  sys_call_table[__NR_exit] = (unsigned long *)new_sys_exit;
  enable_page_protection();

  change_cstar_target();

  printk(KERN_INFO "Interceptor: new exit function on addr: %lx\n",
         (unsigned long)new_sys_exit);
  return 0;
}

static void __exit interceptor_end(void) {

  printk(KERN_INFO "unloading Interceptor...\n");

  if (!sys_call_table)
    return;

  restore_cstar_entry();

  disable_page_protection();
  sys_call_table[__NR_exit] = (unsigned long *)old_sys_exit;
  enable_page_protection();
}

module_init(interceptor_start);
module_exit(interceptor_end);
