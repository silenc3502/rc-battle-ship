
EXE = dgemm_test

include ../make.inc

$(EXE): dgemm_test.o
	$(CC) $(CFLAGS) dgemm_test.o $(LD_FLAGS) $(BLASLIB) -o $@

run: ARMtest DSPtest OPTtest

ARMtest: $(EXE)
	@echo "Forcing BLAS level 3 execution on ARM."; \
	export TI_CBLAS_OFFLOAD=000; \
	./$(EXE); \
	cp dgemm_time.dat dgemm_time_ARM.dat; cp dgemm_gflops.dat dgemm_gflops_ARM.dat;

DSPtest: $(EXE)
	@echo "Forcing BLAS level 3 execution on DSP."; \
	export TI_CBLAS_OFFLOAD=001; \
	./$(EXE); \
	cp dgemm_time.dat dgemm_time_DSP.dat; cp dgemm_gflops.dat dgemm_gflops_DSP.dat;

OPTtest: $(EXE)
	@echo "Optimal BLAS level 3 execution on ARM or DSP."; \
	export TI_CBLAS_OFFLOAD=002; \
	./$(EXE); \
	cp dgemm_time.dat dgemm_time_OPT.dat; cp dgemm_gflops.dat dgemm_gflops_OPT.dat;
