#ifndef TOOLS_H
#define TOOLS_H

#define UNUSED(v)	((void)v)
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))

void msleep(unsigned long msec);
void clear_screen(void);

#endif /* TOOLS_H */
