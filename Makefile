
all:
	g++ serverTest.cpp -pthread -o test
	./test 200 >> data
	./test 400 >> data
	./test 600 >> data
	./test 800 >> data
	./test 1000 >> data
	./test 1200 >> data
	./test 1400 >> data
	./test 1600 >> data
	./test 1800 >> data
	./test 2000 >> data

clear:
	rm test
