#include <dir.h>

void
foreach_pcx(void (*fn)(char *fname))
{
	struct ffblk ffblk;
	int r;

	for (
		r = findfirst("*.pcx", &ffblk, 0);
		r == 0;
		r = findnext(&ffblk)
	) {
		fn(ffblk.ff_name);
	}
}

int
pcx_present(void)
{
	struct ffblk ffblk;

	return findfirst("*.pcx", &ffblk, 0) == 0;
}
