EXE = eig

include ../make.inc

$(EXE): main.o dlaran.o dlarnd.o dlatm1.o dlatm2.o dlatm3.o dlatmr.o
	$(CC) $(CFLAGS) main.o dlaran.o dlarnd.o dlatm1.o dlatm2.o dlatm3.o dlatmr.o $(LD_FLAGS) $(LAPACKLIB) -o $@

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

