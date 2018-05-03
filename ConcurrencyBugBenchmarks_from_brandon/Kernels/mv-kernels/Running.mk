###############
#Config Options
SHELL=/bin/bash
PINTOOL=/home/blucia0a/cvsandbox/StoreSets/GetStoreSets
PINOPTS=-c 1 -commtab 1 
NUM_ITERS=25

OUTPUTDIR=/home/blucia0a/cvsandbox/bugbench/mv-kernels/output

###########################
#Pin Install Paths Etc. 
###########################
PINDIR=/home/blucia0a/pin
PIN_DRIVER_SCRIPT=$(PINDIR)/pin -mt -follow_execv -t $(PINTOOL) $(PINOPTS)

################
#MV-Kernel Options
################
KERNELBASEDIR=/home/blucia0a/cvsandbox/bugbench/mv-kernels
STDERR=/dev/null
OUTFILE=$(PWD)/mv-kernel-$(KERNEL)-result-file
GRAPHFILE=$(PWD)/graph
BUGGYFILE=$(PWD)/$(KERNEL).buggy
ITERS=`perl $(KERNELBASEDIR)/countTo.pl $(NUM_ITERS)`

########################
#Data Processing Scripts
########################
BUILDCLASSIFIER=/home/blucia0a/cvsandbox/StoreSets/buildClassifier.pl
GRAPHDIFF=/home/blucia0a/cvsandbox/StoreSets/buggy_nonbuggy_graph_diff.pl
NEWGRAPHDIFF=/home/blucia0a/cvsandbox/StoreSets/FullGraph_BuggyNonBuggyDiff.pl

run:
	echo "Running $(KERNEL)..."; \
	for q in $(ITERS) ; do \
		$(PIN_DRIVER_SCRIPT) -- ./$(KERNEL) > $(OUTFILE) 2>$(STDERR); \
		bugginess=`cat $(BUGGYFILE)`; \
		if [ $$bugginess -eq 0 ]; then \
			echo "NonBuggy"; \
		else \
			echo "Buggy"; \
		fi; \
	done

run-multi:
	echo "Running $(KERNEL)..."; \
        if [ ! -d $(OUTPUTDIR)/$(KERNEL) ]; then \
		mkdir -p $(OUTPUTDIR)/$(KERNEL)/Buggy; \
		mkdir -p $(OUTPUTDIR)/$(KERNEL)/NonBuggy; \
        fi; \
	for q in $(ITERS) ; do \
		$(PIN_DRIVER_SCRIPT) -- ./$(KERNEL) > $(OUTFILE) 2>$(STDERR); \
		bugginess=`cat $(BUGGYFILE)`; \
		if [ $$bugginess -eq 0 ]; then \
			echo "NonBuggy"; \
			cp $(OUTFILE) $(OUTPUTDIR)/$(KERNEL)/NonBuggy/$$q; \
		else \
			echo "Buggy"; \
			cp $(OUTFILE) $(OUTPUTDIR)/$(KERNEL)/Buggy/$$q; \
		fi; \
	done 

run-fixed-num:
	echo "Running $(KERNEL)..."; \
        if [ ! -d $(OUTPUTDIR)/$(KERNEL) ]; then \
		mkdir -p $(OUTPUTDIR)/$(KERNEL)/Buggy; \
		mkdir -p $(OUTPUTDIR)/$(KERNEL)/NonBuggy; \
        fi; \
        let qbug=0; \
        let qnonbug=0; \
	while [ $$qbug -lt $(NUM_ITERS) -o $$qnonbug -lt $(NUM_ITERS) ]; do \
		$(PIN_DRIVER_SCRIPT) -- ./$(KERNEL) > $(OUTFILE) 2>$(STDERR); \
		bugginess=`cat $(BUGGYFILE)`; \
		if [ $$bugginess -eq 0 ]; then \
			echo "NonBuggy"; \
			if [ $$qnonbug -lt $(NUM_ITERS) ]; then \
				cp $(OUTFILE) $(OUTPUTDIR)/$(KERNEL)/NonBuggy/$$qnonbug; \
				cp $(GRAPHFILE) $(OUTPUTDIR)/$(KERNEL)/NonBuggy/$$qnonbug.graph; \
                        	let qnonbug=$$qnonbug+1; \
			fi; \
		else \
			echo "Buggy"; \
			if [ $$qbug -lt $(NUM_ITERS) ]; then \
			cp $(OUTFILE) $(OUTPUTDIR)/$(KERNEL)/Buggy/$$qbug; \
			cp $(GRAPHFILE) $(OUTPUTDIR)/$(KERNEL)/Buggy/$$qbug.graph; \
			let qbug=$$qbug+1; \
			fi; \
		fi; \
	done

learn:
	echo "Learning about $(KERNEL)...(output file is at $(OUTPUTDIR)/$(KERNEL)/$(KERNEL).gdiff)"; \
	$(BUILDCLASSIFIER) $(OUTPUTDIR)/$(KERNEL)/Buggy $(OUTPUTDIR)/$(KERNEL)/NonBuggy > $(OUTPUTDIR)/$(KERNEL)/$(KERNEL).arff; \
	echo "Executable: $(PWD)/$(KERNEL)"; \
	$(GRAPHDIFF) $(OUTPUTDIR)/$(KERNEL)/$(KERNEL).arff $(PWD)/$(KERNEL) > $(OUTPUTDIR)/$(KERNEL)/$(KERNEL).gdiff;
	$(NEWGRAPHDIFF) $(OUTPUTDIR)/$(KERNEL)/Buggy $(OUTPUTDIR)/$(KERNEL)/NonBuggy $(PWD)/$(KERNEL) > $(OUTPUTDIR)/$(KERNEL)/$(KERNEL).fgdiff
