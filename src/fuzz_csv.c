#include "csv.h"
#include "fuzz_csv.h"
#include "fuzzer.h"

static struct {

	char **fields;

} csv = {0};

void fuzz_handle_csv(void)
{
	csv.fields = split_on_unescaped_newlines(state.mem, state.stat.st_size);
	return;
}


