#ifndef _DISPLAY_H_
#define _DISPLAY_H_

/* This should be called once and once only to initialise the display */
void display_init(void);

/* This function is called every time a payload is sent to the target */
void display_deploy(void);

/* This function is called once when we are about to finish executution of
 * the fuzzer */
void display_fini(void);

#endif /* _DISPLAY_H_ */

