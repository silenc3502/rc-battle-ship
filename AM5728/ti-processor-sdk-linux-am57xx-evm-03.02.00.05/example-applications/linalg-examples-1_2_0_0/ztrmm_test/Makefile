
EXE = ztrmm_test

include ../make.inc

$(EXE): ztrmm_test.o 
	$(CC) $(CFLAGS) ztrmm_test.o $(LD_FLAGS) $(BLASLIB) -o $@

run: ARMtest DSPtest OPTtest

ARMtest: $(EXE)
	@echo "Forcing BLAS level 3 execution on ARM."; \
	export TI_CBLAS_OFFLOAD=000; \
	./$(EXE); \
	cp ztrmm_time.dat ztrmm_time_ARM.dat; cp ztrmm_gflops.dat ztrmm_gflops_ARM.dat;

DSPtest: $(EXE)
	@echo "Forcing BLAS level 3 execution on DSP."; \
	export TI_CBLAS_OFFLOAD=001; \
	./$(EXE); \
	cp ztrmm_time.dat ztrmm_time_DSP.dat; cp ztrmm_gflops.dat ztrmm_gflops_DSP.dat;

OPTtest: $(EXE)
	@echo "Optimal BLAS level 3 execution on ARM or DSP."; \
	export TI_CBLAS_OFFLOAD=002; \
	./$(EXE); \
	cp ztrmm_time.dat ztrmm_time_OPT.dat; cp ztrmm_gflops.dat ztrmm_gflops_OPT.dat;
