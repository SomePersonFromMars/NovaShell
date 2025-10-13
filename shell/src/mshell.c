#include <stdio.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"

int
main(int argc, char *argv[])
{
	pipelineseq * ln;
	command *com;

	char buf[2048];


	while (fgets(buf, 2048, stdin)){	
		ln = parseline(buf);
		printparsedline(ln);
	}

	return 0;

	ln = parseline("ls -las | grep k | wc ; echo abc > f1 ;  cat < f2 ; echo abc >> f3\n");
	printparsedline(ln);
	printf("\n");
	com = pickfirstcommand(ln);
	printcommand(com,1);

	ln = parseline("sleep 3 &");
	printparsedline(ln);
	printf("\n");
	
	ln = parseline("echo  & abc >> f3\n");
	printparsedline(ln);
	printf("\n");
	com = pickfirstcommand(ln);
	printcommand(com,1);
}
