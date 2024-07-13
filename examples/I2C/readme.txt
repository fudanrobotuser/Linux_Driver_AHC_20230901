test programs:
	i2cdetect -l
		show all of i2c bus
	i2cdetect -y 7
		show all of i2c devices on i2c-7 bus
	eeprog -x -8 -w 0x00 /dev/i2c-7 0x50
		write 'a' to 'z' at address 0x00 on 0x50 device of i2c-7 bus.
        eeprog -x -8 -r 0x00:26 /dev/i2c-7 0x50
		read 32 bytes at address 0x00 on 0x50 device of i2c-7 bus.