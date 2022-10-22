#include <linux/stacktrace.h>
#include <stdio.h>

#define HOW_MANY_ENTRIES_TO_STORE 16

int main()
{
	unsigned long stack_entries[HOW_MANY_ENTRIES_TO_STORE];
	struct stack_trace trace = {
		.nr_entries = 0,
		.entries = &stack_entries[0],
		.max_entries = HOW_MANY_ENTRIES_TO_STORE,]
		.skip = 0};
	save_stack_trace(&trace);

    return 0;
}