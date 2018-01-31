# LinuxSocketProgramming
Linux下应用套接字抓包C语言实现Capture packets on networking interface with socket in Linux 
要求如下:![要求](./linux大作业实验要求初稿1.0.docx)
1. ![命令行完整版](./ProjectWithCli)
	1. client usage: 
	`$ ./client 127.0.0.1`
	2. server usage 
	`$ ./server 80`
	3. ![代码说明](./ProjectWithCli/代码说明.md)

2. ![命令行精简版](./LinuxSocketPlain)
	1. server usage:
	`$ ./server 80`
	2. client usage:
	`$ ./client 127.0.0.1 80`
	3. ![详细使用说明](./LinuxSocketPlain/使用说明.txt)
3. ![Gtk3+ 图形界面](./ProjectWithGtk)
	使用make命令编译，注意需要gtk3+和pcap环境。代码中![客户端代码](./ProjectWithGtk/client/testGtk3.c)的第94行需要注意使用绝对路径。(由于未知原因，相对路径无法通过编译)

4. ![makefile使用](http://likent.cn/makefile使用)






