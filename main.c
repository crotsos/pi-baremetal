#include "led.h"
#include "atags.h"
#include "barrier.h"
#include "framebuffer.h"
#include "interrupts.h"
#include "mailbox.h"
#include "memory.h"
#include "memutils.h"
#include "textutils.h"

/* Use some free memory in the area below the kernel/stack */
#define BUFFER_ADDRESS  0x1000

/* Pull various bits of information from the VideoCore and display it on
 * screen
 */
void mailboxtest(void)
{
	volatile unsigned int *ptr = (unsigned int *) BUFFER_ADDRESS;
	unsigned int count, var;
	unsigned int mem, size;

	console_write(BG_GREEN BG_HALF "Reading from tag mailbox\n\n" BG_BLACK);

	ptr[0] = 8 * 4;		// Total size
	ptr[1] = 0;		// Request

	ptr[2] = 0x40003;	// Display size
	ptr[3] = 8;		// Buffer size
	ptr[4] = 0;		// Request size
	ptr[5] = 0;
	ptr[6] = 0;

	ptr[7] = 0;

	writemailbox(8, BUFFER_ADDRESS);

	var = readmailbox(8);

	/*
	console_write(FG_GREEN "\n\nReading mailbox 8\n" FG_WHITE);

	console_write(COLOUR_PUSH BG_GREEN BG_HALF "Result = 0x");
	console_write(tohex(var, sizeof(var)));
	console_write(COLOUR_POP "\n");

	for(count=0; count<=7; count++)
	{
		console_write("Value ");
		console_write(todec(count, 2));
		console_write(" = 0x");
		console_write(tohex(ptr[count], 4));
		console_write("\n");
	}

	*/

	console_write(COLOUR_PUSH FG_CYAN "Display resolution: " BG_WHITE BG_HALF BG_HALF);
	console_write(todec(ptr[5], 0));
	console_write("x");
	console_write(todec(ptr[6], 0));
	console_write(COLOUR_POP "\n");

	ptr[0] = 8 * 4;		// Total size
	ptr[1] = 0;		// Request

	ptr[2] = 0x40008;	// Display size
	ptr[3] = 8;		// Buffer size
	ptr[4] = 0;		// Request size
	ptr[5] = 0;
	ptr[6] = 0;

	ptr[7] = 0;

	writemailbox(8, BUFFER_ADDRESS);

	var = readmailbox(8);

	console_write(COLOUR_PUSH FG_CYAN "Pitch: " BG_WHITE BG_HALF BG_HALF);
	console_write(todec(ptr[5], 0));
	console_write(" bytes" COLOUR_POP "\n");

	ptr[0] = 200 * 4;	// Total size
	ptr[1] = 0;		// Request

	ptr[2] = 0x50001;	// Command line
	ptr[3] = 195*4;		// Buffer size
	ptr[4] = 0;		// Request size
	
	for(count=5; count<200; count++)
		ptr[count] = 0;

	writemailbox(8, BUFFER_ADDRESS);

	var = readmailbox(8);

	console_write("\n" COLOUR_PUSH FG_RED "Kernel command line: " COLOUR_PUSH BG_RED BG_HALF BG_HALF);
	console_write((char *)(BUFFER_ADDRESS + 20));
	console_write(COLOUR_POP COLOUR_POP "\n\n");


	ptr[0] = 13 * 4;	// Total size
	ptr[1] = 0;		// Request

	ptr[2] = 0x10005;	// ARM memory
	ptr[3] = 8;		// Buffer size
	ptr[4] = 0;		// Request size
	ptr[5] = 0;
	ptr[6] = 0;

	ptr[7] = 0x10006;	// VideoCore memory
	ptr[8] = 8;		// Buffer size
	ptr[9] = 0;		// Request size
	ptr[10] = 0;
	ptr[11] = 0;

	ptr[12] = 0;

	writemailbox(8, BUFFER_ADDRESS);

	var = readmailbox(8);

	mem = ptr[5];
	size = ptr[6];
	var = size / (1024*1024);

	console_write(COLOUR_PUSH FG_YELLOW "ARM memory: " BG_YELLOW BG_HALF BG_HALF "0x");
	console_write(tohex(mem, 4));
	console_write(" - 0x");
	console_write(tohex(mem+size-1, 4));
	console_write(" (");
	console_write(todec(size, 0));
	/* ] appears as an arrow in the SAA5050 character set */
	console_write(" bytes ] ");
	console_write(todec(var, 0));
	console_write(" megabytes)" COLOUR_POP "\n");

	mem = ptr[10];
	size = ptr[11];
	var = size / (1024*1024);
	console_write(COLOUR_PUSH FG_YELLOW "VC memory:  " BG_YELLOW BG_HALF BG_HALF "0x");
	console_write(tohex(mem, 4));
	console_write(" - 0x");
	console_write(tohex(mem+size-1, 4));
	console_write(" (");
	console_write(todec(size, 0));
	console_write(" bytes ] ");
	console_write(todec(var, 0));
	console_write(" megabytes)" COLOUR_POP "\n");
}

