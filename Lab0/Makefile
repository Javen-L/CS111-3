# NAME: Anh Mac
# EMAIL: anhmvc@gmail.com
# ID: 905111606

lab0: lab0.c
	gcc -g -Wall -Wextra -o lab0 lab0.c

check:	test1 test2 test3 test4 test5 test6 test7 test8 test9
	if [ -s error.txt ]; then \
		cat error.txt; \
		rm -f error.txt; \
	else \
		echo "VIBE CHECK PASSED!"; \
	fi; \

clean:
	rm -f lab0 *.tar.gz *.txt

dist:
	tar -czvf lab0-905111606.tar.gz lab0.c Makefile backtrace.png breakpoint.png README


# no arguments
test1:
	echo "wassup homies" > input.txt; \
	./lab0 < input.txt > output.txt; \
	if [ $$? -eq 0 ]; then \
		echo "//=== PASSED TEST 1 ===/// [no arguments]. Program exited with code 0."; \
	else \
		echo "** FAILED TEST 1 ** [no arguments]. Program did not exit with code 0." > error.txt; \
	fi; \
	rm -f output.txt

# invalid arguments
test2:
	./lab0 --bogus < input.txt; \
	if [ $$? -eq 1 ]; then \
		echo "//=== PASSED TEST 2 ===// [invalid argument]. Program exited with code 1."; \
	else \
		echo "** FAILED TEST 2 ** [invalid argument]. Program did not exit with code 1." > error.txt; \
	fi; \

# invalid input file
test3:
	./lab0 --input=doesNotExist.txt; \
	if [ $$? -eq 2 ]; then \
		echo "//=== PASSED TEST 3 ===// [invalid input file]. Program exited with code 2."; \
	else \
		echo "** FAILED TEST 3 ** [invalid input file]. Program did not exit with code 2." > error.txt; \
	fi; \

# invalid output file
test4:
	echo "Bad!!!" > invalidOutput.txt; \
	chmod -w invalidOutput.txt; \
	./lab0 --input=input.txt --output=invalidOutput.txt; \
	if [ $$? -eq 3 ]; then \
		echo "//=== PASSED TEST 4 ===// [invalid output file]. Program exited with code 3."; \
	else \
		echo "** FAILED TEST 4 ** [invalid output file]. Program did not exit with code 3." > error.txt; \
	fi; \
	rm -f invalidOutput.txt

# --catch argument with --segfault
test5:
	./lab0 --segfault --catch; \
	if [ $$? -eq 4 ]; then \
		echo "//=== PASSED TEST 5 ===// [catch with segfault]. Program exited with code 4."; \
	else \
		echo "** FAILED TEST 5 ** [catch with segfault]. Program did not exit with code 4." > error.txt; \
	fi; \

# valid I/O argument
test6:
	echo "vibe check" > testinput.txt; \
	./lab0 --input=testinput.txt --output=testoutput.txt; \
	cmp testinput.txt testoutput.txt; \
	if [ $$? -eq 0 ]; then \
		echo "//=== PASSED TEST 6 ===// [specified I/O]. Program exited with code 0."; \
	else \
		echo "** FAILED TEST 6 ** [specified I/O]. Program did not exit with code 0." > error.txt; \
	fi; \
	rm -f testinput.txt testoutput.txt

# valid input argument
test7:
	./lab0 --input=input.txt > output.txt; \
	cmp input.txt output.txt; \
	if [ $$? -eq 0 ]; then \
		echo "//=== PASSED TEST 7 ===// [valid input file]. Program exited with code 0."; \
	else \
		echo "** FAILED TEST 7 ** [valid input file]. Program did not exit with code 0." > error.txt; \
	fi; \
	rm -f output.txt

# valid output argument
test8:
	./lab0 --output=output.txt < input.txt; \
	cmp input.txt output.txt; \
	if [ $$? -eq 0 ]; then \
		echo "//=== PASSED TEST 8 ===// [valid output file]. Program exited with code 0."; \
	else \
		echo "** FAILED TEST 8 ** [valid output file]. Program did not exit with code 0." > error.txt; \
	fi; \
	rm -f output.txt

# --catch without --segfault
test9:
	./lab0 --catch < input.txt > output.txt; \
	cmp input.txt output.txt; \
	if [ $$? -eq 0 ]; then \
		echo "//=== PASSED TEST 9 ===// [catch without segfault]. Program exited with code 0."; \
	else \
		echo "** FAILED TEST 9 ** [catch without segfault]. Program did not exit with code 0." > error.txt; \
	fi; \
	rm -f output.txt
