
EXE = dsyrk_test

include ../make.inc

$(EXE): dsyrk_test.o 
	$(CC) $(CFLAGS) dsyrk_test.o $(LD_FLAGS) $(BLASLIB) -o $@

run: ARMtest DSPtest OPTtest

ARMtest: $(EXE)
	@echo "Forcing BLAS level 3 execution on ARM."; \
	export TI_CBLAS_OFFLOAD=000; \
	./$(EXE); \
	cp dsyrk_time.dat dsyrk_time_ARM.dat; cp dsyrk_gflops.dat dsyrk_gflops_ARM.dat;

DSPtest: $(EXE)
	@echo "Forcing BLAS level 3 execution on DSP."; \
	export TI_CBLAS_OFFLOAD=001; \
	./$(EXE); \
	cp dsyrk_time.dat dsyrk_time_DSP.dat; cp dsyrk_gflops.dat dsyrk_gflops_DSP.dat;

OPTtest: $(EXE)
	@echo "Optimal BLAS level 3 execution on ARM or DSP."; \
	export TI_CBLAS_OFFLOAD=002; \
	./$(EXE); \
	cp dsyrk_time.dat dsyrk_time_OPT.dat; cp dsyrk_gflops.dat dsyrk_gflops_OPT.dat;
