
EXE = gemm_bench

include ../make.inc

$(EXE): main.o
	$(CC) $(CFLAGS) main.o $(LD_FLAGS) $(BLASLIB) -o $@

run: $(EXE)
	@echo "Benchmarking xGEMM"; \
	export TI_CBLAS_OFFLOAD=002; \
	./$(EXE);
