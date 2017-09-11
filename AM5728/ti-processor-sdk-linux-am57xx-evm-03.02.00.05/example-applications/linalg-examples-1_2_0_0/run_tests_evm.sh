./matmpy/matmpy > testlog.txt
./dsyrk_test/dsyrk_test >> testlog.txt
./ztrsm_test/ztrsm_test >> testlog.txt
./dgemm_test/dgemm_test >> testlog.txt
./eig/eig >> testlog.txt
./ludinv/ludinv >> testlog.txt
./ztrmm_test/ztrmm_test >> testlog.txt

grep FAIL testlog.txt && echo "tests failed" || echo "All tests have passed."

