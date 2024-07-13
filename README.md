研华工控机的 CAN 驱动安装


Issues Description:

	Rebuild the drivers of the path 20220421_Linux_Kernel_4.13_to_6.2.0 on Ubuntu 22.04.2 LTS(kernel 6.2),
	and patch the issues until 2023/09/01
	
Patch Items:

	CAN: 
		Issue: build fail on kernel 5.19
		Root: function netif_napi_add() parameter define change
		Patch: 	netif_napi_add
		    	
	GPIO: 
		Issue: build fail on kernel 5.19
		Root: lack of the build fail on kernel 5.19
		Patch: add #include <linux/kdev_t.h> 
		
	COM:
		Issue: build fail on kernel 5.19
		Root: function uart_ops_set_termios() parameter define change
		Patch: uart_ops_set_termios()
		
Tested Build Enviroment:
	Ubuntu 18.04 LTS kernel version 4.15 
	Ubuntu 18.04 LTS kernel version 5.8.1
	Ubuntu 20.04.4 LTS kernel version 5.13.0
	Ubuntu 22.04.2 LTS kernel version 5.19 	
	Ubuntu 22.04.3 LTS Kernel version 6.2.0

