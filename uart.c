
#define UART_DR (*(volatile unsigned int *)0x12100000)
#define UART_FR (*(volatile unsigned int *)0x12100018)

_putchar(char a)
{
	while(UART_FR & (1 << 5));
	UART_DR = a;	
}

void puts(char *s)
{
	uart_early_puts(s);
}

