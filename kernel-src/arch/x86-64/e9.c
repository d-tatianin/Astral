#include <arch/io.h>

void arch_e9_putc(char c) {
#ifdef X86_64_ENABLE_E9
	outb(0xe9, c);
#endif
}

void arch_e9_puts(char *c) {
#ifdef X86_64_ENABLE_E9
	while (*c) {
		outb(0xe9, *c++);
	}
#endif
}
