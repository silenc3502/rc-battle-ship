EXE = matmpy

include ../make.inc

$(EXE): main.o
	$(CC) $(CFLAGS) main.o $(LD_FLAGS) $(BLASLIB) -o $@

run: ARMtest DSPtest OPTtest

ARMtest: $(EXE)
	@echo "Forcing BLAS level 3 execution on ARM."; \
	export TI_CBLAS_OFFLOAD=000; \
	./$(EXE);

DSPtest: $(EXE)
	@echo "Forcing BLAS level 3 execution on DSP."; \
	export TI_CBLAS_OFFLOAD=001; \
	./$(EXE);

OPTtest: $(EXE)
	@echo "Optimal BLAS level 3 execution on ARM or DSP."; \
	export TI_CBLAS_OFFLOAD=002; \
	./$(EXE);



