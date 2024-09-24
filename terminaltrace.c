#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A module for logging everything from a terminal");
MODULE_AUTHOR("@quesadilla_exe");

#define read_cr0() ({ unsigned long __cr0; \
    asm volatile("mov %%cr0,%0" : "=r" (__cr0)); __cr0; })

#define write_cr0(x) \
    asm volatile("mov %0,%%cr0" :: "r" ((unsigned long)(x)))

static unsigned long cr0;

// Function pointers for original and new execve
asmlinkage long (*original_execve)(const char __user *filename, const char __user *const __user *argv, const char __user *const __user *envp) = NULL;
asmlinkage long my_execve(const char __user *filename, const char __user *const __user *argv, const char __user *const __user *envp);

// Function to hook execve
asmlinkage long my_execve(const char __user *filename, const char __user *const __user *argv, const char __user *const __user *envp) {
    // Log the execve call
    printk(KERN_INFO "execve called for: %s\n", filename);
    
    // Call the original execve
    return original_execve(filename, argv, envp);
}


// Hooking function
static void hook_execve(void) {
    // Save current CR0 value
    cr0 = read_cr0();
    // Disable write protection by clearing the WP bit in CR0
    write_cr0(cr0 & ~0x10000);
    // Hook execve
    original_execve = (void *)sys_call_table[__NR_execve];
    sys_call_table[__NR_execve] = my_execve;
    // Restore write protection
    write_cr0(cr0);
}

// Unhooking function
static void unhook_execve(void) {
    // Save current CR0 value
    cr0 = read_cr0();
    // Disable write protection
    write_cr0(cr0 & ~0x10000);
    // Restore original execve
    sys_call_table[__NR_execve] = original_execve;
    // Restore write protection
    write_cr0(cr0);
}

static int __init terminaltrace_init(void) {
    printk(KERN_INFO "TerminalTrace: Initializing module\n");
    hook_execve();
    return 0;
}

static void __exit terminaltrace_exit(void) {
    printk(KERN_INFO "TerminalTrace: Exiting module\n");
    unhook_execve();
}

module_init(terminaltrace_init);
module_exit(terminaltrace_exit);


