
EXE = ztrsm_test

include ../make.inc

$(EXE): ztrsm_test.o 
	$(CC) $(CFLAGS) ztrsm_test.o $(LD_FLAGS) $(BLASLIB) -o $@

run: ARMtest DSPtest OPTtest

ARMtest: $(EXE)
	@echo "Forcing BLAS level 3 execution on ARM."; \
	export TI_CBLAS_OFFLOAD=000; \
	./$(EXE); \
	cp ztrsm_time.dat ztrsm_time_ARM.dat; cp ztrsm_gflops.dat ztrsm_gflops_ARM.dat;

DSPtest: $(EXE)
	@echo "Forcing BLAS level 3 execution on DSP."; \
	export TI_CBLAS_OFFLOAD=001; \
	./$(EXE); \
	cp ztrsm_time.dat ztrsm_time_DSP.dat; cp ztrsm_gflops.dat ztrsm_gflops_DSP.dat;

OPTtest: $(EXE)
	@echo "Optimal BLAS level 3 execution on ARM or DSP."; \
	export TI_CBLAS_OFFLOAD=002; \
	./$(EXE); \
	cp ztrsm_time.dat ztrsm_time_OPT.dat; cp ztrsm_gflops.dat ztrsm_gflops_OPT.dat;
