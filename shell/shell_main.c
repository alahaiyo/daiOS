
int shell_parse(char * buf, char * argv[])
{
	int argc = 0;
	int state = 0;
	
	// ��������ַ�
	while (*buf)
	{
		// ���һ������
		if (*buf != ' ' && state == 0)
		{
			argv[argc++] = buf;
			state = 1;
		}
		
		// �����ո�
		if (*buf == ' ' && state == 1)
		{
			*buf = '\0';
			state = 0;
		}
		
		buf++;	
	}
	
	return argc;
}

void command_do(int argc, char * argv[])
{
//	printk("argc = %d\r\n", argc);
//	while(argc-- > 0) {
//		printk("argv[%d] = %s\r\n", argc, argv[argc]);
//	}
	if(!strcmp("reboot", argv[0]))
	{
		do_reboot();
	}
	if(!strcmp("ls", argv[0])) {
		do_ls();
	}
	
}

void shell_main_loop()
{
	char buf[64];
	int argc = 0;
	char * argv[10];
	
	while (1)
	{
		printk("#daios: ");
		gets(buf);	
		printk("\r");		
	// 	printk("\r%s\r\n", buf);
		argc = shell_parse(buf, argv);			// ��������
		command_do(argc, argv);					// ��������
	}
}