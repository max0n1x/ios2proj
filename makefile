proj2:
	gcc -std=gnu99 -Wall -Wextra -pthread -Werror -pedantic -o proj2 2proj.c

run:
	gcc -std=gnu99 -Wall -Wextra -pthread -Werror -pedantic -o proj2 test1.c
	./proj2 3 2 100 100 100
	sh kontrola-vystupu.sh < proj2.out