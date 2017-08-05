
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

char _getchar()
{
	while(UART_FR & (1 << 4));
	return UART_DR;
}

int getchar(void)
{
	int c;

	c = (int)_getchar();

	if (c == '\r')
		return '\n';

	return c;
}

char * gets(char * s)
{
	char * p = s;

	while ((*p = getchar()) != '\n')
	{
		if (*p != '\b')
			_putchar(*p++);
		else
			if (p > s)
				_putchar(*p--);
	}

	*p = '\0';
	_putchar('\n');

	return s;
}

