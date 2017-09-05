default:
	gcc -lmraa -lm -pthread -lssl -lcrypto -Wall -g -o lab4c_tls lab4c_tls.c
	gcc -lmraa -lm -pthread -Wall -g -o lab4c_tcp lab4c_tcp.c

run_tcp:
	./lab4c_tcp 18000 --id=204600605 --host=131.179.192.136 --log=logfile.txt

run_tls:
	./lab4c_tls --id=204600605 --host=131.179.192.136 --log=logfile.txt 19000

clean:
	rm *.txt *.tar.gz lab4c_tcp lab4c_tls

dist:
	tar -zcvf lab4c-204600605.tar.gz lab4c_tcp.c lab4c_tls.c README Makefile

