default:
	gcc -lmraa -lm -pthread -lssl -lcrypto -Wall -g -o lab4c_tls lab4c_tls.c
	gcc -lmraa -lm -pthread -Wall -g -o lab4c_tcp lab4c_tcp.c

run_tcp:
	./lab4c_tcp --id=204600605 --host=131.179.192.136 --log=logfile.txt 18000
	./lab4c_tcp --id=204600605 --host=131.179.192.136 --log=logfile.txt 19000

clean:
	rm *.txt lab4b *.tar.gz lab4c_tcp

check:
	./lab4b --log=logfile.txt --scale=C --period=2

dist:
	tar -zcvf lab4b-204600605.tar.gz lab4b.c README Makefile

