test programs:
	./tst-raw -i can0 -smbdlr
		-smbdlr
		s: send
		m: show message
		b: block operation.
			In general, you should use this flag.
		d: dealy usleep(50)
		l: let tx message loopback(copy) into its socket buffer and mark this message with echo.
			echo message will removed from socket buffer after tx done.
			In general, you should use this flag.
		r: receive own (echo) message
	./tst-raw-r -i can0 -smbl
		receiving state.
        
	./tst-raw-s -i can1 -smblr
		send messages.

