all:
	cd blik && $(MAKE) all
	cd post && $(MAKE) all && ./test
	cd boot && $(MAKE) all
	cd post && ./test
	cd midi && $(MAKE) all
	cd http && $(MAKE) all
clean:
	cd blik && $(MAKE) clean
	cd boot && $(MAKE) clean
	cd midi && $(MAKE) clean
	cd post && $(MAKE) clean
	cd http && $(MAKE) clean
