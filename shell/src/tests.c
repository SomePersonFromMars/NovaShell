#include <stdio.h>

#include "siparse.h"
#include "utils.h"

void testparser(void)
{
	pipelineseq * parsed_line;
	command *com;

	parsed_line = parseline("ls -las | grep k | wc ; echo abc > f1 ;  cat < f2 ; echo abc >> f3\n");
	printparsedline(parsed_line);
	printf("\n");
	com = pickfirstcommand(parsed_line);
	printcommand(com,1);

	parsed_line = parseline("sleep 3 &");
	printparsedline(parsed_line);
	printf("\n");
	
	parsed_line = parseline("echo  & abc >> f3\n");
	printparsedline(parsed_line);
	printf("\n");
	com = pickfirstcommand(parsed_line);
	printcommand(com,1);
}

int
main(int argc, char *argv[])
{
    testparser();
}
