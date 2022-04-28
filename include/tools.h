#ifndef TOOLS_H
#define TOOLS_H

#define UNUSED(v)	((void)v)
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))

/* ANSI escape codes for printing out output to a terminal */
#define RED		"\x1B[0;31m"	/* Green */
#define GRN		"\x1B[0;32m"	/* Yellow */
#define YEL		"\x1B[0;33m"	/* Red */
#define UWHT		"\e[4;37m"	/* Underlined white font */
#define RST		"\033[0m"	/* Reset colour */
#define CLEAR_SCREEN	"\e[1;1H\e[2J"	/* Clear screen */

void msleep(unsigned long msec);
void clear_screen(void);

#endif /* TOOLS_H */
