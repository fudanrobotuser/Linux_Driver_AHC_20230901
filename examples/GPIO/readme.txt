test programs:
	/sys/class/gpio/gpiochipN/
		"base" ... same as N, the first GPIO managed by this chip
		"label" ... provided for diagnostics (not always unique)
		"ngpio" ... how many GPIOs this manages (N to N + ngpio - 1)
	/sys/class/gpio/
		"export" ... Userspace may ask the kernel to export control of
			a GPIO to userspace by writing its number to this file.
			Example:  "echo 19 > export" will create a "gpio19" node
			for GPIO #19, if that's not requested by kernel code.
		"unexport" ... Reverses the effect of exporting to userspace.
			Example:  "echo 19 > unexport" will remove a "gpio19"
			node exported using the "export" file.
	/sys/class/gpio/gpioN/
		"direction" ... reads as either "in" or "out"
		"value"... reads as either 0 (low) or 1 (high).
		

