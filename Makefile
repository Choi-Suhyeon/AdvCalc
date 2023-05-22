AdvCalc: 2-AdvCalc.l 2-AdvCalc.y 2-AdvCalc.h
	bison -d 2-AdvCalc.y
	flex -o 2-AdvCalc.lex.c 2-AdvCalc.l
	gcc -o $@ 2-AdvCalc.tab.c 2-AdvCalc.lex.c 2-AdvCalc.c

clean:
	rm -f *.lex.* *.tab.* AdvCalc