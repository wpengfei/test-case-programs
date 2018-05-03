default: plain

plain:
	@for i in $(KERNELS); do \
	echo "Making all in $$i..."; \
	(cd $$i; $(MAKE) plain); done

test:
	@for i in $(KERNELS); do \
	echo "Making all in $$i..."; \
	(cd $$i; $(MAKE) test); done

clean:
	@for i in $(KERNELS); do \
	echo "Making clean in $$i..."; \
	(cd $$i; $(MAKE) clean); done