/* Call non-existent code at 33MB - should cause a prefetch abort */
static void(*deliberate_prefetch_abort)(void) = (void(*)(void))0x02100000;

/* Main routine - called directly from start.s
 * ARM procedure call standard says the first 3 parameters of a function
 * are r0, r1, r2. These registers are untouched by _start, so will be
 * exactly as the bootloader set them
 * r0 should be 0
 * r1 should be the machine type - 0x0c42 = Raspberry Pi
 * r2 should be the ATAGs structure address (probably 0x100)
 */
void main(unsigned int r0, unsigned int machtype, unsigned int atagsaddr)
{
	led_init();
	fb_init();
	interrupts_init();

	volatile unsigned int *memory;

	/* Say hello */
	console_write("Pi-Baremetal booted\n\n");

	console_write(FG_RED "Machine type is 0x");
	console_write(tohex(machtype, 4));

	if(machtype == 0xc42)
		console_write(", a Broadcom BCM2708 (Raspberry Pi)\n\n" FG_WHITE);
	else
		console_write(". Unknown machine type. Good luck!\n\n" FG_WHITE);

	/* Read in ATAGS */
	print_atags(atagsaddr);
	
	/* Read in some system data */
	mailboxtest();

	/* Test interrupt */
	console_write("\nTest SWI: ");
	asm volatile("swi #1234");

	console_write(FG_YELLOW "\nMMU tests - writing data to memory\n" FG_CYAN);
	memory = (unsigned int *)0x00100000;
	*memory = 0xdeadbeef;
	console_write("0x00100000 = 0x");
	console_write(tohex(*memory, 4));
	console_write("\n");

	memory = (unsigned int *)0x00200000;
	*memory = 0xf00dcafe;
	console_write("0x00200000 = 0x");
	console_write(tohex(*memory, 4));
	console_write("\n");

	/* Write to 32MB */
	memory = (unsigned int *)0x02000000;
	*memory = 0xc0ffee;
	console_write("0x02000000 = 0x");
	console_write(tohex(*memory, 4));
	console_write("\n");

	mem_init();
	console_write(FG_RED "MMU on - "
		"0x00100000 and 0x00200000 swapped, 0x02000000 unmapped\n"
		FG_YELLOW "Reading data from memory\n" FG_CYAN);

	memory = (unsigned int *)0x00100000;
	console_write("0x00100000 = 0x");
	console_write(tohex(*memory, 4));
	console_write("\n");

	memory = (unsigned int *)0x00200000;
	console_write("0x00200000 = 0x");
	console_write(tohex(*memory, 4));
	console_write("\n");
	/* Read from 32MB - should abort */
	memory = (unsigned int *)0x02000000;
	console_write("0x02000000 = 0x");
	console_write(tohex(*memory, 4));
	console_write("\n");

	console_write(FG_WHITE BG_GREEN BG_HALF "\nOK LED flashing under interrupt");

	console_write(BG_BLACK FG_YELLOW
		"\n\nPerforming deliberate prefetch abort: "
		FG_RED BG_RED BG_HALF);
	deliberate_prefetch_abort();

	/* Prefetch abort doesn't return, so it won't get past this point */

	while(1);
}

void main_endloop(void)
{
	console_write(FG_WHITE BG_GREEN BG_HALF "\nPrefetch abort done");

	while(1);
}
