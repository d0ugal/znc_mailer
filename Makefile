push.so: mailer.cpp
	znc-buildmod mailer.cpp

clean:
	-rm -f mailer.so
