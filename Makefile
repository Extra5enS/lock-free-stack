
all:
	g++ serverTest.cpp -pthread -o test
	./test 200 >> data1
	./test 400 >> data1
	./test 600 >> data1
	./test 800 >> data1
	./test 1000 >> data1
	./test 1200 >> data1
	./test 1400 >> data1
	./test 1600 >> data1
	./test 1800 >> data1
	./test 2000 >> data1

clear:
	rm test
