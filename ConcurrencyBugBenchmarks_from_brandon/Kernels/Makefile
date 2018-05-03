KERNELFAMILIES=sv-kernels mv-kernels


default: build-all-plain


build-all-plain:
	@for i in $(KERNELFAMILIES); do \
	echo "Making in $$i..."; \
	(cd $$i; $(MAKE) plain); done

test:
	@for i in $(KERNELFAMILIES); do \
	echo "Making in $$i..."; \
	(cd $$i; $(MAKE) test); done

clean:
	@for i in $(KERNELFAMILIES); do \
	echo "Making clean in $$i..."; \
	(cd $$i; $(MAKE) $(MFLAGS) $(MYMAKEFLAGS) clean); done
	
