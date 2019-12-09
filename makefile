CC = gcc
MA_OBJS = matamazom.o amount_set.o matamazom_print.o tests/matamazom_main.o tests/matamazom_tests.o 
AS_OBJS = amount_set.o mtm tests/amount_set_main.o tests/amount_set_tests.o
EXEC1 = matamazom
EXEC2 = amount_set 
COMP_FLAG =  -std=c99 -Wall -Werror -pedantic-errors ñDNDEBUG


$(EXEC1): $(MA_OBJS)
	$(CC) $(MA_OBJS) -o -L. -lm -lmtm $@

$(EXEC2): $(AS_OBJS)
	$(CC) $(AS_OBJS) -o $@

matamazom.o: matamazom.c matamazom.h amount_set.h matamazom_print.h list.h
	$(CC) -c $(COMP_FLAG) $*.c

amount_set.o: amount_set.c amount_set.h
	$(CC) -c $(COMP_FLAG) $*.c

matamazom_print.o: matamazom_print.c matamazom_print.h
	$(CC) -c $(COMP_FLAG) $*.c

tests/matamazom_main.o: tests/matamazom_main.c tests/matamazom.h
	$(CC) -c $(COMP_FLAG) $*.c

tests/matamazom_tests.o: tests/matamazom_tests.c tests/matamazom_tests.h matamazom.h 
	$(CC) -c $(COMP_FLAG) $*.c

tests/amount_set_main.o: tests/amount_set_main.c tests/amount_set_tests.h tests/test_utilities.h
	$(CC) -c $(COMP_FLAG) $*.c

tests/amount_set_tests.o: tests/amount_set_tests.c amount_set.h tests/amount_set_tests.h tests/test_utilities.h
	$(CC) -c $(COMP_FLAG) $*.c

clean:
	rm -f $(MA_OBJS) $(AS_OBJS)  $(EXEC1) $(EXEC2)
